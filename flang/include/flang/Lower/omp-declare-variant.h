//===-- flang/Lower/omp-declare-variant.h -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef FORTRAN_LOWER_OMP_DECLARE_VARIANT_H_
#define FORTRAN_LOWER_OMP_DECLARE_VARIANT_H_

namespace Fortran::lower {
class AbstractConverter;
namespace pft {
struct Evaluation;
} // namespace pft
} // namespace Fortran::lower

namespace Fortran::semantics {
class Symbol;
}

namespace Fortran::lower::omp {

const semantics::Symbol *resolveDeclareVariantCallee(
    const semantics::Symbol &base, const pft::Evaluation &eval,
    AbstractConverter &converter);

} // namespace Fortran::lower::omp

#endif // FORTRAN_LOWER_OMP_DECLARE_VARIANT_H_
