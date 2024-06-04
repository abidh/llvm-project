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
#include "flang/Optimizer/Support/DataLayout.h"
#include "mlir/Pass/Pass.h"
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

mlir::LLVM::DITypeAttr DebugTypeGenerator::convertBoxedSequenceType(
    fir::SequenceType seqTy, mlir::LLVM::DIFileAttr fileAttr,
    mlir::LLVM::DIScopeAttr scope, mlir::Location loc, bool genAlloc,
    bool genAssoc) {

  mlir::MLIRContext *context = module.getContext();
  // FIXME: Assumed rank arrays not supported yet
  if(seqTy.hasUnknownShape())
    return genPlaceholderType(context);

  llvm::SmallVector<mlir::LLVM::DIExpressionElemAttr> ops;
  auto addOp = [&](unsigned opc, llvm::ArrayRef<uint64_t> vals) {
    ops.push_back(mlir::LLVM::DIExpressionElemAttr::get(
        context, opc, vals));
  };
  auto createExpr = [&]() {
    return mlir::LLVM::DIExpressionAttr::get(context, ops);
  };
  addOp(llvm::dwarf::DW_OP_push_object_address, {});
  addOp(llvm::dwarf::DW_OP_deref, {});
  mlir::LLVM::DIExpressionAttr dataLocation = createExpr();
  addOp(llvm::dwarf::DW_OP_lit0, {});
  addOp(llvm::dwarf::DW_OP_ne, {});
  mlir::LLVM::DIExpressionAttr associated = genAssoc ? createExpr() : nullptr;
  mlir::LLVM::DIExpressionAttr allocated = genAlloc ? createExpr() : nullptr;
  ops.clear();

  llvm::SmallVector<mlir::LLVM::DINodeAttr> elements;
  auto intTy = mlir::IntegerType::get(context, 64);
  auto lowerAttr = mlir::IntegerAttr::get(intTy, llvm::APInt(64, 1));
  mlir::LLVM::DITypeAttr elemTy =
      convertType(seqTy.getEleTy(), fileAttr, scope, loc);
  unsigned offset = 32;
  for (auto dim : seqTy.getShape()) {
    addOp(llvm::dwarf::DW_OP_push_object_address, {});
    addOp(llvm::dwarf::DW_OP_plus_uconst, {offset});
    addOp(llvm::dwarf::DW_OP_deref, {});
    mlir::LLVM::DIExpressionAttr countAttr = createExpr();
    ops.clear();
    offset += 24;
    mlir::LLVM::DISubrangeAttr subrangeTy = mlir::LLVM::DISubrangeAttr::get(
        context, nullptr, lowerAttr, countAttr, nullptr);
    elements.push_back(subrangeTy);
  }

  // Apart from arrays, the `DICompositeTypeAttr` is used for other
  // things like structure types. Many of its fields which are not
  // applicable to arrays have been set to some valid default values.

    return mlir::LLVM::DICompositeTypeAttr::get(
        context, llvm::dwarf::DW_TAG_array_type, /*recursive id*/ {},
        /* name */ nullptr, /* file */ nullptr, /* line */ 0,
        /* scope */ nullptr, elemTy, mlir::LLVM::DIFlags::Zero,
        /* sizeInBits */ 0, /*alignInBits*/ 0,
        elements, dataLocation, nullptr, allocated, associated);
}

