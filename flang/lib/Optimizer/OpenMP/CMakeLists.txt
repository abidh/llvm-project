get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)

add_flang_library(FlangOpenMPTransforms
  DoConcurrentConversion.cpp
  FunctionFiltering.cpp
  GlobalFiltering.cpp
  MapsForPrivatizedSymbols.cpp
  MapInfoFinalization.cpp
  MarkDeclareTarget.cpp
  LowerWorkshare.cpp

  DEPENDS
  FIRDialect
  HLFIROpsIncGen
  FlangOpenMPPassesIncGen

  LINK_LIBS
  FIRAnalysis
  FIRBuilder
  FIRCodeGen
  FIRDialect
  FIRDialectSupport
  FIRSupport
  FortranCommon
  MLIRFuncDialect
  MLIRMathTransforms
  MLIROpenMPDialect
  HLFIRDialect
  MLIRIR
  MLIRPass
)
