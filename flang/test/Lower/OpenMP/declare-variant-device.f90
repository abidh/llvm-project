! RUN: %flang_fc1 -emit-fir -fopenmp -fopenmp-version=51 %s -o - | FileCheck %s

! Variant resolution driven by `device={kind(...)}` selectors and the
! `OpenMPTargetRegionFrame` state-stack frame pushed during host-side
! lowering of an `omp target` body.

! Variant predicated on a device kind that does not match the host
! compilation context. The base call must be preserved.

subroutine test_device_no_match
  call base()
  !$omp parallel
  call base()
  !$omp end parallel
end subroutine test_device_no_match

subroutine base
  !$omp declare variant (base:vsub) match (device={kind(fpga)})
contains
  subroutine vsub
  end subroutine
end subroutine base

! CHECK-LABEL: func.func @_QPtest_device_no_match
! CHECK: fir.call @_QPbase(){{.*}}: () -> ()
! CHECK: omp.parallel
! CHECK: fir.call @_QPbase(){{.*}}: () -> ()
! CHECK-NOT: fir.call @_QFbasePvsub

! Two variants on the same base, one for the host and one for "nohost".
! On host compilation the host variant matches outside `omp target` (the
! state-stack frame is absent, so OMPContext is built with
! IsDeviceCompilation=false) and the nohost variant matches inside the
! target body (where `OpenMPTargetRegionFrame` pushes IsDeviceCompilation
! to true).

subroutine test_host_vs_target_region
  call dual()
  !$omp target
  call dual()
  !$omp end target
end subroutine test_host_vs_target_region

subroutine dual
  !$omp declare variant (dual:dual_host)   match (device={kind(host)})
  !$omp declare variant (dual:dual_device) match (device={kind(nohost)})
contains
  subroutine dual_host
  end subroutine
  subroutine dual_device
  end subroutine
end subroutine dual

! CHECK-LABEL: func.func @_QPtest_host_vs_target_region
! CHECK: fir.call @_QFdualPdual_host(){{.*}}: () -> ()
! CHECK: omp.target
! CHECK: fir.call @_QFdualPdual_device(){{.*}}: () -> ()
