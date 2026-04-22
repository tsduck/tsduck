//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCBIOPMessage.h"
#include "tsNames.h"
#include "tsxmlElement.h"
#include <cstring>


//----------------------------------------------------------------------------
// Out-of-line destructors: give the empty leaf classes a vtable anchor.
//----------------------------------------------------------------------------

ts::BIOPServiceGatewayMessage::~BIOPServiceGatewayMessage() = default;
ts::BIOPDirectoryMessage::~BIOPDirectoryMessage() = default;


//----------------------------------------------------------------------------
// BIOPMessageHeader - Check if the header is valid
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::isValid() const
{
    if (magic != BIOP_MAGIC) {
        return false;
    }

    if (version_major != BIOP_VERSION_MAJOR) {
        return false;
    }

    if (byte_order != BIOP_BYTE_ORDER_BIG_ENDIAN) {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Clear the content
//----------------------------------------------------------------------------

void ts::BIOPMessageHeader::clear()
{
    magic = BIOP_MAGIC;
    version_major = BIOP_VERSION_MAJOR;
    version_minor = BIOP_VERSION_MINOR;
    byte_order = BIOP_BYTE_ORDER_BIG_ENDIAN;
    message_type = BIOP_MESSAGE_TYPE_STANDARD;
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Serialize
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::serialize(PSIBuffer& buf) const
{
    if (buf.remainingWriteBytes() < HEADER_SIZE) {
        buf.setUserError();
        return false;
    }

    // Write header fields
    buf.putUInt32(magic);
    buf.putUInt8(version_major);
    buf.putUInt8(version_minor);
    buf.putUInt8(byte_order);
    buf.putUInt8(message_type);

    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Deserialize
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::deserialize(PSIBuffer& buf)
{
    if (buf.remainingReadBytes() < HEADER_SIZE) {
        buf.setUserError();
        return false;
    }

    if (!buf.isBigEndian()) {
        buf.setBigEndian();
    }

    magic = buf.getUInt32();
    version_major = buf.getUInt8();
    version_minor = buf.getUInt8();
    byte_order = buf.getUInt8();
    message_type = buf.getUInt8();

    if (!isValid()) {
        buf.setUserError();
        return false;
    }

    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Display
//----------------------------------------------------------------------------

void ts::BIOPMessageHeader::display(TablesDisplay& disp, const UString& margin) const
{
    disp << margin << "BIOP Message Header:" << std::endl;
    disp << margin << UString::Format(u"  Magic: %n", magic);

    if (magic == BIOP_MAGIC) {
        disp << " (ASCII: \"BIOP\")";
    }
    else {
        disp << " (Invalid magic number)";
    }
    disp << std::endl;

    disp << margin << UString::Format(u"  BIOP version: %d.%d", version_major, version_minor) << std::endl;

    disp << margin << "  Byte order: ";
    if (byte_order == BIOP_BYTE_ORDER_BIG_ENDIAN) {
        disp << "Big-endian";
    }
    else {
        disp << UString::Format(u"Invalid (%n)", byte_order);
    }
    disp << std::endl;

    disp << margin << UString::Format(u"  Message type: %n", message_type) << std::endl;
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - Static Display
//----------------------------------------------------------------------------

bool ts::BIOPMessageHeader::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (buf.remainingReadBytes() < HEADER_SIZE) {
        disp.displayExtraData(buf, margin);
        return false;
    }

    BIOPMessageHeader header;
    if (header.deserialize(buf)) {
        header.display(disp, margin);
        return true;
    }
    else {
        disp << margin << "Invalid BIOP message header" << std::endl;
        disp.displayExtraData(buf, margin);
        return false;
    }
}


//----------------------------------------------------------------------------
// BIOPMessage - kindTag (strip trailing nulls)
//----------------------------------------------------------------------------

std::string ts::BIOPMessage::kindTag() const
{
    size_t len = object_kind.size();
    while (len > 0 && object_kind[len - 1] == 0) {
        --len;
    }
    return std::string(reinterpret_cast<const char*>(object_kind.data()), len);
}


//----------------------------------------------------------------------------
// BIOPMessage - deserializeCommon
//----------------------------------------------------------------------------

bool ts::BIOPMessage::deserializeCommon(PSIBuffer& buf, uint32_t& body_length)
{
    body_length = 0;

    if (!header.deserialize(buf)) {
        return false;
    }

    // message_size is checked explicitly so we don't push a scope larger than the buffer.
    const uint32_t message_size = buf.getUInt32();
    if (buf.error() || buf.remainingReadBytes() < message_size) {
        buf.setUserError();
        return false;
    }

    // Constrain subsequent reads to message_size. The body-length scope is pushed
    // again inside Parse() once we know the body length. RAII guard pops the scope
    // on any failure path; on success we release it so Parse() can pop it itself.
    buf.pushReadSize(buf.currentReadByteOffset() + message_size);
    struct ScopeGuard {
        PSIBuffer& buf;
        bool released = false;
        ~ScopeGuard() { if (!released) { buf.popState(); } }
    } scope_guard{buf};

    // From here on, PSIBuffer self-checks overruns — `getBytes` / `getUIntN` flip the
    // buffer into error state on underflow, and later reads become no-ops. A single
    // `!buf.error()` at the bottom catches every failure path.

    const uint8_t key_len = buf.getUInt8();
    buf.getBytes(object_key, key_len);

    const uint32_t kind_len = buf.getUInt32();
    buf.getBytes(object_kind, kind_len);

    const uint16_t info_len = buf.getUInt16();
    buf.getBytes(object_info, info_len);

    const uint8_t svc_count = buf.getUInt8();
    service_contexts.clear();
    service_contexts.reserve(svc_count);
    for (uint8_t i = 0; i < svc_count; ++i) {
        BIOPServiceContext ctx;
        ctx.context_id = buf.getUInt32();
        const uint16_t ctx_len = buf.getUInt16();
        buf.getBytes(ctx.context_data, ctx_len);
        service_contexts.push_back(std::move(ctx));
    }

    body_length = buf.getUInt32();
    if (buf.error() || buf.remainingReadBytes() < body_length) {
        return false;
    }

    scope_guard.released = true;
    return true;
}


//----------------------------------------------------------------------------
// BIOPMessage - CreateForKind factory
//----------------------------------------------------------------------------

std::unique_ptr<ts::BIOPMessage> ts::BIOPMessage::CreateForKind(const std::string& tag)
{
    if (tag == BIOPObjectKind::FILE) {
        return std::make_unique<BIOPFileMessage>();
    }
    if (tag == BIOPObjectKind::SERVICE_GATEWAY) {
        return std::make_unique<BIOPServiceGatewayMessage>();
    }
    if (tag == BIOPObjectKind::DIRECTORY) {
        return std::make_unique<BIOPDirectoryMessage>();
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// BIOPMessage - static Parse factory
//----------------------------------------------------------------------------

namespace {
    // Smallest concrete BIOPMessage we can use as a temporary holder for the common
    // fields before we know the object_kind. Body deserialization is a no-op; the
    // instance exists only long enough to read the header/object_key/object_kind
    // and then have its fields moved into the real subclass.
    class BIOPCommonHolder : public ts::BIOPMessage
    {
        TS_NOCOPY(BIOPCommonHolder);

    public:
        BIOPCommonHolder() = default;

        bool readCommon(ts::PSIBuffer& buf, uint32_t& body_length)
        {
            return deserializeCommon(buf, body_length);
        }
    protected:
        bool deserializeBody(ts::PSIBuffer&) override { return true; }
        void toXMLBody(ts::DuckContext&, ts::xml::Element*) const override {}
        bool fromXMLBody(ts::DuckContext&, const ts::xml::Element*) override { return true; }
    };
}

std::unique_ptr<ts::BIOPMessage> ts::BIOPMessage::Parse(PSIBuffer& buf)
{
    std::unique_ptr<ts::BIOPMessage> msg;
    BIOPCommonHolder common;
    uint32_t body_length = 0;
    if (common.readCommon(buf, body_length)) {
        msg = CreateForKind(common.kindTag());
        if (!msg) {
            // Unsupported kind: popState jumps to the end of the message_size scope,
            // swallowing the remaining body bytes so the next Parse() can resume cleanly.
            buf.popState();
        }
        else {
            // Transfer common fields.
            msg->header = common.header;
            msg->object_key = std::move(common.object_key);
            msg->object_kind = std::move(common.object_kind);
            msg->object_info = std::move(common.object_info);
            msg->service_contexts = std::move(common.service_contexts);

            // Constrain body reads to messageBody_length.
            buf.pushReadSize(buf.currentReadByteOffset() + body_length);
            const bool ok = msg->deserializeBody(buf);
            buf.popState();  // body scope
            buf.popState();  // message_size scope from deserializeCommon

            if (!ok) {
                msg.reset();
            }
        }
    }
    return msg;
}


//----------------------------------------------------------------------------
// BIOPFileMessage - deserializeBody
//----------------------------------------------------------------------------

bool ts::BIOPFileMessage::deserializeBody(PSIBuffer& buf)
{
    const uint32_t content_length = buf.getUInt32();
    buf.getBytes(content, content_length);
    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPNameComponent helpers
//----------------------------------------------------------------------------

ts::UString ts::BIOPNameComponent::idString() const
{
    size_t len = id.size();
    while (len > 0 && id[len - 1] == 0) {
        --len;
    }
    return UString::FromUTF8(reinterpret_cast<const char*>(id.data()), len);
}


std::string ts::BIOPNameComponent::kindTag() const
{
    size_t len = kind.size();
    while (len > 0 && kind[len - 1] == 0) {
        --len;
    }
    return std::string(reinterpret_cast<const char*>(kind.data()), len);
}


//----------------------------------------------------------------------------
// BIOPBinding - deserialize one binding
//----------------------------------------------------------------------------

bool ts::BIOPBinding::deserialize(PSIBuffer& buf)
{
    const uint8_t nc_count = buf.getUInt8();
    name.clear();
    name.reserve(nc_count);
    for (uint8_t i = 0; i < nc_count; ++i) {
        BIOPNameComponent nc;
        const uint8_t id_len = buf.getUInt8();
        buf.getBytes(nc.id, id_len);
        const uint8_t kind_len = buf.getUInt8();
        buf.getBytes(nc.kind, kind_len);
        name.push_back(std::move(nc));
    }

    binding_type = buf.getUInt8();
    ior.deserialize(buf);

    const uint16_t oi_len = buf.getUInt16();
    buf.getBytes(object_info, oi_len);

    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPBinding - target IOR location and joined path helpers
//----------------------------------------------------------------------------

std::optional<std::pair<uint16_t, ts::ByteBlock>> ts::BIOPBinding::targetLocation() const
{
    for (const auto& tp : ior.tagged_profiles) {
        if (tp.profile_id_tag != DSMCC_TAG_BIOP) {
            continue;
        }
        for (const auto& lc : tp.lite_components) {
            if (lc.component_id_tag == DSMCC_TAG_OBJECT_LOCATION) {
                return std::make_pair(lc.module_id, lc.object_key_data);
            }
        }
    }
    return std::nullopt;
}


ts::UString ts::BIOPBinding::pathString() const
{
    UString out;
    for (const auto& nc : name) {
        if (!out.empty()) {
            out += u"/";
        }
        out += nc.idString();
    }
    return out;
}


//----------------------------------------------------------------------------
// BIOPBindingListMessage - deserializeBody (shared by Directory / ServiceGateway)
//----------------------------------------------------------------------------

bool ts::BIOPBindingListMessage::deserializeBody(PSIBuffer& buf)
{
    const uint16_t count = buf.getUInt16();
    bindings.clear();
    bindings.reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        BIOPBinding b;
        if (!b.deserialize(buf)) {
            return false;
        }
        bindings.push_back(std::move(b));
    }
    return !buf.error();
}


//----------------------------------------------------------------------------
// BIOPMessageHeader - toXML / fromXML
//----------------------------------------------------------------------------

void ts::BIOPMessageHeader::toXML(DuckContext& duck, xml::Element* element) const
{
    element->setIntAttribute(u"magic", magic, true);
    element->setIntAttribute(u"version_major", version_major, true);
    element->setIntAttribute(u"version_minor", version_minor, true);
    element->setIntAttribute(u"byte_order", byte_order, true);
    element->setIntAttribute(u"message_type", message_type, true);
}

bool ts::BIOPMessageHeader::fromXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(magic, u"magic", false, BIOP_MAGIC) &&
           element->getIntAttribute(version_major, u"version_major", false, BIOP_VERSION_MAJOR) &&
           element->getIntAttribute(version_minor, u"version_minor", false, BIOP_VERSION_MINOR) &&
           element->getIntAttribute(byte_order, u"byte_order", false, BIOP_BYTE_ORDER_BIG_ENDIAN) &&
           element->getIntAttribute(message_type, u"message_type", false, BIOP_MESSAGE_TYPE_STANDARD);
}


//----------------------------------------------------------------------------
// BIOPServiceContext - toXML / fromXML
//----------------------------------------------------------------------------

void ts::BIOPServiceContext::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"service_context");
    e->setIntAttribute(u"context_id", context_id, true);
    e->addHexaTextChild(u"context_data", context_data, true);
}

bool ts::BIOPServiceContext::fromXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(context_id, u"context_id", true) &&
           element->getHexaTextChild(context_data, u"context_data");
}


//----------------------------------------------------------------------------
// BIOPNameComponent - toXML / fromXML
//----------------------------------------------------------------------------

void ts::BIOPNameComponent::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"name_component");
    e->addHexaTextChild(u"id", id, true);
    e->addHexaTextChild(u"kind", kind, true);
}

bool ts::BIOPNameComponent::fromXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaTextChild(id, u"id") &&
           element->getHexaTextChild(kind, u"kind");
}


//----------------------------------------------------------------------------
// BIOPBinding - toXML / fromXML
//----------------------------------------------------------------------------

void ts::BIOPBinding::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"binding");
    e->setIntAttribute(u"binding_type", binding_type, true);
    for (const auto& nc : name) {
        nc.toXML(duck, e);
    }
    ior.toXML(duck, e);
    e->addHexaTextChild(u"object_info", object_info, true);
}

