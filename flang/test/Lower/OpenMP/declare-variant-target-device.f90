! RUN: %flang_fc1 -emit-fir -fopenmp -fopenmp-version=51 \
! RUN:   -fopenmp-is-target-device %s -o - | FileCheck %s

! On device compilation `OffloadModuleInterface::getIsTargetDevice()` is
! true, so `isInsideOpenMPTargetRegion` returns true module-wide and a
! call to a base subprogram with a matching `device={kind(nohost)}`
! variant is rewritten to the variant — even outside an explicit `omp
! target` region, as long as the enclosing function is alive in the
! device module (here pinned by `declare target to ... device_type(nohost)`).

module m
contains
  subroutine base
    !$omp declare variant (base:vsub) match (device={kind(nohost)})
  contains
    subroutine vsub
    end subroutine
  end subroutine base

  subroutine device_caller
    !$omp declare target to(device_caller) device_type(nohost)
    call base()
  end subroutine device_caller

  subroutine target_region_caller
    !$omp target
    call base()
    !$omp end target
  end subroutine target_region_caller
end module m

! CHECK-LABEL: func.func @_QMmPdevice_caller
! CHECK: fir.call @_QMmFbasePvsub(){{.*}}: () -> ()
! CHECK-NOT: fir.call @_QMmPbase

! CHECK-LABEL: func.func @_QMmPtarget_region_caller
! CHECK: omp.target
! CHECK: fir.call @_QMmFbasePvsub(){{.*}}: () -> ()
