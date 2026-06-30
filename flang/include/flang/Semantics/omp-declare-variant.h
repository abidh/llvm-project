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
#include <vector>

namespace Fortran::parser {
struct OmpDeclareVariantDirective;
inline namespace traits {
struct OmpContextSelectorSpecification;
} // namespace traits
} // namespace Fortran::parser

namespace Fortran::semantics {
class SemanticsContext;
class Symbol;

// A DECLARE VARIANT directive recorded on its base procedure. Lowering
// builds the VariantMatchInfo on demand (see resolveDeclareVariantCallee).
struct OmpDeclareVariantEntry {
  common::Reference<const Symbol> variant;
  const parser::traits::OmpContextSelectorSpecification *matchSelector{nullptr};
};

// Kind of a DECLARE VARIANT directive's argument(s).
enum class OmpDeclareVariantArgKind {
  WrongArgCount, // not exactly one argument
  Invalid, // argument is neither [base-name:]variant-name nor a bare name
  Names, // base-name:variant-name
  OmittedBaseName, // variant-name only; base is the host procedure
};

// Pieces of a DECLARE VARIANT directive resolved from the parse tree.
struct OmpDeclareVariantResolution {
  OmpDeclareVariantArgKind kind{OmpDeclareVariantArgKind::Invalid};
  const Symbol *base{nullptr};
  const Symbol *variant{nullptr};
  const parser::traits::OmpContextSelectorSpecification *matchSelector{nullptr};
};

OmpDeclareVariantResolution ResolveOmpDeclareVariant(
    const parser::OmpDeclareVariantDirective &, SemanticsContext &);

void RecordOmpDeclareVariantOnBase(
    const parser::OmpDeclareVariantDirective &, SemanticsContext &);

} // namespace Fortran::semantics

#endif // FORTRAN_SEMANTICS_OMP_DECLARE_VARIANT_H_
