! RUN: %flang_fc1 -triple amdgcn-amd-amdhsa -emit-llvm -fopenmp  -fopenmp-is-target-device -debug-info-kind=standalone %s -o - | FileCheck  %s

subroutine fff(x, y)
  implicit none
  integer :: y(:)
  integer :: x

!$omp target map(tofrom: x) map(tofrom: y)
    x = 5
    y = 10
!$omp end target

end subroutine fff

!CHECK: define{{.*}}amdgpu_kernel void @[[FN:[0-9a-zA_Z_]+]]({{.*}}){{.*}}!dbg ![[SP:[0-9]+]]
!CHECK: #dbg_declare(ptr addrspace(5) {{.*}}, ![[X:[0-9]+]], !DIExpression(DIOpArg(0, ptr addrspace(5)), DIOpDeref(ptr)), {{.*}})
!CHECK: #dbg_declare(ptr addrspace(5) {{.*}}, ![[Y:[0-9]+]], !DIExpression(DIOpArg(0, ptr addrspace(5)), DIOpDeref(ptr)), {{.*}})
!CHECK: }

! CHECK-DAG: ![[SP]] = {{.*}}!DISubprogram(name: "[[FN]]"{{.*}}retainedNodes: ![[VARS:[0-9]+]])
! CHECK-DAG: ![[X]] = !DILocalVariable(name: "x", arg: 2, scope: ![[SP]]{{.*}}type: ![[TYX:[0-9]+]])
! CHECK-DAG: ![[TYX]] = !DIDerivedType(tag: DW_TAG_reference_type, baseType: ![[INT:[0-9]+]])
! CHECK-DAG: ![[INT]] = !DIBasicType(name: "integer", size: 32, encoding: DW_ATE_signed)
! CHECK-DAG: ![[Y]] = !DILocalVariable(name: "y", arg: 3, scope: ![[SP]]{{.*}}type: ![[TYY:[0-9]+]])
! CHECK-DAG: ![[TYY]] = !DIDerivedType(tag: DW_TAG_reference_type, baseType: ![[ARR:[0-9]+]])
! CHECK-DAG: ![[ARR]] = !DICompositeType(tag: DW_TAG_array_type, baseType: ![[INT]]{{.*}})
! CHECK-DAG: ![[VARS]] = !{![[X]], ![[Y]]}