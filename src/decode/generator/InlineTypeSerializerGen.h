#pragma once

#include "decode/Config.h"
#include "decode/generator/InlineTypeInspector.h"

namespace decode {

class InlineTypeSerializerGen : public InlineTypeInspector<InlineTypeSerializerGen> {
public:
    InlineTypeSerializerGen(TypeReprGen* reprGen, SrcBuilder* output);
    ~InlineTypeSerializerGen();

    void inspectPointer(const Type* type);
    void inspectNonInlineType(const Type* type);

    void genSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix);
    void genVarSer(bmcl::StringView suffix);
};

}
