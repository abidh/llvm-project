// RUN: fir-opt --add-debug-info --mlir-print-debuginfo %s | FileCheck %s

module {
  func.func @_QMmodPbuild(%arg0: !fir.ref<!fir.type<_QMmodTcart{x1:i32}>>, %arg1: !fir.ref<i32> {fir.bindc_name = "x"}) attributes {fir.internal_name = "_QMmodPbuild"} {
    %0 = fir.undefined !fir.dscope
    // %arg0 is the return value (moved to first arg) - no dummy_scope, no arg number
    %1 = fircg.ext_declare %arg0 {uniq_name = "_QMmodFbuildEbuild"} : (!fir.ref<!fir.type<_QMmodTcart{x1:i32}>>) -> !fir.ref<!fir.type<_QMmodTcart{x1:i32}>>
    // %arg1 is the actual argument - has dummy_scope and arg 1
    %2 = fircg.ext_declare %arg1 dummy_scope %0 arg 1 {uniq_name = "_QMmodFbuildEx"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %3 = fir.load %2 : !fir.ref<i32>
    %4 = fir.coordinate_of %1, x1 : (!fir.ref<!fir.type<_QMmodTcart{x1:i32}>>) -> !fir.ref<i32>
    fir.store %3 to %4 : !fir.ref<i32>
    return
  } loc(#loc1)
}

#loc1 = loc("test.f90":6:1)


// CHECK-DAG: #[[CART_TYPE:.*]] = #llvm.di_composite_type<{{.*}}tag = DW_TAG_structure_type, name = "cart"
// CHECK-DAG: #[[INT_TYPE:.*]] = #llvm.di_basic_type<{{.*}}name = "integer"
// CHECK-DAG: #[[SUBR_TYPE:.*]] = #llvm.di_subroutine_type<{{.*}}types = #[[CART_TYPE]], #[[INT_TYPE]]>
// CHECK-DAG: #llvm.di_subprogram<{{.*}}name = "build"{{.*}}type = #[[SUBR_TYPE]]
