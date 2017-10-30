/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/CmdDecoderGen.h"
#include "decode/ast/Function.h"

namespace decode {

CmdDecoderGen::CmdDecoderGen(TypeReprGen* reprGen, SrcBuilder* output)
    : _typeReprGen(reprGen)
    , _output(output)
    , _inlineSer(reprGen, _output)
    , _inlineDeser(reprGen, _output)
    , _paramInspector(_output)
{
}

CmdDecoderGen::~CmdDecoderGen()
{
}

void CmdDecoderGen::generateHeader(ComponentMap::ConstRange comps)
{
    (void)comps;
    _output->startIncludeGuard("PRIVATE", "CMD_DECODER");
    _output->appendEol();

    _output->appendLocalIncludePath("core/Error");
    _output->appendLocalIncludePath("core/Reader");
    _output->appendLocalIncludePath("core/Writer");
    _output->appendLocalIncludePath("core/Try");
    _output->appendEol();

    _output->startCppGuard();

    appendMainFunctionPrototype();
    _output->append(";\n");

    _output->appendEol();
    _output->endCppGuard();
    _output->appendEol();
    _output->endIncludeGuard();
}

void CmdDecoderGen::appendFunctionPrototype(unsigned componenNum, unsigned cmdNum)
{
    _output->append("PhotonError ");
    appendFunctionName(componenNum, cmdNum);
    _output->append("(PhotonReader* src, PhotonWriter* dest)");
}

void CmdDecoderGen::appendFunctionName(unsigned componenNum, unsigned cmdNum)
{
    _output->append("decodeCmd");
    _output->appendNumericValue(componenNum);
    _output->append('_');
    _output->appendNumericValue(cmdNum);
}

void CmdDecoderGen::generateSource(ComponentMap::ConstRange comps)
{
    _output->appendLocalIncludePath("CmdDecoder.Private");
    _output->appendLocalIncludePath("core/Logging");
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"CmdDecoder.Private.c\"\n\n");

    for (const Component* it : comps) {
        _output->appendModIfdef(it->moduleName());
        _output->appendComponentInclude(it->moduleName());
        _output->appendEndif();
    }

    for (const Component* it : comps) {
        if (!it->hasCmds()) {
            continue;
        }
        std::size_t cmdNum = 0;
        _output->appendModIfdef(it->moduleName());
        _output->appendEol();
        for (const Function* jt : it->cmdsRange()) {
            generateFunc(it, jt, it->number(), cmdNum);
            cmdNum++;
            _output->appendEol();
            _output->appendEol();
        }
        _output->appendEndif();
        _output->appendEol();
    }

    generateMainFunc(comps);
    _output->append("\n#undef _PHOTON_FNAME\n");
}

void CmdDecoderGen::appendMainFunctionPrototype()
{
    _output->append("PhotonError Photon_ExecCmd(uint8_t compNum, uint8_t cmdNum, PhotonReader* src, PhotonWriter* dest)");
}

void CmdDecoderGen::generateMainFunc(ComponentMap::ConstRange comps)
{
    appendMainFunctionPrototype();
    _output->append("\n{\n");
    _output->append("    switch (compNum) {\n");
    for (const Component* it : comps) {
        if (!it->hasCmds()) {
            continue;
        }
        _output->appendModIfdef(it->moduleName());

        _output->append("    case ");
        _output->appendNumericValue(it->number());
        _output->append(": {\n");

        _output->append("        switch (cmdNum) {\n");

        std::size_t cmdNum = 0;
        for (const Function* func : it->cmdsRange()) {
            (void)func;
            _output->append("        case ");
            _output->appendNumericValue(cmdNum);
            _output->append(":\n");
            _output->append("            return ");
            appendFunctionName(it->number(), cmdNum);
            _output->append("(src, dest);\n");
            cmdNum++;
        }
        _output->append("        default:\n");
        _output->append("            PHOTON_CRITICAL(\"Recieved invalid cmd id\");\n");
        _output->append("            return PhotonError_InvalidCmdId;\n");
        _output->append("        }\n    }\n");
        _output->appendEndif();
    }
    _output->append("    }\n    PHOTON_CRITICAL(\"Recieved invalid component id\");\n");
    _output->append("    return PhotonError_InvalidComponentId;\n}");
}