mlir::LLVM::DITypeAttr DebugTypeGenerator::convertSequenceType(
    fir::SequenceType seqTy, mlir::LLVM::DIFileAttr fileAttr,
    mlir::LLVM::DIScopeAttr scope, mlir::Location loc) {
  mlir::MLIRContext *context = module.getContext();
  if (seqTy.hasDynamicExtents())
    return genPlaceholderType(context);

  llvm::SmallVector<mlir::LLVM::DINodeAttr> elements;
  auto intTy = mlir::IntegerType::get(context, 64);
  auto lowerAttr = mlir::IntegerAttr::get(intTy, llvm::APInt(64, 1));
  mlir::LLVM::DITypeAttr elemTy =
      convertType(seqTy.getEleTy(), fileAttr, scope, loc);
  // FIXME: Only fixed sizes arrays handled at the moment.

  for (fir::SequenceType::Extent dim : seqTy.getShape()) {
    // FIXME: Only supporting lower bound of 1 at the moment. The
    // 'SequenceType' has information about the shape but not the shift. In
    // cases where the conversion originated during the processing of
    // 'DeclareOp', it may be possible to pass on this information. But the
    // type conversion should ideally be based on what information present in
    // the type class so that it works from everywhere (e.g. when it is part
    // of a module or a derived type.)
    auto countAttr = mlir::IntegerAttr::get(intTy, llvm::APInt(64, dim));
    auto subrangeTy = mlir::LLVM::DISubrangeAttr::get(
        context, countAttr, lowerAttr, nullptr, nullptr);
    elements.push_back(subrangeTy);
  }
  // Apart from arrays, the `DICompositeTypeAttr` is used for other things like
  // structure types. Many of its fields which are not applicable to arrays
  // have been set to some valid default values.

  return mlir::LLVM::DICompositeTypeAttr::get(
      context, llvm::dwarf::DW_TAG_array_type, /*recursive id*/ {},
      /* name */ nullptr, /* file */ nullptr, /* line */ 0, /* scope */ nullptr,
      elemTy, mlir::LLVM::DIFlags::Zero, /* sizeInBits */ 0,
      /*alignInBits*/ 0, elements, /* dataLocation */ nullptr,
      /* rank */ nullptr, /* allocated */ nullptr,
      /* associated */ nullptr);
}

mlir::LLVM::DITypeAttr DebugTypeGenerator::convertCharacterType(
    fir::CharacterType charTy, mlir::LLVM::DIFileAttr fileAttr,
    mlir::LLVM::DIScopeAttr scope, mlir::Location loc) {
  mlir::MLIRContext *context = module.getContext();
  if (charTy.hasConstantLen()) {
    std::string name(charTy.getMnemonic());
    name += "(" + std::to_string(charTy.hasConstantLen()) + ")";
    return mlir::LLVM::DIStringTypeAttr::get(
        context, llvm::dwarf::DW_TAG_string_type,
        mlir::StringAttr::get(context, name),
        charTy.getLen() * 8, 0, nullptr, nullptr, nullptr,
        llvm::dwarf::DW_ATE_unsigned);
  } else {
  llvm::SmallVector<mlir::LLVM::DIExpressionElemAttr> ops;
    auto addOp = [&](unsigned opc, llvm::ArrayRef<uint64_t> vals) {
      ops.push_back(mlir::LLVM::DIExpressionElemAttr::get(
          context, opc, vals));
    };
    auto createExpr = [&]() {
      return mlir::LLVM::DIExpressionAttr::get(context, ops);
    };
  
    addOp(llvm::dwarf::DW_OP_push_object_address, {});
    addOp(llvm::dwarf::DW_OP_plus_uconst, {8});
    mlir::LLVM::DIExpressionAttr length = createExpr();
    ops.clear();
    addOp(llvm::dwarf::DW_OP_push_object_address, {});
    addOp(llvm::dwarf::DW_OP_deref, {});
    mlir::LLVM::DIExpressionAttr location = createExpr();
    std::string name(charTy.getMnemonic());
    name += "(*)";
    return mlir::LLVM::DIStringTypeAttr::get(
        context, llvm::dwarf::DW_TAG_string_type,
        mlir::StringAttr::get(context, name), 0, 0, nullptr,
        length, location, llvm::dwarf::DW_ATE_unsigned);
  }
}

