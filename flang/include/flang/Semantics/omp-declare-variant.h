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

// A DECLARE VARIANT directive recorded on its base procedure. Rather than
// storing a lowered llvm::omp::VariantMatchInfo (which would pull OMPContext.h
// into symbol.h), keep a pointer to the parsed MATCH-clause context selector.
// The pointer references the parse tree of the current compilation, which
// outlives the symbol table; lowering rebuilds the VariantMatchInfo on demand
// (see resolveDeclareVariantCallee). Keeping the parsed selector rather than a
// lowered form is also meant to make recording this in module files feasible
// later, but that module-file support is a follow-up and is not handled here.
struct OmpDeclareVariantEntry {
  common::Reference<const Symbol> variant;
  const parser::traits::OmpContextSelectorSpecification *matchSelector{nullptr};
};

// Form of the single argument to a DECLARE VARIANT directive.
enum class OmpDeclareVariantForm {
  WrongArgCount, // not exactly one argument
  Invalid, // argument is neither [base-name:]variant-name nor a locator
  Names, // base-name:variant-name
  Locator, // a single locator naming the variant; base is the host procedure
};

// Pieces of a DECLARE VARIANT directive resolved from the parse tree, shared by
// the structure checker and the recorder so both use a single resolution path.
// Symbols are raw (not ultimate); callers apply GetUltimate() as needed. No
// diagnostics are emitted here -- the structure checker is responsible for
// diagnosing any problems.
struct OmpDeclareVariantResolution {
  OmpDeclareVariantForm form{OmpDeclareVariantForm::Invalid};
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