template <typename C>
void CmdDecoderGen::foreachParam(const Function* func, C&& f)
{
    _paramInspector.reset();
    for (const Field* field : func->type()->argumentsRange()) {
        _paramInspector.beginField(field);
        f(field, _paramInspector.paramName.view());
        _paramInspector.endField(field);
    }
}

void CmdDecoderGen::writePointerOp(const Type* type)
{
    const Type* t = type; //HACK: fix Rc::operator->
    switch (type->typeKind()) {
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::Function:
    case TypeKind::Enum:
    case TypeKind::Builtin:
        break;
    case TypeKind::DynArray:
    case TypeKind::Struct:
    case TypeKind::Variant:
        _output->append("&");
        break;
    case TypeKind::Imported:
        writePointerOp(t->asImported()->link());
        break;
    case TypeKind::Alias:
        writePointerOp(t->asAlias()->alias());
        break;
    case TypeKind::Generic:
        assert(false);
        break;
    case TypeKind::GenericInstantiation:
        writePointerOp(t->asGenericInstantiation()->instantiatedType());
        break;
    case TypeKind::GenericParameter:
        assert(false);
        break;
    }
}

void CmdDecoderGen::writeReturnOp(const Type* type)
{
    const Type* t = type; //HACK: fix Rc::operator->
    switch (type->typeKind()) {
    case TypeKind::Array:
        break;
    case TypeKind::Function:
    case TypeKind::Reference:
    case TypeKind::Enum:
    case TypeKind::Builtin:
    case TypeKind::DynArray:
    case TypeKind::Struct:
    case TypeKind::Variant:
        _output->append("&");
        break;
    case TypeKind::Imported:
        writeReturnOp(t->asImported()->link());
        break;
    case TypeKind::Alias:
        writeReturnOp(t->asAlias()->alias());
        break;
    case TypeKind::Generic:
        assert(false);
        break;
    case TypeKind::GenericInstantiation:
        writeReturnOp(t->asGenericInstantiation()->instantiatedType());
        break;
    case TypeKind::GenericParameter:
        assert(false);
        break;
    }
}

void CmdDecoderGen::generateFunc(const Component* comp, const Function* func, unsigned componenNum, unsigned cmdNum)
{
    const FunctionType* ftype = func->type();
    _output->append("static ");
    appendFunctionPrototype(componenNum, cmdNum);
    _output->append("\n{\n");

    if (!func->type()->hasArguments()) {
        _output->append("    (void)src;\n");
    }

    foreachParam(func, [this](const Field* field, bmcl::StringView name) {
        _output->append("    ");
        _typeReprGen->genTypeRepr(field->type(), name);
        _output->append(";\n");
    });

    bmcl::OptionPtr<const Type> rv = ftype->returnValue();
    if (rv.isSome()) {
        _output->append("    ");
        _typeReprGen->genTypeRepr(rv.unwrap(), "_rv");
        _output->append(";\n");
    } else {
        _output->append("    (void)dest;\n");
    }
    _output->appendEol();


    _paramInspector.reset();
    _paramInspector.inspect(func->fieldsRange(), &_inlineDeser);
    _output->appendEol();

    //TODO: gen command call
    _output->append("    PHOTON_TRY_MSG(Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_");
    _output->appendWithFirstUpper(func->name());
    _output->append("(");
    foreachParam(func, [this](const Field* field, bmcl::StringView name) {
        (void)field;
        writePointerOp(field->type());
        _output->append(name);
        _output->append(", ");
    });
    if (rv.isSome()) {
        writeReturnOp(rv.unwrap());
        _output->append("_rv");
    } else if (ftype->hasArguments()) {
        _output->removeFromBack(2);
    }
    _output->append("), \"Failed to decode cmd\");\n\n");

    InlineSerContext ctx;
    if (rv.isSome()) {
        _inlineSer.inspect(rv.unwrap(), ctx, "_rv");
    }

    _output->append("\n    return PhotonError_Ok;\n}");
}
}