bool ts::BIOPBinding::fromXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(binding_type, u"binding_type", true);

    for (auto& xnc : element->children(u"name_component", &ok)) {
        ok = ok && name.emplace_back().fromXML(duck, &xnc);
    }

    const xml::Element* xior = element->findFirstChild(u"IOR", true);
    ok = ok && xior != nullptr && ior.fromXML(duck, xior);

    ByteBlock oi;
    if (ok && element->getHexaTextChild(oi, u"object_info", false)) {
        object_info = std::move(oi);
    }

    return ok;
}


//----------------------------------------------------------------------------
// BIOPMessage - toXML / fromXML (common fields + body dispatch)
//----------------------------------------------------------------------------

void ts::BIOPMessage::toXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"BIOP_message");
    e->setAttribute(u"object_kind", UString::FromUTF8(kindTag()));
    header.toXML(duck, e);
    e->addHexaTextChild(u"object_key", object_key, true);
    e->addHexaTextChild(u"object_info", object_info, true);
    for (const auto& ctx : service_contexts) {
        ctx.toXML(duck, e);
    }
    toXMLBody(duck, e);
}

bool ts::BIOPMessage::fromXML(DuckContext& duck, const xml::Element* element)
{
    UString kind_str;
    bool ok = element->getAttribute(kind_str, u"object_kind", true);
    if (ok) {
        // Rebuild the on-wire form: trimmed tag bytes followed by a single NUL terminator.
        const std::string tag = kind_str.toUTF8();
        object_kind.assign(tag.begin(), tag.end());
        object_kind.push_back(0);
    }

    ok = ok && header.fromXML(duck, element);

    ok = ok && element->getHexaTextChild(object_key, u"object_key");

    ByteBlock oi;
    if (ok && element->getHexaTextChild(oi, u"object_info", false)) {
        object_info = std::move(oi);
    }

    for (auto& xctx : element->children(u"service_context", &ok)) {
        ok = ok && service_contexts.emplace_back().fromXML(duck, &xctx);
    }

    ok = ok && fromXMLBody(duck, element);
    return ok;
}


