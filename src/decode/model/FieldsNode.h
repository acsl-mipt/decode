#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Hash.h"
#include "decode/parser/Containers.h"
#include "decode/model/Node.h"

#include <bmcl/StringView.h>
#include <bmcl/Fwd.h>

#include <cstdint>
#include <unordered_map>

namespace decode {

class Component;
class ValueNode;
class ValueInfoCache;
class ModelEventHandler;

class FieldsNode : public Node {
public:
    FieldsNode(FieldVec::ConstRange, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~FieldsNode();

    bmcl::OptionPtr<ValueNode> nodeWithName(bmcl::StringView name);

    void setName(bmcl::StringView name);

    bool encodeFields(ModelEventHandler* handler, bmcl::MemWriter* dest) const;

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

public:
    std::unordered_map<bmcl::StringView, Rc<ValueNode>> _nameToNodeMap;
    std::vector<Rc<ValueNode>> _nodes;
    bmcl::StringView _name;
};

}
