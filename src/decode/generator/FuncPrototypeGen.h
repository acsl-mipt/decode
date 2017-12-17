/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <cstdint>

namespace decode {

class SrcBuilder;
class TypeReprGen;
class Type;
class EventMsg;
class Component;
class Command;
class StatusMsg;

class FuncPrototypeGen {
public:
    FuncPrototypeGen(SrcBuilder* output);
    ~FuncPrototypeGen();

    void appendEventFuncDecl(const Component* comp, const EventMsg* msg, TypeReprGen* reprGen);
    void appendCmdFuncDecl(const Component* comp, const Command* cmd, TypeReprGen* reprGen);
    void appendDeserializerFuncDecl(const Type* type);
    void appendSerializerFuncDecl(const Type* type);
    void appendStatusMessageGenFuncDecl(const Component* comp, const StatusMsg* msg);
    void appendStatusMessageGenFuncName(const Component* comp, const StatusMsg* msg);

private:
    SrcBuilder* _output;
};

}
