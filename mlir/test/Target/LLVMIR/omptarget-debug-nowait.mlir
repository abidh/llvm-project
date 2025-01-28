// RUN: mlir-translate -mlir-to-llvmir %s

#di_basic_type = #llvm.di_basic_type<tag = DW_TAG_base_type, name = "real", sizeInBits = 32, encoding = DW_ATE_float>
#di_file = #llvm.di_file<"test.f90" in "">
#di_null_type = #llvm.di_null_type
#di_compile_unit = #llvm.di_compile_unit<id = distinct[0]<>, sourceLanguage = DW_LANG_Fortran95, file = #di_file, producer = "flang version 20.0.0 (/home/haqadeer/work/src/llvm-project/flang 97c3a990f05606cb807faf53bc41302fb62c7980)", isOptimized = false, emissionKind = Full>
#di_subroutine_type = #llvm.di_subroutine_type<callingConvention = DW_CC_program, types = #di_null_type>
#di_subprogram = #llvm.di_subprogram<recId = distinct[1]<>, id = distinct[2]<>, compileUnit = #di_compile_unit, scope = #di_file, name = "nowait_reproducer", linkageName = "_QQmain", file = #di_file, line = 6, scopeLine = 6, subprogramFlags = "Definition|MainSubprogram", type = #di_subroutine_type>

module attributes {omp.is_target_device = false} {
  llvm.func @_QQmain() {
    %0 = llvm.mlir.constant(1 : i64) : i64 loc(#loc2)
    %1 = llvm.alloca %0 x f32 {bindc_name = "x"} : (i64) -> !llvm.ptr loc(#loc2)
    %2 = llvm.mlir.constant(1 : i64) : i64 loc(#loc2)
    %3 = omp.map.info var_ptr(%1 : !llvm.ptr, f32) map_clauses(tofrom) capture(ByRef) -> !llvm.ptr loc(#loc2)
    omp.target nowait map_entries(%3 -> %arg0 : !llvm.ptr) {
      %4 = llvm.mlir.constant(5.000000e+00 : f32) : f32 loc(#loc3)
      llvm.store %4, %arg0 : f32, !llvm.ptr loc(#loc3)
      omp.terminator loc(#loc3)
    }
    llvm.return
  } loc(#loc8)
}
#loc1 = loc("test.f90":6:7)
#loc2 = loc("test.f90":9:12)
#loc3 = loc("test.f90":11:10)
#loc8 = loc(fused<#di_subprogram>[#loc1])


