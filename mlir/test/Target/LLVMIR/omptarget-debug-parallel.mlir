// RUN: mlir-translate -mlir-to-llvmir %s | FileCheck %s

#di_basic_type = #llvm.di_basic_type<tag = DW_TAG_base_type, name = "integer",
 sizeInBits = 32, encoding = DW_ATE_signed>
#di_file = #llvm.di_file<"target7.f90" in "">
#di_null_type = #llvm.di_null_type
#di_compile_unit = #llvm.di_compile_unit<id = distinct[0]<>,
 sourceLanguage = DW_LANG_Fortran95, file = #di_file, producer = "flang",
 isOptimized = false, emissionKind = LineTablesOnly>
#di_subroutine_type = #llvm.di_subroutine_type<
  callingConvention = DW_CC_program, types = #di_null_type>
#sp = #llvm.di_subprogram<id = distinct[1]<>,
  compileUnit = #di_compile_unit, scope = #di_file, name = "test",
  file = #di_file, subprogramFlags = "Definition",
  type = #di_subroutine_type>
#sp1 = #llvm.di_subprogram<compileUnit = #di_compile_unit,
  name = "target", file = #di_file, subprogramFlags = "Definition",
  type = #di_subroutine_type>
#var1 = #llvm.di_local_variable<scope = #sp1, name = "x", file = #di_file,
 line = 4, type = #di_basic_type>
#var2 = #llvm.di_local_variable<scope = #sp1, name = "y", file = #di_file,
 line = 3, type = #di_basic_type>

module attributes {llvm.target_triple = "amdgcn-amd-amdhsa", omp.is_target_device = true, dlti.dl_spec = #dlti.dl_spec<"dlti.alloca_memory_space" = 5 : ui64>} {
  llvm.func @_QQmain() {
    %0 = llvm.mlir.constant(1 : i64) : i64 loc(#loc1)
    %1 = llvm.alloca %0 x i32 {bindc_name = "y"} : (i64) -> !llvm.ptr<5> loc(#loc1)
    %2 = llvm.addrspacecast %1 : !llvm.ptr<5> to !llvm.ptr loc(#loc1)
    %4 = llvm.alloca %0 x i32 {bindc_name = "x"} : (i64) -> !llvm.ptr<5> loc(#loc1)
    %5 = llvm.addrspacecast %4 : !llvm.ptr<5> to !llvm.ptr loc(#loc1)
    %8 = omp.map.info var_ptr(%5 : !llvm.ptr, i32) map_clauses(tofrom) capture(ByRef) -> !llvm.ptr {name = "x"} loc(#loc1)
    %9 = omp.map.info var_ptr(%2 : !llvm.ptr, i32) map_clauses(tofrom) capture(ByRef) -> !llvm.ptr {name = "y"} loc(#loc1)
    omp.target map_entries(%8 -> %arg0, %9 -> %arg1 : !llvm.ptr, !llvm.ptr) {
      llvm.intr.dbg.declare #var1 = %arg0 : !llvm.ptr loc(#loc2)
      llvm.intr.dbg.declare #var2 = %arg1 : !llvm.ptr loc(#loc2)
      omp.parallel {
        %10 = llvm.load %arg0 : !llvm.ptr -> i32 loc(#loc3)
        %11 = llvm.load %arg1 : !llvm.ptr -> i32 loc(#loc3)
        %12 = llvm.add %10, %11 : i32 loc(#loc3)
        llvm.store %12, %arg0 : i32, !llvm.ptr loc(#loc3)
        omp.terminator loc(#loc3)
      } loc(#loc3)
      omp.terminator loc(#loc2)
    } loc(#loc11)
    llvm.return loc(#loc1)
  } loc(#loc10)
}

#loc1 = loc("target.f90":1:7)
#loc2 = loc("target.f90":3:18)
#loc3 = loc("target.f90":4:18)
#loc10 = loc(fused<#sp>[#loc1])
#loc11 = loc(fused<#sp1>[#loc2])


// CHECK: define internal void @__omp_offloading_{{.*}}_QQmain_l3..omp_par{{.*}} !dbg ![[SP:[0-9]+]] {
// CHECK: #dbg_declare(ptr %{{.*}}, ![[X:[0-9]+]], !DIExpression(DIOpArg(0, ptr), DIOpDeref(ptr)), !{{.*}})
// CHECK: #dbg_declare(ptr %{{.*}}, ![[Y:[0-9]+]], !DIExpression(DIOpArg(0, ptr), DIOpDeref(ptr)), !{{.*}})
// CHECK: }
// CHECK: ![[X]] = !DILocalVariable(name: "x", scope: ![[SP]]{{.*}})
// CHECK: ![[Y]] = !DILocalVariable(name: "y", scope: ![[SP]]{{.*}})

