/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/generator/NameVisitor.h"
#include "decode/generator/SrcBuilder.h"

#include <string>

namespace decode {

class TypeNameGen : public NameVisitor<TypeNameGen> {
public:
    TypeNameGen(StringBuilder* dest);

    static std::string genTypeNameAsString(const Type* type);

    void genTypeName(const Type* type);

    bool visitBuiltinType(const BuiltinType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitSliceType(const SliceType* type);

    bool appendTypeName(const NamedType* type);

private:
    StringBuilder* _output;
};
}
