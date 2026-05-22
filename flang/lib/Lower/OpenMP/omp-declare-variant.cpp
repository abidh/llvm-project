//===-- lib/Lower/omp-declare-variant.cpp ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "flang/Lower/omp-declare-variant.h"
#include "flang/Lower/AbstractConverter.h"
#include "Utils.h"
#include "flang/Lower/PFTBuilder.h"
#include "flang/Parser/parse-tree.h"
#include "flang/Semantics/omp-declare-variant.h"
#include "flang/Semantics/symbol.h"
#include "flang/Optimizer/Dialect/Support/FIRContext.h"
#include "llvm/Frontend/OpenMP/OMPContext.h"
#include "llvm/TargetParser/Triple.h"

namespace Fortran::lower::omp {

static void collectConstructTraitsFromEvaluation(
    const pft::Evaluation &eval,
    llvm::SmallVectorImpl<llvm::omp::TraitProperty> &traits) {
  llvm::SmallVector<llvm::omp::TraitProperty, 8> reversed;
  for (const pft::Evaluation *parent = eval.parentConstruct; parent;
       parent = parent->parentConstruct) {
    if (const auto *ompConstruct =
            parent->getIf<parser::OpenMPConstruct>()) {
      const parser::OmpDirectiveSpecification *spec{nullptr};
      common::visit(
          common::visitors{
              [&](const parser::OmpBlockConstruct &c) {
                spec = &c.BeginDir();
              },
              [&](const parser::OpenMPLoopConstruct &c) {
                spec = &c.BeginDir();
              },
              [&](const parser::OpenMPStandaloneConstruct &) {},
              [&](const auto &) {},
          },
          ompConstruct->u);
      if (!spec)
        continue;
      const llvm::omp::Directive dirId{spec->DirId()};
      llvm::omp::VariantMatchInfo scratch;
      semantics::appendConstructTraitsForDirective(dirId, scratch);
      for (unsigned bit : scratch.RequiredTraits.set_bits())
        reversed.push_back(static_cast<llvm::omp::TraitProperty>(bit));
    }
  }
  traits.assign(reversed.rbegin(), reversed.rend());
}

static llvm::omp::OMPContext
buildOpenMPContext(AbstractConverter &converter,
                   llvm::ArrayRef<llvm::omp::TraitProperty> constructTraits) {
  mlir::ModuleOp module{converter.getModuleOp()};
  llvm::Triple hostTriple{fir::getTargetTriple(module)};
  llvm::Triple offloadTriple{hostTriple};

  const bool isDeviceCompilation{
      omp::isInsideOpenMPTargetRegion(converter) ||
      llvm::any_of(constructTraits, [](llvm::omp::TraitProperty p) {
        return p == llvm::omp::TraitProperty::construct_target_target;
      })};

  llvm::omp::OMPContext ctx(isDeviceCompilation, hostTriple, offloadTriple,
                            /*DeviceNum=*/-1);
  for (llvm::omp::TraitProperty prop : constructTraits)
    ctx.addTrait(prop);
  return ctx;
}

const semantics::Symbol *resolveDeclareVariantCallee(
    const semantics::Symbol &base, const pft::Evaluation &eval,
    AbstractConverter &converter) {
  const semantics::Symbol &ultimate{base.GetUltimate()};
  const auto *details{ultimate.detailsIf<semantics::SubprogramDetails>()};
  if (!details || details->ompDeclareVariants().empty())
    return &ultimate;

  llvm::SmallVector<llvm::omp::VariantMatchInfo, 4> vmis;
  llvm::SmallVector<const semantics::Symbol *, 4> variants;
  for (const semantics::OmpDeclareVariantEntry &entry :
       details->ompDeclareVariants()) {
    vmis.push_back(entry.match);
    variants.push_back(&entry.variant.get());
  }

  llvm::SmallVector<llvm::omp::TraitProperty, 8> constructTraits;
  collectConstructTraitsFromEvaluation(eval, constructTraits);
  llvm::omp::OMPContext ompCtx{
      buildOpenMPContext(converter, constructTraits)};

  const int bestIdx{
      llvm::omp::getBestVariantMatchForContext(vmis, ompCtx)};
  if (bestIdx < 0)
    return &ultimate;
  return variants[bestIdx];
}

} // namespace Fortran::lower::omp
