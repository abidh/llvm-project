//===-- lib/Semantics/omp-declare-variant.cpp -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "flang/Semantics/omp-declare-variant.h"
#include "flang/Common/idioms.h"
#include "flang/Common/reference.h"
#include "flang/Evaluate/fold.h"
#include "flang/Evaluate/tools.h"
#include "flang/Parser/parse-tree.h"
#include "flang/Semantics/openmp-directive-sets.h"
#include "flang/Semantics/openmp-utils.h"
#include "flang/Semantics/semantics.h"
#include "flang/Semantics/tools.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/Frontend/OpenMP/OMP.h"
#include "llvm/Frontend/OpenMP/OMPContext.h"

namespace Fortran::semantics {

using omp::GetArgumentSymbol;
using omp::GetObjectSymbol;

void appendConstructTraitsForDirective(llvm::omp::Directive dir,
    llvm::omp::VariantMatchInfo &vmi) {
  auto add = [&](llvm::omp::TraitProperty prop) {
    vmi.addTrait(prop, llvm::omp::getOpenMPContextTraitPropertyName(prop, ""));
  };
  if (llvm::omp::allTargetSet.test(dir))
    add(llvm::omp::TraitProperty::construct_target_target);
  if (llvm::omp::allTeamsSet.test(dir))
    add(llvm::omp::TraitProperty::construct_teams_teams);
  if (llvm::omp::allParallelSet.test(dir))
    add(llvm::omp::TraitProperty::construct_parallel_parallel);
  if (llvm::omp::allDoSet.test(dir))
    add(llvm::omp::TraitProperty::construct_for_for);
  if (llvm::omp::allSimdSet.test(dir))
    add(llvm::omp::TraitProperty::construct_simd_simd);
}

static const std::list<parser::OmpTraitProperty> &getTraitPropertyList(
    const parser::OmpTraitSelector &selector) {
  static const std::list<parser::OmpTraitProperty> empty{};
  const auto &maybeProps{
      std::get<std::optional<parser::OmpTraitSelector::Properties>>(selector.t)};
  if (!maybeProps)
    return empty;
  return std::get<std::list<parser::OmpTraitProperty>>(maybeProps->t);
}

static void addTraitPropertiesFromSelector(
    llvm::omp::TraitSet set, const parser::OmpTraitSelector &selector,
    llvm::omp::VariantMatchInfo &vmi, evaluate::FoldingContext &foldingContext) {
  const auto &traitName{std::get<parser::OmpTraitSelectorName>(selector.t)};
  const std::list<parser::OmpTraitProperty> &properties{
      getTraitPropertyList(selector)};

  if (set == llvm::omp::TraitSet::user &&
      std::holds_alternative<parser::OmpTraitSelectorName::Value>(traitName.u) &&
      std::get<parser::OmpTraitSelectorName::Value>(traitName.u) ==
          parser::OmpTraitSelectorName::Value::Condition) {
    if (properties.size() != 1) {
      vmi.addTrait(llvm::omp::TraitProperty::user_condition_false, "<condition>");
      return;
    }
    const parser::OmpTraitProperty &property = properties.front();
    const auto &scalarExpr{std::get<parser::ScalarExpr>(property.u)};
    if (const auto *expr{GetExpr(nullptr, scalarExpr)}) {
      auto folded{evaluate::Fold(foldingContext, common::Clone(*expr))};
      if (auto val{omp::GetLogicalValue(folded)}) {
        vmi.addTrait(*val ? llvm::omp::TraitProperty::user_condition_true
                          : llvm::omp::TraitProperty::user_condition_false,
            "<condition>");
        return;
      }
    }
    vmi.addTrait(llvm::omp::TraitProperty::user_condition_false, "<condition>");
    return;
  }

  std::optional<llvm::APSInt> score;
  llvm::APInt *scorePtr{nullptr};
  if (!properties.empty()) {
    const auto &maybeProps{
        std::get<std::optional<parser::OmpTraitSelector::Properties>>(selector.t)};
    const auto &[maybeScore, ignoredProperties]{maybeProps->t};
    (void)ignoredProperties;
    if (maybeScore) {
      if (auto val{GetIntValue(*maybeScore)})
        score = llvm::APSInt(llvm::APInt(64, *val), /*isUnsigned=*/true);
      if (score)
        scorePtr = &*score;
    }

    if (set == llvm::omp::TraitSet::construct) {
      if (const auto *dir{std::get_if<llvm::omp::Directive>(&traitName.u)}) {
        appendConstructTraitsForDirective(*dir, vmi);
        return;
      }
    }

    if (!std::holds_alternative<parser::OmpTraitSelectorName::Value>(
            traitName.u))
      return;
    using TN = parser::OmpTraitSelectorName::Value;
    const TN selectorValue{std::get<TN>(traitName.u)};
    llvm::omp::TraitSelector selectorKind{llvm::omp::TraitSelector::invalid};
    switch (selectorValue) {
    case TN::Arch:
      selectorKind = llvm::omp::TraitSelector::device_arch;
      break;
    case TN::Isa:
      selectorKind = llvm::omp::TraitSelector::device_isa;
      break;
    case TN::Kind:
      selectorKind = llvm::omp::TraitSelector::device_kind;
      break;
    case TN::Simd:
      selectorKind = llvm::omp::TraitSelector::construct_simd;
      break;
    default:
      break;
    }
    if (selectorKind == llvm::omp::TraitSelector::invalid)
      return;

    for (const parser::OmpTraitProperty &property : properties) {
      if (const auto *name{
              std::get_if<parser::OmpTraitPropertyName>(&property.u)}) {
        if (auto prop{llvm::omp::getOpenMPContextTraitPropertyKind(
                set, selectorKind, parser::ToLowerCaseLetters(name->v))};
            prop != llvm::omp::TraitProperty::invalid)
          vmi.addTrait(set, prop, name->v, scorePtr);
      }
    }
  } else if (set == llvm::omp::TraitSet::construct) {
    if (const auto *dir{std::get_if<llvm::omp::Directive>(&traitName.u)})
      appendConstructTraitsForDirective(*dir, vmi);
  }
}

static llvm::omp::VariantMatchInfo getVariantMatchInfo(
    const parser::traits::OmpContextSelectorSpecification &contextSelector,
    evaluate::FoldingContext &foldingContext) {
  llvm::omp::VariantMatchInfo vmi;
  for (const parser::OmpTraitSetSelector &traitSet : contextSelector.v) {
    const auto &setName{std::get<parser::OmpTraitSetSelectorName>(traitSet.t)};
    llvm::omp::TraitSet set{llvm::omp::TraitSet::invalid};
    switch (setName.v) {
    case parser::OmpTraitSetSelectorName::Value::Construct:
      set = llvm::omp::TraitSet::construct;
      break;
    case parser::OmpTraitSetSelectorName::Value::Device:
      set = llvm::omp::TraitSet::device;
      break;
    case parser::OmpTraitSetSelectorName::Value::Implementation:
      set = llvm::omp::TraitSet::implementation;
      break;
    case parser::OmpTraitSetSelectorName::Value::Target_Device:
      set = llvm::omp::TraitSet::target_device;
      break;
    case parser::OmpTraitSetSelectorName::Value::User:
      set = llvm::omp::TraitSet::user;
      break;
    }
    if (set == llvm::omp::TraitSet::invalid)
      continue;
    for (const parser::OmpTraitSelector &selector :
         std::get<std::list<parser::OmpTraitSelector>>(traitSet.t))
      addTraitPropertiesFromSelector(set, selector, vmi, foldingContext);
  }
  return vmi;
}

static const parser::traits::OmpContextSelectorSpecification *
getMatchClauseContextSelector(const parser::OmpDirectiveSpecification &spec) {
  for (const parser::OmpClause &clause : spec.Clauses().v) {
    if (clause.Id() == llvm::omp::Clause::OMPC_match)
      return &std::get<parser::OmpClause::Match>(clause.u).v.v;
  }
  return nullptr;
}

void RecordOmpDeclareVariantOnBase(
    const parser::OmpDeclareVariantDirective &directive,
    SemanticsContext &context) {
  const parser::OmpDirectiveSpecification &spec{directive.v};
  const parser::OmpArgumentList &args{spec.Arguments()};
  if (args.v.size() != 1)
    return;

  const Symbol *base{nullptr};
  const Symbol *variant{nullptr};
  const parser::OmpArgument &arg{args.v.front()};
  if (const auto *names{std::get_if<parser::OmpBaseVariantNames>(&arg.u)}) {
    base = GetObjectSymbol(std::get<0>(names->t));
    variant = GetObjectSymbol(std::get<1>(names->t));
  } else if (std::holds_alternative<parser::OmpLocator>(arg.u)) {
    variant = GetArgumentSymbol(arg);
    const Scope &containingScope{context.FindScope(directive.source)};
    if (const Symbol *
        host{GetProgramUnitContaining(containingScope).symbol()}) {
      base = host;
    }
  }
  if (!base || !variant)
    return;

  base = &base->GetUltimate();
  variant = &variant->GetUltimate();
  if (!base->detailsIf<SubprogramDetails>())
    return;

  const parser::traits::OmpContextSelectorSpecification *matchSelector{
      getMatchClauseContextSelector(spec)};
  if (!matchSelector)
    return;

  OmpDeclareVariantEntry entry{
      *variant,
      getVariantMatchInfo(*matchSelector, context.foldingContext())};
  const_cast<Symbol &>(*base).get<SubprogramDetails>().addOmpDeclareVariant(
      std::move(entry));
}

} // namespace Fortran::semantics
