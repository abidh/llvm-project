! RUN: bbc -emit-fir %s -o - | FileCheck %s

module testmod
  integer :: var_a = 10, var_b = 20, var_c = 30
end module testmod

module testmod2
  real :: var_x = 1.0, var_y = 2.0
end module testmod2

program test_use
  use testmod, only: var_b, var_c
  use testmod2, var_z => var_y
  implicit none
  print *, var_b
  print *, var_c
  print *, var_z
end program

! CHECK-DAG: fir.global @_QMtestmodEvar_a
! CHECK-DAG: fir.global @_QMtestmodEvar_b
! CHECK-DAG: fir.global @_QMtestmodEvar_c
! CHECK-DAG: fir.global @_QMtestmod2Evar_x
! CHECK-DAG: fir.global @_QMtestmod2Evar_y

! CHECK-LABEL: func.func @_QQmain()
! CHECK: fir.use_stmt @testmod only_symbols{{.*}}@_QMtestmodEvar_b{{.*}}@_QMtestmodEvar_c
! CHECK: fir.use_stmt @testmod2 renames{{.*}}#fir.use_rename<"var_z", @_QMtestmod2Evar_y>


