/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/TypeReprGen.h"
#include "decode/core/Foreach.h"
#include "decode/generator/TypeNameGen.h"

#include <bmcl/Logging.h>

namespace decode {

TypeReprGen::TypeReprGen(SrcBuilder* dest)
    : _output(dest)
{
}

TypeReprGen::~TypeReprGen()
{
}

static bmcl::StringView builtinToC(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        return "size_t";
    case BuiltinTypeKind::ISize:
        return "ptrdiff_t";
    case BuiltinTypeKind::Varuint:
        return "uint64_t";
    case BuiltinTypeKind::Varint:
        return "int64_t";
    case BuiltinTypeKind::U8:
        return "uint8_t";
    case BuiltinTypeKind::I8:
        return "int8_t";
    case BuiltinTypeKind::U16:
        return "uint16_t";
    case BuiltinTypeKind::I16:
        return "int16_t";
    case BuiltinTypeKind::U32:
        return "uint32_t";
    case BuiltinTypeKind::I32:
        return "int32_t";
    case BuiltinTypeKind::U64:
        return "uint64_t";
    case BuiltinTypeKind::I64:
        return "int64_t";
    case BuiltinTypeKind::F32:
        return "float";
    case BuiltinTypeKind::F64:
        return "double";
    case BuiltinTypeKind::Bool:
        return "bool";
    case BuiltinTypeKind::Void:
        return "void";
    case BuiltinTypeKind::Char:
        return "char";
    }
    return nullptr;
}

inline bool TypeReprGen::visitBuiltinType(const BuiltinType* type)
{
    _typeName.append(builtinToC(type));
    return false;
}

bool TypeReprGen::visitArrayType(const ArrayType* type)
{
    _arrayIndices.push_back('[');
    _arrayIndices.append(std::to_string(type->elementCount()));
    _arrayIndices.push_back(']');
    return true;
}

bool TypeReprGen::visitReferenceType(const ReferenceType* type)
{
    if (type->isMutable()) {
        _pointers.push_back(false);
    } else {
        _pointers.push_back(true);
    }
    return true;
}

bool TypeReprGen::visitDynArrayType(const DynArrayType* type)
{
    _hasPrefix = true;
    _typeName.setModName(bmcl::StringView::empty());
    TypeNameGen sng(&_typeName);
    sng.genTypeName(type);
    return false;
}

inline bool TypeReprGen::visitFunctionType(const FunctionType* type)
{
    genFnPointerTypeRepr(type);
    return false;
}

bool TypeReprGen::visitGenericInstantiationType(const GenericInstantiationType* type)
{
    _hasPrefix = true;
    _typeName.setModName(bmcl::StringView::empty());
    TypeNameGen sng(&_typeName);
    sng.genTypeName(type);
    return false;
}

inline bool TypeReprGen::appendTypeName(const NamedType* type)
{
    _hasPrefix = true;
    if (type->moduleName() == "core") {
        _typeName.setModName(bmcl::StringView::empty());
    } else {
        _typeName.setModName(type->moduleName());
    }
    _typeName.append(type->name());
    return false;
}

void TypeReprGen::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    _fieldName = fieldName;
    if (type->isFunction()) {

        genFnPointerTypeRepr(type->asFunction());
        return;
    }
    _hasPrefix = false;
    _typeName.clear();
    _pointers.clear();
    _arrayIndices.clear();
    traverseType(type);

    auto writeTypeName = [&]() {
        if (_hasPrefix) {
            _output->append("Photon");
            _output->appendWithFirstUpper(_typeName.modName());
        }
        _output->append(_typeName.result());
    };

    if (_pointers.size() == 1) {
        if (_pointers[0] == true) {
            _output->append("const ");
        }
        writeTypeName();
        _output->append('*');
    } else {
        writeTypeName();

        for (auto it = _pointers.crbegin(); it < _pointers.crend(); it++) {
            bool isConst  = *it;
            if (isConst) {
                _output->append(" *const");
            } else {
                _output->append(" *");
            }
        }
    }
    if (!fieldName.isEmpty()) {
        _output->appendSpace();
        _output->append(fieldName.begin(), fieldName.end());
    }
    if (!_arrayIndices.empty()) {
        _output->append(_arrayIndices);
    }
}

void TypeReprGen::genFnPointerTypeRepr(const FunctionType* type)
{
    std::vector<const FunctionType*> fnStack;
    const FunctionType* current = type;
    fnStack.push_back(current);
    while (true) {
        bmcl::OptionPtr<const Type> rv = current->returnValue();
        if (rv.isSome()) {
            if (rv.unwrap()->typeKind() == TypeKind::Function) {
                current = static_cast<const FunctionType*>(rv.unwrap());
                fnStack.push_back(current);
            } else {
                break;
            }
        } else {
            break;
        }
    }
    bmcl::StringView fieldName = _fieldName;
    bmcl::OptionPtr<const Type> rv = fnStack.back()->returnValue();
    if (rv.isSome()) {
        genTypeRepr(rv.unwrap());
    } else {
        _output->append("void");
    }
    _output->append(" ");
    for (std::size_t i = 0; i < fnStack.size(); i++) {
        _output->append("(*");
    }
    _output->append(fieldName);

    _output->append(")(");

    auto appendParameters = [this](const FunctionType* t) {
        foreachList(t->argumentsRange(), [this](const Field* field) {
            genTypeRepr(field->type());
        }, [this](const Field*) {
            _output->append(", ");
        });
    };

    for (auto it = fnStack.begin(); it < (fnStack.end() - 1); it++) {
        appendParameters(*it);
        _output->append("))(");
    }
    appendParameters(fnStack.back());
    _output->append(")");
}
}