//----------------------------------------------------------------------------
// BIOPMessage - FromXML static factory
//----------------------------------------------------------------------------

std::unique_ptr<ts::BIOPMessage> ts::BIOPMessage::FromXML(DuckContext& duck, const xml::Element* element)
{
    std::unique_ptr<ts::BIOPMessage> msg;
    UString kind_str;
    if (element->getAttribute(kind_str, u"object_kind", true)) {
        msg = CreateForKind(kind_str.toUTF8());
        if (!msg || !msg->fromXML(duck, element)) {
            msg.reset();
        }
    }
    return msg;
}


//----------------------------------------------------------------------------
// BIOPFileMessage - toXMLBody / fromXMLBody
//----------------------------------------------------------------------------

void ts::BIOPFileMessage::toXMLBody(DuckContext& duck, xml::Element* msg_element) const
{
    xml::Element* body = msg_element->addElement(u"fil");
    body->addHexaTextChild(u"content", content, true);
}

bool ts::BIOPFileMessage::fromXMLBody(DuckContext& duck, const xml::Element* msg_element)
{
    const xml::Element* body = msg_element->findFirstChild(u"fil", true);
    return body != nullptr && body->getHexaTextChild(content, u"content");
}


//----------------------------------------------------------------------------
// BIOPBindingListMessage - toXMLBody / fromXMLBody
//----------------------------------------------------------------------------

void ts::BIOPBindingListMessage::toXMLBody(DuckContext& duck, xml::Element* msg_element) const
{
    xml::Element* body = msg_element->addElement(u"bindings");
    for (const auto& b : bindings) {
        b.toXML(duck, body);
    }
}

bool ts::BIOPBindingListMessage::fromXMLBody(DuckContext& duck, const xml::Element* msg_element)
{
    const xml::Element* body = msg_element->findFirstChild(u"bindings", true);
    if (body == nullptr) {
        return false;
    }
    bool ok = true;
    for (auto& xb : body->children(u"binding", &ok)) {
        ok = ok && bindings.emplace_back().fromXML(duck, &xb);
    }
    return ok;
}
