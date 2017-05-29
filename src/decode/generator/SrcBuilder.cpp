/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/SrcBuilder.h"

namespace decode {

void SrcBuilder::appendModPrefix()
{
    append("Photon");
    appendWithFirstUpper(_modName);
}

void SrcBuilder::appendReadableSizeCheck(const InlineSerContext& ctx, bmcl::StringView sizeCheck)
{
    appendIndent(ctx);
    append("if (PhotonReader_ReadableSize(src) < ");
    append(sizeCheck);
    append(") {\n");
    appendIndent(ctx);
    append("    return PhotonError_NotEnoughData;\n");
    appendIndent(ctx);
    append("}\n");
}

void SrcBuilder::appendWritableSizeCheck(const InlineSerContext& ctx, bmcl::StringView sizeCheck)
{
    appendIndent(ctx);
    append("if (PhotonWriter_WritableSize(dest) < ");
    append(sizeCheck);
    append(") {\n");
    appendIndent(ctx);
    append("    return PhotonError_NotEnoughSpace;\n");
    appendIndent(ctx);
    append("}\n");
}

void SrcBuilder::appendReadableSizeCheck(const InlineSerContext& ctx, std::size_t size)
{
    appendReadableSizeCheck(ctx, std::to_string(size));
}

void SrcBuilder::appendWritableSizeCheck(const InlineSerContext& ctx, std::size_t size)
{
    appendWritableSizeCheck(ctx, std::to_string(size));
}

void SrcBuilder::appendLoopHeader(const InlineSerContext& ctx, bmcl::StringView loopSize)
{
    appendIndent(ctx);
    append("for (size_t ");
    assert(ctx.indentLevel < 30);
    append(ctx.currentLoopVar());
    append(" = 0; ");
    append(ctx.currentLoopVar());
    append(" < ");
    append(loopSize);
    append("; ");
    append(ctx.currentLoopVar());
    append("++) {\n");
}

void SrcBuilder::appendLoopHeader(const InlineSerContext& ctx, std::size_t loopSize)
{
    appendLoopHeader(ctx, std::to_string(loopSize));
}

void SrcBuilder::appendWithTryMacro(const SrcGen& func)
{
    append("PHOTON_TRY(");
    func(this);
    append(");\n");
}

void SrcBuilder::appendVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix)
{
    append(prefix);
    append(typeName);
    append(' ');
    appendWithFirstLower(varName);
    append(";\n");
}

void SrcBuilder::appendLocalIncludePath(bmcl::StringView path)
{
    append("#include \"photon/");
    append(path);
    append(".h\"\n");
}

void SrcBuilder::appendTagHeader(bmcl::StringView name)
{
    append("typedef ");
    append(name.begin(), name.end());
    append(" {\n");
}

void SrcBuilder::appendTagFooter(bmcl::StringView typeName)
{
    append("} ");
    appendModPrefix();
    append(typeName);
    append(";\n");
}

void SrcBuilder::startIncludeGuard(bmcl::StringView modName, bmcl::StringView typeName)
{
    auto writeGuardMacro = [this, typeName, modName]() {
        append("__PHOTON_");
        append(modName.toUpper());
        append('_');
        append(typeName.toUpper()); //FIXME
        append("_H__\n");
    };
    append("#ifndef ");
    writeGuardMacro();
    append("#define ");
    writeGuardMacro();
    appendEol();
}

void SrcBuilder::startCppGuard()
{
    append("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");
}

void SrcBuilder::endCppGuard()
{
    append("#ifdef __cplusplus\n}\n#endif\n\n");
}

void SrcBuilder::endIncludeGuard()
{
    append("#endif\n");
    appendEol();
}

void SrcBuilder::appendByteArrayDefinition(bmcl::StringView name, bmcl::Bytes data)
{
    append("static uint8_t ");
    append(name);
    append('[');
    appendNumericValue(data.size());
    append("] = {");
    std::size_t size;
    const std::size_t maxBytes = 12;
    for (size = 0; size < data.size(); size++) {
        if ((size % maxBytes) == 0) {
            append("\n   ");
        }
        uint8_t b = data[size];
        append(" ");
        appendHexValue(b);
        append(",");
    }
    append("\n};\n");

}
}
