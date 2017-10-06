/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/SourceGen.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/IncludeCollector.h"

namespace decode {

//TODO: refact

SourceGen::SourceGen(TypeReprGen* reprGen, SrcBuilder* output)
    : _output(output)
    , _typeReprGen(reprGen)
    , _inlineSer(reprGen, output)
    , _inlineDeser(reprGen, output)
    , _prototypeGen(reprGen, output)
{
}

SourceGen::~SourceGen()
{
}

void SourceGen::appendIncludes(bmcl::StringView modName)
{
    StringBuilder path(modName.toStdString());
    path.append('/');
    path.append(_fileName);
    _output->appendLocalIncludePath(path.view());
    _output->appendLocalIncludePath("core/Try");
    _output->appendLocalIncludePath("core/Logging");
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"");
    _output->append(path.result());
    _output->append(".gen.c\"\n");
}

void SourceGen::appendEnumSerializer(const EnumType* type)
{
    _output->append("    switch(self) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case ");
        _output->append("Photon");
        _output->append(_name);
        _output->append("_");
        _output->append(c->name());
        _output->append(":\n");
    }
    _output->append("        break;\n");
    _output->append("    default:\n");
    _output->append("        PHOTON_CRITICAL(\"Failed to serialize enum\");\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n    ");
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVarint(dest, (int64_t)self)");
    }, "Failed to write enum");
}

void SourceGen::appendEnumDeserializer(const EnumType* type)
{
    _output->appendIndent(1);
    _output->appendVarDecl("int64_t", "value");
    _output->appendIndent(1);
    _output->append("Photon");
    _output->appendVarDecl(_name, "result");
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    }, "Failed to read enum");

    _output->append("    switch(value) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case ");
        _output->appendNumericValue(c->value());
        _output->append(":\n");
        _output->append("        result = ");
        _output->append("Photon");
        _output->append(_name);
        _output->append("_");
        _output->append(c->name());
        _output->append(";\n");
        _output->append("        break;\n");
    }
    _output->append("    default:\n");
    _output->append("        PHOTON_WARNING(\"Failed to deserialize enum\");\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n");

    _output->append("    *self = result;\n");
}

static bmcl::Option<std::size_t> typeFixedSize(const Type* type)
{
    return type->fixedSize();
}

class InlineStructInspector : public InlineFieldInspector<InlineStructInspector> {
public:
    InlineStructInspector(SrcBuilder* dest)
        : InlineFieldInspector<InlineStructInspector>(dest)
        , _argName("self->")
    {
    }

    void beginField(const Field* field)
    {
        _argName.append(field->name().begin(), field->name().end());
    }

    void endField(const Field*)
    {
        _argName.resize(6);
    }

    bmcl::StringView currentFieldName() const
    {
        return _argName.view();
    }

private:
    StringBuilder _argName;
};

void SourceGen::appendStructSerializer(const StructType* type)
{
    InlineStructInspector inspector(_output);
    inspector.inspect(type->fieldsRange(), &_inlineSer);
}

void SourceGen::appendStructDeserializer(const StructType* type)
{
    InlineStructInspector inspector(_output);
    inspector.inspect(type->fieldsRange(), &_inlineDeser);
}

void SourceGen::appendVariantSerializer(const VariantType* type)
{
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVarint(dest, (int64_t)self->type)");
    }, "Failed to write variant type");

    _output->append("    switch(self->type) {\n");
    StringBuilder argName("self->data.");
    for (const VariantField* field : type->fieldsRange()) {
        _output->append("    case ");
        _output->append("Photon");
        _output->append(_name);
        _output->append("Type");
        _output->append("_");
        _output->appendWithFirstUpper(field->name());
        _output->append(": {\n");

        InlineSerContext ctx(2);
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple: {
            const TupleVariantField* tupField = static_cast<const TupleVariantField*>(field);
            std::size_t j = 1;
            for (const Type* t : tupField->typesRange()) {
                argName.appendWithFirstLower(field->name());
                argName.append(_name);
                argName.append("._");
                argName.appendNumericValue(j);
                _inlineSer.inspect(t, ctx, argName.view());
                argName.resize(11);
                j++;
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<const StructVariantField*>(field);
            for (const Field* f : varField->fieldsRange()) {
                argName.appendWithFirstLower(field->name());
                argName.append(_name);
                argName.append(".");
                argName.append(f->name());
                _inlineSer.inspect(f->type(), ctx, argName.view());
                argName.resize(11);
            }
            break;
        }
        }

        _output->append("        break;\n");
        _output->append("    }\n");
    }
    _output->append("    default:\n");
    _output->append("        PHOTON_CRITICAL(\"Failed to serialize variant\");\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n");
}

void SourceGen::appendVariantDeserializer(const VariantType* type)
{
    _output->appendIndent(1);
    _output->appendVarDecl("int64_t", "value");
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    }, "Failed to read variant type");

    _output->append("    switch(value) {\n");
    std::size_t i = 0;
    StringBuilder argName("self->data.");
    for (const VariantField* field : type->fieldsRange()) {
        _output->append("    case ");
        _output->appendNumericValue(i);
        i++;
        _output->append(": {\n");
        _output->append("        self->type = ");
        _output->append("Photon");
        _output->append(_name);
        _output->append("Type");
        _output->append("_");
        _output->appendWithFirstUpper(field->name());
        _output->append(";\n");

        InlineSerContext ctx(2);
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple: {
            const TupleVariantField* tupField = static_cast<const TupleVariantField*>(field);
            std::size_t j = 1;
            for (const Type* t : tupField->typesRange()) {
                    argName.appendWithFirstLower(field->name());
                    argName.append(_name);
                    argName.append("._");
                    argName.appendNumericValue(j);
                    _inlineDeser.inspect(t, ctx, argName.view());
                    argName.resize(11);
                    j++;
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<const StructVariantField*>(field);
            for (const Field* f : varField->fieldsRange()) {
                argName.appendWithFirstLower(field->name());
                argName.append(_name);
                argName.append(".");
                argName.append(f->name());
                _inlineDeser.inspect(f->type(), ctx, argName.view());
                argName.resize(11);
            }
            break;
        }
        }

        _output->append("        break;\n");
        _output->append("    }\n");
    }
    _output->append("    default:\n");
    _output->append("        PHOTON_WARNING(\"Failed to deserialize variant\");\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n");
}

