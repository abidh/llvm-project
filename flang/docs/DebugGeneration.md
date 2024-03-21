# Debug Generation

Application developers spend a significant time debugging the applications that
they create. Hence it is important that a compiler provide support for a good
debug experience. DWARF[1] is the standard debugging file format used by
compilers and debuggers. The LLVM infrastructure supports debug info generation
using metadata[2]. Recently, support for generating debug metadata was added
to MLIR by way of MLIR attributes. Flang can leverage these MLIR attributes to
generate good debug information.

We can break the work for debug generation into two separate tasks:
1) Line Table generation
2) Full debug generation
This RFC only deals with the first task of Line Table generation. Full Debug
generation is left as TODO. The support for Fortran Debug in LLVM
infrastructure[3] has made great progress due to many Fortran frontends adopting
LLVM as the backend as well as the availability of the Classic Flang compiler.


## Line Table Generation

While the recent MLIR change provides a way for frontends to generate debug,
it disabled the default generation of line table information[4]. Line number
information is useful for setting breakpoints in the debugger and also
for generating stacktrace, performance profiles and optimization reports
from the LLVM Optimizer. Hence, it is important that we enable support for it.

In MLIR, every operation has a source location. Presence of location information
on all operations makes available the information needed  to generate the
linetable information. Currently, LLVM IR debug metadata is generated from
MLIR for a function only if there is a `FusedLoc` with a `SubprogramAttr`[5]
on  it. A `FusedLoc` is an MLIR Location fused with a metadata. This requires
that all Function operations in FIR/MLIR have the `FusedLoc`.

I propose that we add a pass that walks over all the Functions and add a 
`FusedLoc` with `SubprogramAttr`. `SubprogramAttr` requires a few other
attributes like `CompileUnitAttr`, `FileAttr`, `SubroutineTypeAttr` etc.
The basic information that is required for creating these attributes are
the following:
- Location of the source file (name and directory). This information can be
passed by the driver.
- Details of the compiler (name and version and git hash). This information
can be passed by the driver.
- Language Standard. We can set it to Fortran95 for now and periodically
revise it when full support for later standards is available.
- Optimisation Level. This information can be passed by the driver.
- Type of debug generated (linetable/full debug). This information can be
passed by the driver based on the flags used.
- Name of the function. Available on the operation itself.
- Calling Convention: `DW_CC_normal` by default and `DW_CC_program` if it is
the main program.
- The source level types (arguments and return) of the subroutine/function.

The pass is prototyped in https://reviews.llvm.org/D137956. Alternative
implementation choice includes adding the `FusedLoc` info during lowering to
FIR or alternatively during the FIRToLLVMConversion pass. The former will
miss any outlined functions that are created by FIR transformation passes.
The latter only operates at the Operation level and hence might not be
suitable for creating Module level information like `CompileUnitAttr`. Also,
the presence of the relevant Module level attribute and `SubroutineAttr`
can be used for generating further debug info during the conversion to LLVM.

###Â Driver Flags
By default, Flang will not generate any debug or linetable information.
Linetable information will be generated if the following flags are present.
-gline-tables-only, -g1 : Emit debug line number tables only

## Full Debug Generation
TODO.
### Variables

Local Variables

  %3 = fir.alloca i32
  %4:2 = hlfir.declare %3 {uniq_name = "_QFEi"}


  #di_local_variable = #llvm.di_local_variable<name = "i", line = 20, type = #di_basic_type>

  %1 = llvm.alloca %0 x i64
  llvm.intr.dbg.declare #di_local_variable = %1
  
We need to create DILocalVariableAttr which will have all the information about the variable like its type and source
location. This happens in AddDebugFoundationPass by processing the DeclareOp. Next we need to connect this metadata
to a value. This can only happen at the time when we convert to llvm-ir dialect as mlir::LLVM::DbgDeclareOp requires
llvm.ptr type. It mean that we need to carry the DILocalVariableAttr somehow till we can use it.

  auto localVarAttr = mlir::LLVM::DILocalVariableAttr::get(...);
  declOp.getMemref().getDefiningOp()->setAttr("debug", localVarAttr);


Array variables inside function can point to a global variable outside. Those globals will be ignored while iterating globalOps.
### Arrays

The fixed size arrray in the FIR will be represented by DICompositeTypeAttr with a tag value of DW_TAG_array_type.
Array bounds in each dimension will be represented by DISubrangeAttr. 

DICompositeTypeAttr(dwarf::DW_TAG_array_type)
  list of DISubrangeAttr(list of IntegerAttr for array bounds in that dimensions)
end

DISubrangeAttr in mlir takes IntegerAttr at the moment so only works with fixed sizes arrays. It will need to change to support
assumed size/rank arrays.


Example: TODO

#### Adjustable 

The bounds information in DISubrangeAttr will point to an artificial variable which will hold the runtime
value of the expression used in the array declaration. I am not sure how 

Example; TODO
#### Assumed Shape
The assumed shape array will use the similar representation as fixed size array but there will be 2 differences.

1. There will be a DataLocation field whic will be an expression. This will enable debugger to get the data
pointer from array descriptor. This field is not currently available in DICompositeTypeAttr and may need to be added.

2. The field in DISubrangeAttr for array bounds will be expression which will allow the debugger to get the array bounds
from descriptor. 

