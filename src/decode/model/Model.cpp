#include "decode/model/Model.h"
#include "decode/core/Try.h"
#include "decode/parser/Package.h"
#include "decode/model/Decoder.h"
#include "decode/model/FieldsNode.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/parser/Decl.h" //HACK
#include "decode/parser/Type.h" //HACK
#include "decode/parser/Component.h" //HACK

#include <bmcl/ArrayView.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>

namespace decode {

Model::Model(const Package* package)
    : Node(nullptr)
    , _package(package)
    , _cache(new ValueInfoCache)
{
    for (auto it : package->components()) {
        BMCL_DEBUG() << it.first << " " << it.second->number() << " " <<  it.second->name().toStdString() << " " << it.second->parameters().isSome() << " " << it.second->statuses().isSome();
        if (it.second->parameters().isNone()) {
            continue;
        }

        Rc<FieldsNode> node = new FieldsNode(it.second->parameters()->get(), _cache.get(), this);
        node->setName(it.second->name());
        _nodes.emplace_back(node);

        if (it.second->statuses().isNone()) {
            continue;
        }

        Rc<StatusDecoder> decoder = new StatusDecoder(it.second->statuses()->get(), node.get());
        _decoders.emplace(it.first, decoder);
    }
}

Model::~Model()
{
}

std::size_t Model::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> Model::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

Node* Model::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView Model::fieldName() const
{
    return "photon";
}

void Model::acceptTelemetry(bmcl::ArrayView<uint8_t> bytes)
{
    bmcl::MemReader stream(bytes.data(), bytes.size());

    while (!stream.isEmpty()) {
        if (stream.sizeLeft() < 2) {
            //TODO: report error
            return;
        }

        uint16_t msgSize = stream.readUint16Le();
        if (stream.sizeLeft() < msgSize) {
            //TODO: report error
            return;
        }

        bmcl::MemReader msg(stream.current(), msgSize);
        uint64_t compId;
        if (!msg.readVarUint(&compId)) {
            //TODO: report error
            return;
        }

        auto it = _decoders.find(compId);
        if (it == _decoders.end()) {
            //TODO: report error
            return;
        }

        if (!it->second->decode(&msg)) {
            //TODO: report error
            return;
        }

        stream.skip(msgSize);
    }
}
}