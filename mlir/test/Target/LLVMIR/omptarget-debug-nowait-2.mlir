// RUN: mlir-translate -mlir-to-llvmir %s

#di_basic_type = #llvm.di_basic_type<tag = DW_TAG_base_type, name = "integer", sizeInBits = 32, encoding = DW_ATE_signed>
#di_basic_type1 = #llvm.di_basic_type<tag = DW_TAG_base_type, name = "real", sizeInBits = 32, encoding = DW_ATE_float>
#di_file = #llvm.di_file<"test16_reduced-host-x86_64-unknown-linux-gnu.i" in "/home/haqadeer/work/fortran/t1">
#di_file1 = #llvm.di_file<"test16_reduced.f90" in "/home/haqadeer/work/fortran/t1/..">
#di_null_type = #llvm.di_null_type
#loc6 = loc("/home/haqadeer/work/fortran/t1/../test16_reduced.f90":13:7)
#loc8 = loc("/home/haqadeer/work/fortran/t1/../test16_reduced.f90":18:7)
#di_compile_unit = #llvm.di_compile_unit<id = distinct[0]<>, sourceLanguage = DW_LANG_Fortran95, file = #di_file, producer = "flang version 20.0.0 (/home/haqadeer/work/src/llvm-project/flang 53a596431289fab939c6a2f320a1dd5a99c09d52)", isOptimized = false, emissionKind = Full>
#di_subroutine_type = #llvm.di_subroutine_type<callingConvention = DW_CC_program, types = #di_null_type>
#di_subprogram = #llvm.di_subprogram<recId = distinct[1]<>, id = distinct[2]<>, compileUnit = #di_compile_unit, scope = #di_file, name = "nowait_reproducer", linkageName = "_QQmain", file = #di_file1, line = 6, scopeLine = 6, subprogramFlags = "Definition|MainSubprogram", type = #di_subroutine_type>
#di_local_variable = #llvm.di_local_variable<scope = #di_subprogram, name = "i", file = #di_file, line = 9, type = #di_basic_type>
#di_local_variable1 = #llvm.di_local_variable<scope = #di_subprogram, name = "x", file = #di_file, line = 10, type = #di_basic_type1>
module attributes {omp.is_target_device = false, omp.target_triples = ["amdgcn-amd-amdhsa"]} {
  llvm.func @_QQmain() {
    %0 = llvm.mlir.constant(1 : i64) : i64
    %1 = llvm.alloca %0 x f32 {bindc_name = "x"} : (i64) -> !llvm.ptr
    %2 = llvm.mlir.constant(1 : i64) : i64
    %3 = llvm.alloca %2 x i32 {bindc_name = "i"} : (i64) -> !llvm.ptr
    %4 = llvm.mlir.constant(1 : i64) : i64
    %5 = llvm.mlir.constant(1 : i64) : i64
    %6 = omp.map.info var_ptr(%1 : !llvm.ptr, f32) map_clauses(tofrom) capture(ByRef) -> !llvm.ptr {name = "x"}
    %7 = omp.map.info var_ptr(%3 : !llvm.ptr, i32) map_clauses(implicit, exit_release_or_enter_alloc) capture(ByCopy) -> !llvm.ptr {name = "i"}
    omp.target nowait map_entries(%6 -> %arg0, %7 -> %arg1 : !llvm.ptr, !llvm.ptr) {
      %8 = llvm.mlir.constant(0 : index) : i64
      %9 = llvm.mlir.constant(100 : index) : i64
      %10 = llvm.mlir.constant(1.000000e+00 : f32) : f32
      %11 = llvm.mlir.constant(1 : index) : i64
      %12 = llvm.trunc %11 : i64 to i32
      llvm.br ^bb1(%12, %9 : i32, i64)
    ^bb1(%13: i32, %14: i64):  // 2 preds: ^bb0, ^bb2
      %15 = llvm.icmp "sgt" %14, %8 : i64
      llvm.cond_br %15, ^bb2, ^bb3
    ^bb2:  // pred: ^bb1
      llvm.store %13, %arg1 : i32, !llvm.ptr
      %16 = llvm.load %arg0 : !llvm.ptr -> f32
      %17 = llvm.fadd %16, %10 {fastmathFlags = #llvm.fastmath<contract>} : f32
      llvm.store %17, %arg0 : f32, !llvm.ptr
      %18 = llvm.load %arg1 : !llvm.ptr -> i32
      %19 = llvm.add %18, %12 overflow<nsw> : i32
      %20 = llvm.sub %14, %11 : i64
      llvm.br ^bb1(%19, %20 : i32, i64)
    ^bb3:  // pred: ^bb1
      llvm.store %13, %arg1 : i32, !llvm.ptr
      omp.terminator
    } loc(#loc4)
    llvm.return
  } loc(#loc11)
}
#loc = loc("/home/haqadeer/work/fortran/t1/test16_reduced-host-x86_64-unknown-linux-gnu.i":0:0)
#loc1 = loc("/home/haqadeer/work/fortran/t1/../test16_reduced.f90":6:7)
#loc2 = loc("/home/haqadeer/work/fortran/t1/../test16_reduced.f90":10:12)
#loc3 = loc("/home/haqadeer/work/fortran/t1/../test16_reduced.f90":9:15)
#loc4 = loc("/home/haqadeer/work/fortran/t1/../test16_reduced.f90":12:10)
#loc5 = loc(unknown)
#loc7 = loc("/home/haqadeer/work/fortran/t1/../test16_reduced.f90":14:7)
#di_subroutine_type1 = #llvm.di_subroutine_type<callingConvention = DW_CC_normal, types = #di_null_type, #di_basic_type, #di_basic_type, #di_basic_type, #di_basic_type>
#di_subroutine_type2 = #llvm.di_subroutine_type<callingConvention = DW_CC_normal, types = #di_null_type>
#di_subroutine_type3 = #llvm.di_subroutine_type<callingConvention = DW_CC_normal, types = #di_basic_type, #di_basic_type, #di_basic_type, #di_basic_type>
#loc9 = loc("x"(#loc2))
#loc10 = loc("i"(#loc3))
#di_subprogram1 = #llvm.di_subprogram<recId = distinct[3]<>, scope = #di_file, name = "_FortranAProgramStart", linkageName = "_FortranAProgramStart", file = #di_file1, line = 18, scopeLine = 18, type = #di_subroutine_type1>
#di_subprogram2 = #llvm.di_subprogram<recId = distinct[4]<>, scope = #di_file, name = "_FortranAProgramEndStatement", linkageName = "_FortranAProgramEndStatement", file = #di_file1, line = 18, scopeLine = 18, type = #di_subroutine_type2>
#di_subprogram3 = #llvm.di_subprogram<recId = distinct[5]<>, id = distinct[6]<>, compileUnit = #di_compile_unit, scope = #di_file, name = "main", linkageName = "main", file = #di_file1, line = 18, scopeLine = 18, subprogramFlags = Definition, type = #di_subroutine_type3>
#loc11 = loc(fused<#di_subprogram>[#loc1])
#loc12 = loc(fused<#di_subprogram1>[#loc8])
#loc13 = loc(fused<#di_subprogram2>[#loc8])
#loc14 = loc(fused<#di_subprogram3>[#loc8])
