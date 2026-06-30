//===-- lib/Semantics/omp-declare-variant.cpp -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "flang/Semantics/omp-declare-variant.h"
#include "flang/Common/reference.h"
#include "flang/Parser/parse-tree.h"
#include "flang/Semantics/openmp-utils.h"
#include "flang/Semantics/semantics.h"
#include "flang/Semantics/tools.h"
#include "llvm/Frontend/OpenMP/OMP.h"

namespace Fortran::semantics {

using omp::GetArgumentSymbol;
using omp::GetObjectSymbol;

static const parser::traits::OmpContextSelectorSpecification *
GetMatchClauseContextSelector(const parser::OmpDirectiveSpecification &spec) {
  for (const parser::OmpClause &clause : spec.Clauses().v) {
    if (clause.Id() == llvm::omp::Clause::OMPC_match)
      return &std::get<parser::OmpClause::Match>(clause.u).v.v;
  }
  return nullptr;
}

OmpDeclareVariantResolution ResolveOmpDeclareVariant(
    const parser::OmpDeclareVariantDirective &directive,
    SemanticsContext &context) {
  OmpDeclareVariantResolution result;
  const parser::OmpDirectiveSpecification &spec{directive.v};
  const parser::OmpArgumentList &args{spec.Arguments()};
  if (args.v.size() != 1) {
    result.form = OmpDeclareVariantForm::WrongArgCount;
    return result;
  }

  const parser::OmpArgument &arg{args.v.front()};
  if (const auto *names{std::get_if<parser::OmpBaseVariantNames>(&arg.u)}) {
    result.base = GetObjectSymbol(std::get<0>(names->t));
    result.variant = GetObjectSymbol(std::get<1>(names->t));
    result.form = OmpDeclareVariantForm::Names;
  } else if (std::holds_alternative<parser::OmpObject>(arg.u)) {
    result.variant = GetArgumentSymbol(arg);
    const Scope &containingScope{context.FindScope(directive.source)};
    if (const Symbol *
        host{GetProgramUnitContaining(containingScope).symbol()}) {
      result.base = host;
    }
    result.form = OmpDeclareVariantForm::Locator;
  } else {
    result.form = OmpDeclareVariantForm::Invalid;
    return result;
  }

  result.matchSelector = GetMatchClauseContextSelector(spec);
  return result;
}

void RecordOmpDeclareVariantOnBase(
    const parser::OmpDeclareVariantDirective &directive,
    SemanticsContext &context) {
  // This runs from name resolution (OmpAttributeVisitor), before the structure
  // checker has had a chance to diagnose a malformed directive. Bail out
  // quietly on anything we cannot resolve; the structure checker is responsible
  // for emitting diagnostics.
  OmpDeclareVariantResolution resolved{
      ResolveOmpDeclareVariant(directive, context)};
  if (resolved.form == OmpDeclareVariantForm::WrongArgCount ||
      resolved.form == OmpDeclareVariantForm::Invalid)
    return;
  if (!resolved.base || !resolved.variant || !resolved.matchSelector)
    return;

  const Symbol &base{resolved.base->GetUltimate()};
  const Symbol &variant{resolved.variant->GetUltimate()};
  if (!base.detailsIf<SubprogramDetails>())
    return;

  // Avoid recording the same directive twice. Name resolution walks each
  // program unit more than once (ResolveOmpParts runs the OpenMP attribute
  // visitor twice), so without this guard a single directive would be recorded
  // multiple times.
  SubprogramDetails &details{const_cast<Symbol &>(base).get<SubprogramDetails>()};
  for (const OmpDeclareVariantEntry &existing : details.ompDeclareVariants()) {
    if (&existing.variant.get() == &variant &&
        existing.matchSelector == resolved.matchSelector)
      return;
  }

  details.addOmpDeclareVariant(
      OmpDeclareVariantEntry{variant, resolved.matchSelector});
}

} // namespace Fortran::semantics