Example: TODO

#### Assumed Size

It is treated as raw arrays. Debug information will not provide any bounds information.

#### Assumed Rank

This is currently unsupported in flang. Its representation will be similar to array representation for asumed shape array with the
following difference.

1. DICompositeTypeAttr will have a rank field which will be an expression. It will be used to get the rank value from descriptor.
2. A new DIGenericSubrangeAttr will be added which will allow to debuggers to calculate bounds in any dimension.

Example: TODO

### Pointers and Allocatables
The obvious implementation will be to treat them as pointer to a type. Thats how classic flang and gfortran seems to handle them in debug info.

integer, pointer :: pt
(GDB) ptype pt
type = PTR TO -> ( integer(kind=4) )

But for arrays, the type of the allocatable or pointer variable is the array and not pointer to the array.

integer, pointer, dimension (:,:) :: pa
integer, target :: array(4,5)
pa => array
(gdb) ptype pa
type = integer(kind=4) (4,5)

The proposal is to keep this behavior in flang.

Debug metadata will also be generated to enable debuggers to find the allocated/associated status of these variables.

### Strings

Fixed sized string will be treated like fixed sizes arrays in the debug info. The allocatabe string will be treated like
like allocatable array with metadata that will enable debuggers to calculate its size.

  character(len=:), allocatable :: var
  character(len=20) :: fixed

!1 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_unsigned_char)
!2 = !DIExpression(DW_OP_push_object_address, DW_OP_plus_uconst, 8, DW_OP_deref)
!3 = !DIExpression(DW_OP_push_object_address, DW_OP_deref)
!4 = !DICompositeType(tag: DW_TAG_array_type, baseType: !1, elements: !5, dataLocation: !3)
!5 = !DISubrange(count: !2, lowerBound: 1)
!6 = !DILocalVariable(name: "var", type: !4)

!7 = !DILocalVariable(name: "fixed", type: !8)
!8 = !DICompositeType(tag: DW_TAG_array_type, baseType: !1, size: 160, elements: !9)
!9 = !DISubrange(count: 20, lowerBound: 1)

### Derived Types
TypeInfoOp can be iterated to get the list of all the derived type. It currently lack the location and offsets of the members which will have to beadded either in TypeInfoOp or RecordType/FieldType.

### Association

They will be treated like normal variables. Although we may require to handle the case where 
the DeclareOp of one variable points to the DeclareOp of another variable (e.g. a => b).

### Namelists

I dont see a way to extract namelist information at the FIR level. 

### CommonBlocks

integer a, b
common /test/ a, b

fir.global common @test_(dense<0> : vector<8xi8>) : !fir.array<8xi8> loc(#loc1)

  %0 = fir.address_of(@test_) : !fir.ref<!fir.array<8xi8>> loc(#loc3)
  %1 = fir.convert %0 : (!fir.ref<!fir.array<8xi8>>) -> !fir.ref<!fir.array<?xi8>> loc(#loc3)
  %c0 = arith.constant 0 : index loc(#loc3)
  %2 = fir.coordinate_of %1, %c0 : (!fir.ref<!fir.array<?xi8>>, index) -> !fir.ref<i8> loc(#loc3)
  %3 = fir.convert %2 : (!fir.ref<i8>) -> !fir.ref<i32> loc(#loc3)
  %4:2 = hlfir.declare %3 {uniq_name = "_QMhelperFmatrixEa"} : (!fir.ref<i32>) -> (!fir.ref<i32>, !fir.ref<i32>) loc(#loc3)


A common block generates a GlobalOp that has the storage and then every function where the common block is used has
DeclareOp for the variable which will point to the global storage through CoordinateOp and AddrOfOp. The debug info will
have a DICommonBlock in every function where the common block is used. This DICommonBlock will have the variables which will
point to global storage. Note that currently MLIR does not have any attribute corresponding to DICommonBlock so it will have
to be added.

;@test_ = common global [8 x i8] zeroinitializer, !dbg !5, !dbg !6
!1 = !DISubprogram()
!2 = !DICommonBlock(scope: !1, name: "test")
!3 = !DIGlobalVariable(scope: !2, name: "a")
!4 = !DIExpression()
!5 = !DIGlobalVariableExpression(var: !3, expr: !4)
!6 = !DIGlobalVariable(scope: !2, name: "b")
!7 = !DIExpression(DW_OP_plus_uconst, 4)
!8 = !DIGlobalVariableExpression(var: !6, expr: !7)

### Modules

# Testing

- LLVM LIT tests will be added to test:
  - the driver and ensure that it passes the debug linetable generation info appropriately.
  - that the pass works as expected and generates linetable info. Test will be with `fir-opt`.
  - with `flang -fc1` that end-to-end linetable generation works. 
- Manual external tests will be written to ensure that the following works in debug tools
  - Break at lines.
  - Break at functions.
  - print type (ptype) of function names.

# Resources
- [1] https://dwarfstd.org/doc/DWARF5.pdf
- [2] https://llvm.org/docs/LangRef.html#metadata
- [3] https://archive.fosdem.org/2022/schedule/event/llvm_fortran_debug/
- [4] https://github.com/llvm/llvm-project/issues/58634
- [5] https://github.com/llvm/llvm-project/blob/main/mlir/lib/Target/LLVMIR/DebugTranslation.cpp
