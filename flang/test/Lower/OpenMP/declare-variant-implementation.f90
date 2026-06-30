! RUN: %flang_fc1 -emit-fir -fopenmp -fopenmp-version=51 %s -o - | FileCheck %s
! RUN: %flang_fc1 -emit-hlfir -fopenmp -fopenmp-version=51 %s -o - | FileCheck %s

! DECLARE VARIANT callee resolution driven by implementation={vendor(...)}
! selectors.  Mirrors metadirective-implementation.f90 to confirm the same
! vendor matching applies to declare-variant callee resolution.

! vendor(llvm) matches under flang: the variant replaces the base call.

! CHECK-LABEL: func.func @_QPtest_vendor_llvm
! CHECK: fir.call @_QFbase_llvmPvsub(){{.*}}: () -> ()
! CHECK-NOT: fir.call @_QPbase_llvm
subroutine test_vendor_llvm
  call base_llvm()
end subroutine test_vendor_llvm

subroutine base_llvm
  !$omp declare variant (base_llvm:vsub) match (implementation={vendor(llvm)})
contains
  subroutine vsub
  end subroutine
end subroutine base_llvm

! An unknown vendor does not match: the base call is kept.

! CHECK-LABEL: func.func @_QPtest_vendor_unknown
! CHECK: fir.call @_QPbase_unknown(){{.*}}: () -> ()
! CHECK-NOT: fir.call @_QFbase_unknownPvsub
subroutine test_vendor_unknown
  call base_unknown()
end subroutine test_vendor_unknown

subroutine base_unknown
  !$omp declare variant (base_unknown:vsub) match (implementation={vendor("unknown")})
contains
  subroutine vsub
  end subroutine
end subroutine base_unknown
