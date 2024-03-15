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
Array variables inside function can point to a global variable outside. Those globals will be ignored while iterating globalOps.
### Arrays
#### Adjustable 
#### Assumed Shape
#### Assumed Size
#### Assumed Rank
### Strings
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

### Derived Types
TypeInfoOp can be iterated to get the list of all the derived type. It currently lack the location and offsets of the members which will have to beadded either in TypeInfoOp or RecordType/FieldType.

### Association
### Namelists
### CommonBlocks
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