mlir::LLVM::DITypeAttr DebugTypeGenerator::convertPointerLikeType(
    mlir::Type elTy, mlir::LLVM::DIFileAttr fileAttr,
    mlir::LLVM::DIScopeAttr scope, mlir::Location loc, bool genAlloc,
    bool genAssoc) {
  // The types which use DIStringType or DICompositeType have builtin
  // mechanism to get the location of the data (using dataLocation field or
  // stringLocationExp field.). For other types, we will use pointer style
  // mechanism.
  mlir::MLIRContext *context = module.getContext();
  if (auto seqTy = mlir::dyn_cast_or_null<fir::SequenceType>(elTy))
    return convertBoxedSequenceType(seqTy, fileAttr, scope, loc, genAlloc, genAssoc);
  if (auto charTy = mlir::dyn_cast_or_null<fir::CharacterType>(elTy))
    return convertCharacterType(charTy, fileAttr, scope, loc);
  mlir::LLVM::DITypeAttr elTyAttr = convertType(elTy, fileAttr, scope, loc);
  std::optional<mlir::DataLayout> dl =
      fir::support::getOrSetDataLayout(module, /*allowDefaultLayout=*/true);
  if (!dl) {
    mlir::emitError(module.getLoc(),
                    "module operation must carry a data layout attribute "
                    "to generate llvm IR from FIR");
    return genPlaceholderType(context);
  }
  uint64_t ptrSize = dl->getTypeSizeInBits(mlir::LLVM::LLVMPointerType::get(context));
  return mlir::LLVM::DIDerivedTypeAttr::get(
      context, llvm::dwarf::DW_TAG_pointer_type,
      mlir::StringAttr::get(context, ""), elTyAttr, ptrSize,
      /*alignInBits*/ 0, /* offset */ 0, std::nullopt, nullptr);
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
  } else if (fir::isa_complex(Ty)) {
    unsigned bitWidth;
    if (auto cplxTy = mlir::dyn_cast_or_null<mlir::ComplexType>(Ty)) {
      auto floatTy = mlir::cast<mlir::FloatType>(cplxTy.getElementType());
      bitWidth = floatTy.getWidth();
    } else if (auto cplxTy = mlir::dyn_cast_or_null<fir::ComplexType>(Ty)) {
      bitWidth = kindMapping.getRealBitsize(cplxTy.getFKind());
    } else {
      llvm_unreachable("Unhandled complex type");
    }
    return genBasicType(context, mlir::StringAttr::get(context, "complex"),
                        bitWidth * 2, llvm::dwarf::DW_ATE_complex_float);
  } else if (auto seqTy = mlir::dyn_cast_or_null<fir::SequenceType>(Ty)) {
    return convertSequenceType(seqTy, fileAttr, scope, loc);
  }  else if (auto charTy = mlir::dyn_cast_or_null<fir::CharacterType>(Ty)) {
    return convertCharacterType(charTy, fileAttr, scope, loc);
  } else if (auto boxTy = mlir::dyn_cast_or_null<fir::BoxType>(Ty)) {
    auto elTy = boxTy.getElementType();
    if (auto heapTy = mlir::dyn_cast_or_null<fir::HeapType>(elTy)) {
      elTy = heapTy.getElementType();
      return convertPointerLikeType(elTy, fileAttr, scope, loc, true, false);
    }
    if (auto seqTy = mlir::dyn_cast_or_null<fir::SequenceType>(elTy))
      return convertBoxedSequenceType(seqTy, fileAttr, scope, loc, false, false);
    if (auto ptrTy = mlir::dyn_cast_or_null<fir::PointerType>(elTy)) {
      elTy = ptrTy.getElementType();
      return convertPointerLikeType(elTy, fileAttr, scope, loc, false, true);
    }
    return convertType(elTy, fileAttr, scope, loc);
  } else {
    // FIXME: These types are currently unhandled. We are generating a
    // placeholder type to allow us to test supported bits.
    return genPlaceholderType(context);
  }
}

} // namespace fir
