//===-- flang/Semantics/omp-declare-variant.h --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef FORTRAN_SEMANTICS_OMP_DECLARE_VARIANT_H_
#define FORTRAN_SEMANTICS_OMP_DECLARE_VARIANT_H_

#include "flang/Common/reference.h"
#include "llvm/Frontend/OpenMP/OMPContext.h"
#include <vector>

namespace Fortran::parser {
struct OmpDeclareVariantDirective;
} // namespace Fortran::parser

namespace Fortran::semantics {
class SemanticsContext;
class Symbol;

struct OmpDeclareVariantEntry {
  common::Reference<const Symbol> variant;
  llvm::omp::VariantMatchInfo match;
};

void appendConstructTraitsForDirective(llvm::omp::Directive dir,
    llvm::omp::VariantMatchInfo &vmi);

void RecordOmpDeclareVariantOnBase(
    const parser::OmpDeclareVariantDirective &, SemanticsContext &);

} // namespace Fortran::semantics

#endif // FORTRAN_SEMANTICS_OMP_DECLARE_VARIANT_H_
