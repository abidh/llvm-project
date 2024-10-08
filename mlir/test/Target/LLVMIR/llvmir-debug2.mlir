// RUN: mlir-translate -mlir-to-llvmir --write-experimental-debuginfo=false --split-input-file %s | FileCheck %s

#bt = #llvm.di_basic_type<tag = DW_TAG_base_type, name = "int", sizeInBits = 32>
#file = #llvm.di_file<"test.f90" in "">
#cu = #llvm.di_compile_unit<id = distinct[0]<>, sourceLanguage = DW_LANG_C,
 file = #file, isOptimized = false, emissionKind = Full>
#sp = #llvm.di_subprogram<compileUnit = #cu, scope = #file, name = "test",
 file = #file, subprogramFlags = Definition>
#di_common_block = #llvm.di_common_block<scope = #sp, name = "block", file = #file, line = 3>
#global_var = #llvm.di_global_variable<scope = #di_common_block, name = "a",
 linkageName = "a", file = #file, line = 2, type = #bt, isLocalToUnit = true, isDefined = true>
#var_expression = #llvm.di_global_variable_expression<var = #global_var, expr = <>>

llvm.mlir.global common @block_(dense<0> : tensor<8xi8>) {addr_space = 0 : i32, alignment = 4 : i64, dbg_expr = [#var_expression]} : !llvm.array<8 x i8>

llvm.func @_QQmain() {
  llvm.return
} loc(#loc2)

#loc1 = loc("common3.f90":1:0)
#loc2 = loc(fused<#sp>[#loc1])

// CHECK: !DICommonBlock(scope: ![[SCOPE:[0-9]+]], declaration: null, name: "block", file: ![[FILE:[0-9]+]], line: 3)
// CHECK: ![[SCOPE]] = {{.*}}!DISubprogram(name: "test"{{.*}})
// CHECK: ![[FILE]] = !DIFile(filename: "test.f90"{{.*}})