! RUN: bbc -emit-fir %s -o - | FileCheck %s

! CHECK: module attributes
! CHECK-SAME: fir.module_debug_info = [#fir.module_debug_info<"test_mod", [[@LINE+2]]>, #fir.module_debug_info<"another_mod", [[@LINE+10]]>]

module test_mod
  integer :: mod_var
contains
  subroutine test_sub()
    mod_var = 100
  end subroutine test_sub
end module test_mod

module another_mod
  real :: x
contains
  function get_value() result(res)
    real :: res
    res = 42.0
  end function get_value
end module another_mod

program main
  use test_mod
  use another_mod
  call test_sub()
  x = get_value()
end program main

