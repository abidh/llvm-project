//===-- DebugTypeGenerator.cpp -- type conversion ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Coding style: https://mlir.llvm.org/getting_started/DeveloperGuide/
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "flang-debug-type-generator"

#include "DebugTypeGenerator.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Support/Debug.h"

namespace fir {

DebugTypeGenerator::DebugTypeGenerator(mlir::ModuleOp m)
    : module(m), kindMapping(getKindMapping(m)) {
  LLVM_DEBUG(llvm::dbgs() << "DITypeAttr generator\n");
}

static mlir::LLVM::DITypeAttr genBasicType(mlir::MLIRContext *context,
                                           mlir::StringAttr name,
                                           unsigned bitSize,
                                           unsigned decoding) {
  return mlir::LLVM::DIBasicTypeAttr::get(
      context, llvm::dwarf::DW_TAG_base_type, name, bitSize, decoding);
}

static mlir::LLVM::DITypeAttr genPlaceholderType(mlir::MLIRContext *context) {
  return genBasicType(context, mlir::StringAttr::get(context, "integer"), 32,
                      llvm::dwarf::DW_ATE_signed);
}

mlir::LLVM::DITypeAttr
DebugTypeGenerator::convertType(mlir::Type Ty, mlir::LLVM::DIFileAttr fileAttr,
                                mlir::LLVM::DIScopeAttr scope,
                                mlir::Location loc) {
  mlir::MLIRContext *context = module.getContext();
  if (Ty.isInteger()) {
    return genBasicType(context, mlir::StringAttr::get(context, "integer"),
                        Ty.getIntOrFloatBitWidth(), llvm::dwarf::DW_ATE_signed);
  } else if (mlir::isa<mlir::FloatType>(Ty)) {
    return genBasicType(context, mlir::StringAttr::get(context, "real"),
                        Ty.getIntOrFloatBitWidth(), llvm::dwarf::DW_ATE_float);
  } else if (auto realTy = mlir::dyn_cast_or_null<fir::RealType>(Ty)) {
    return genBasicType(context, mlir::StringAttr::get(context, "real"),
                        kindMapping.getRealBitsize(realTy.getFKind()),
                        llvm::dwarf::DW_ATE_float);
  } else if (auto logTy = mlir::dyn_cast_or_null<fir::LogicalType>(Ty)) {
    return genBasicType(context,
                        mlir::StringAttr::get(context, logTy.getMnemonic()),
                        kindMapping.getLogicalBitsize(logTy.getFKind()),
                        llvm::dwarf::DW_ATE_boolean);
  } else if (auto boxTy = mlir::dyn_cast_or_null<fir::BoxType>(Ty)) {
    auto elTy = boxTy.getElementType();
    if (auto seqTy = mlir::dyn_cast_or_null<fir::SequenceType>(elTy)) {
      auto elemTy = convertType(seqTy.getEleTy(), fileAttr, scope, loc);
        if (seqTy.hasUnknownShape()) {

        } else {
          auto intTy = mlir::IntegerType::get(context, 64);
          auto one = mlir::IntegerAttr::get(intTy, llvm::APInt(64, 1));
          auto lowerAttr = mlir::LLVM::DISubrangeValueAttr::get(context, one, mlir::LLVM::DIExpressionAttr());
          llvm::SmallVector<mlir::LLVM::DINodeAttr> elements;
          unsigned offset = 32;
          for (auto dim : seqTy.getShape()) {
            llvm::SmallVector<mlir::LLVM::DIExpressionElemAttr> ops;
            ops.push_back(mlir::LLVM::DIExpressionElemAttr::get(context, llvm::dwarf::DW_OP_push_object_address, {}));
            ops.push_back(mlir::LLVM::DIExpressionElemAttr::get(context, llvm::dwarf::DW_OP_plus_uconst, {offset}));
            ops.push_back(mlir::LLVM::DIExpressionElemAttr::get(context, llvm::dwarf::DW_OP_deref, {}));
            offset += 24;
            auto expr = mlir::LLVM::DIExpressionAttr::get(context, ops);
            auto countAttr = mlir::LLVM::DISubrangeValueAttr::get(context, mlir::IntegerAttr(), expr);
            auto subrangeTy = mlir::LLVM::DISubrangeAttr::get(
                  context, nullptr, lowerAttr, countAttr, nullptr);
              elements.push_back(subrangeTy);
          }
            llvm::SmallVector<mlir::LLVM::DIExpressionElemAttr> ops;
            ops.push_back(mlir::LLVM::DIExpressionElemAttr::get(context, llvm::dwarf::DW_OP_push_object_address, {}));
            ops.push_back(mlir::LLVM::DIExpressionElemAttr::get(context, llvm::dwarf::DW_OP_deref, {}));
            auto dataLocation = mlir::LLVM::DIExpressionAttr::get(context, ops);
          // Apart from arrays, the `DICompositeTypeAttr` is used for other
          // things like structure types. Many of its fields which are not
          // applicable to arrays have been set to some valid default values.

          return mlir::LLVM::DICompositeTypeAttr::get(
              context, llvm::dwarf::DW_TAG_array_type, /*recursive id*/ {},
              /* name */ nullptr, /* file */ nullptr, /* line */ 0,
              /* scope */ nullptr, elemTy, mlir::LLVM::DIFlags::Zero,
              /* sizeInBits */ 0,
              /*alignInBits*/ 0, elements, dataLocation,
              nullptr,nullptr,nullptr);
        }
      }
    return genPlaceholderType(context);
  } else {
    // return convertSequenceType(seqTy, fileAttr, scope, loc); else {
    // FIXME: These types are currently unhandled. We are generating a
    // placeholder type to allow us to test supported bits.
    return genPlaceholderType(context);
  }
}

} // namespace fir
