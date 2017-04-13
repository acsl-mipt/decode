#include "decode/generator/InlineTypeSerializerGen.h"

namespace decode {

InlineTypeSerializerGen::InlineTypeSerializerGen(TypeReprGen* reprGen, SrcBuilder* output)
    : InlineTypeInspector<InlineTypeSerializerGen>(reprGen, output)
{
}

InlineTypeSerializerGen::~InlineTypeSerializerGen()
{
}

void InlineTypeSerializerGen::inspectPointer(const Type* type)
{
    _output->appendWritableSizeCheck(context(), "sizeof(void*)");
    _output->appendIndent(context());
    _output->append("PhotonWriter_WritePtrLe(dest, (const void*)");
    appendArgumentName();
    _output->append(");\n");
}

void InlineTypeSerializerGen::inspectNonInlineType(const Type* type)
{
    _output->appendIndent(context());
    _output->appendWithTryMacro([&, this, type](SrcBuilder* output) {
        appendTypeRepr(type);
        output->append("_Serialize(");
        if (type->typeKind() != TypeKind::Enum) {
            output->append('&');
        }
        appendArgumentName();
        output->append(", dest)");
    });
}

void InlineTypeSerializerGen::genSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix)
{
    _output->appendWritableSizeCheck(context(), sizeCheck);
    _output->appendIndent(context());
    _output->append("PhotonWriter_Write");
    _output->append(suffix);
    _output->append("(dest, ");
    appendArgumentName();
    _output->append(");\n");
}

void InlineTypeSerializerGen::genVarSer(bmcl::StringView suffix)
{
    _output->appendIndent(context());
    _output->appendWithTryMacro([&](SrcBuilder* output) {
        output->append("PhotonWriter_Write");
        output->append(suffix);
        output->append("(dest, ");
        appendArgumentName();
        output->append(")");
    });
}
}
