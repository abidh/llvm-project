! RUN: %flang_fc1 -emit-llvm -debug-info-kind=standalone %s -o - | FileCheck %s

module testmod
  integer :: var_a = 10, var_b = 20, var_c = 30
end module testmod

module testmod2
  real :: var_x = 1.0, var_y = 2.0
end module testmod2

program test_use
  use testmod, only: var_b, var_d => var_c
  use testmod2, var_z => var_y
  implicit none
  print *, var_b
  print *, var_d
  print *, var_z
end program

! CHECK-DAG: !DIModule(scope: !{{.*}}, name: "testmod"
! CHECK-DAG: !DIModule(scope: !{{.*}}, name: "testmod2"

! CHECK-DAG: distinct !DIGlobalVariable(name: "var_b", linkageName: "_QMtestmodEvar_b"
! CHECK-DAG: distinct !DIGlobalVariable(name: "var_c", linkageName: "_QMtestmodEvar_c"
! CHECK-DAG: distinct !DIGlobalVariable(name: "var_y", linkageName: "_QMtestmod2Evar_y"

! CHECK-DAG: distinct !DISubprogram(name: "TEST_USE", linkageName: "_QQmain"{{.*}}retainedNodes:

! CHECK-DAG: !DIImportedEntity(tag: DW_TAG_imported_declaration{{.*}}name: "var_d"
! CHECK-DAG: !DIImportedEntity(tag: DW_TAG_imported_declaration{{.*}}scope{{.*}}entity{{.*}}file{{.*}}line
! CHECK-DAG: !DIImportedEntity(tag: DW_TAG_imported_module{{.*}}elements:
! CHECK-DAG: !DIImportedEntity(tag: DW_TAG_imported_declaration{{.*}}name: "var_z"