void SourceGen::appendDynArraySerializer(const DynArrayType* type)
{
    InlineSerContext ctx;
    _output->append("    if (self->size > ");
    _output->appendNumericValue(type->maxSize());
    _output->append(") {\n        PHOTON_CRITICAL(\"Failed to serialize dynarray\");\n");
    _output->append("        return PhotonError_InvalidValue;\n    }\n");
    _output->append("    ");
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVaruint(dest, self->size)");
    }, "Failed to write dynarray size");
    auto size = typeFixedSize(type->elementType());
    if (size.isSome()) {
        _inlineSer.appendSizeCheck(ctx, "self->size * " + std::to_string(size.unwrap()), _output);
    }
    _output->appendLoopHeader(ctx, "self->size");
    InlineSerContext lctx = ctx.indent();
    _inlineSer.inspect(type->elementType(), lctx, "self->data[a]", size.isNone());
    _output->append("    }\n");
}

void SourceGen::appendDynArrayDeserializer(const DynArrayType* type)
{
    InlineSerContext ctx;
    _output->appendIndent(1);
    _output->appendVarDecl("uint64_t", "size");
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVaruint(src, &size)");
    }, "Failed to read dynarray size");
    _output->append("    if (size > ");
    _output->appendNumericValue(type->maxSize());
    _output->append(") {\n        PHOTON_WARNING(\"Failed to deserialize dynarray\");\n");
    _output->append("        return PhotonError_InvalidValue;\n    }\n");
    auto size = typeFixedSize(type->elementType());
    if (size.isSome()) {
        _inlineDeser.appendSizeCheck(ctx, "size * " + std::to_string(size.unwrap()), _output);
    }
    _output->appendLoopHeader(ctx, "size");
    InlineSerContext lctx = ctx.indent();
    _inlineDeser.inspect(type->elementType(), lctx, "self->data[a]", size.isNone());
    _output->append("    }\n");
    if (type->elementType()->isBuiltinChar()) {
        _output->append("    self->data[size] = '\\0';\n");
    }
    _output->append("    self->size = size;\n");
}

template <typename T, typename F>
void SourceGen::genSource(const T* type, F&& serGen, F&& deserGen)
{
    _output->appendEol();
    _prototypeGen.appendSerializerFuncDecl(_baseType);
    _output->append("\n{\n");
    (this->*serGen)(type);
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
    _output->appendEol();
    _prototypeGen.appendDeserializerFuncDecl(_baseType);
    _output->append("\n{\n");
    (this->*deserGen)(type);
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n\n");
    _output->append("#undef _PHOTON_FNAME");
    _output->appendEol();
}

bool SourceGen::visitDynArrayType(const DynArrayType* type)
{
    StringBuilder path("_dynarray_/");
    path.append(TypeNameGen::genTypeNameAsString(type));
    _output->appendLocalIncludePath(path.view());
    _output->appendLocalIncludePath("core/Try");
    _output->appendLocalIncludePath("core/Logging");
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"");
    _output->append(path.result());
    _output->append(".gen.c\"\n\n");
    _prototypeGen.appendSerializerFuncDecl(type);
    _output->append("\n{\n");
    appendDynArraySerializer(type);
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
    _output->appendEol();
    _prototypeGen.appendDeserializerFuncDecl(type);
    _output->append("\n{\n");
    appendDynArrayDeserializer(type);
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n\n");
    _output->append("#undef _PHOTON_FNAME");
    _output->appendEol();
    return false;
}

bool SourceGen::visitEnumType(const EnumType* type)
{
    genSource(type, &SourceGen::appendEnumSerializer, &SourceGen::appendEnumDeserializer);
    return false;
}

bool SourceGen::visitStructType(const StructType* type)
{
    genSource(type, &SourceGen::appendStructSerializer, &SourceGen::appendStructDeserializer);
    return false;
}

bool SourceGen::visitVariantType(const VariantType* type)
{
    genSource(type, &SourceGen::appendVariantSerializer, &SourceGen::appendVariantDeserializer);
    return false;
}

void SourceGen::genTypeSource(const NamedType* type, bmcl::StringView name)
{
    _name = name;
    _fileName = type->name();
    _baseType = type;
    genSource(type, type->moduleName());
}

void SourceGen::genTypeSource(const GenericInstantiationType* instantiation, bmcl::StringView name)
{
    _name = name;
    _fileName = name;
    _baseType = instantiation;
    const Type* type = instantiation->instantiatedType()->resolveFinalType();
    genSource(type, "_generic_");

}

void SourceGen::genSource(const Type* type, bmcl::StringView modName)
{
    switch (type->typeKind()) {
        case TypeKind::Variant:
            appendIncludes(modName);
            visitVariantType(type->asVariant());
            return;
        case TypeKind::Struct:
            appendIncludes(modName);
            visitStructType(type->asStruct());
            return;
        case TypeKind::Enum:
            appendIncludes(modName);
            visitEnumType(type->asEnum());
            return;
        default:
            return;
    }
}

void SourceGen::genTypeSource(const DynArrayType* type)
{
    _baseType = type;
    visitDynArrayType(type);
}
}
