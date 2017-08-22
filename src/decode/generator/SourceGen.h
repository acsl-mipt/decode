/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/ast/AstVisitor.h"
#include "decode/generator/FuncPrototypeGen.h"
#include "decode/generator/InlineTypeDeserializerGen.h"
#include "decode/generator/InlineTypeSerializerGen.h"

#include <string>

namespace decode {

class SrcBuilder;
class TypeReprGen;

class SourceGen : public ConstAstVisitor<SourceGen> {
public:
    SourceGen(TypeReprGen* reprGen, SrcBuilder* output);
    ~SourceGen();

    void genTypeSource(const Type* type);

    bool visitBuiltinType(const BuiltinType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitDynArrayType(const DynArrayType* type);
    bool visitFunctionType(const FunctionType* type);
    bool visitEnumType(const EnumType* type);
    bool visitStructType(const StructType* type);
    bool visitVariantType(const VariantType* type);
    bool visitImportedType(const ImportedType* type);
    bool visitAliasType(const AliasType* type);

private:
    template <typename T, typename F>
    void genSource(const T* type, F&& serGen, F&& deserGen);

    void appendEnumSerializer(const EnumType* type);
    void appendEnumDeserializer(const EnumType* type);
    void appendStructSerializer(const StructType* type);
    void appendStructDeserializer(const StructType* type);
    void appendVariantSerializer(const VariantType* type);
    void appendVariantDeserializer(const VariantType* type);
    void appendDynArraySerializer(const DynArrayType* type);
    void appendDynArrayDeserializer(const DynArrayType* type);

    void appendIncludes(const NamedType* type);

    SrcBuilder* _output;
    Rc<TypeReprGen> _typeReprGen;
    InlineTypeSerializerGen _inlineSer;
    InlineTypeDeserializerGen _inlineDeser;
    FuncPrototypeGen _prototypeGen;
};

}
