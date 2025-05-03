//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDescriptor.h"
#include "tsMemory.h"
#include "tsAbstractDescriptor.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::Descriptor::Descriptor(DID tag, const void* data, size_t size)
{
    if (data != nullptr && size < 256) {
        ByteBlockPtr ptr(std::make_shared<ByteBlock>(size + 2));
        (*ptr)[0] = tag;
        (*ptr)[1] = uint8_t(size);
        MemCopy(ptr->data() + 2, data, size);
        SuperClass::reload(ptr); // reuse the pointer
    }
}

ts::Descriptor::Descriptor(DID tag, const ByteBlock& data)
{
    if (data.size() < 256) {
        ByteBlockPtr ptr(std::make_shared<ByteBlock>(2));
        (*ptr)[0] = tag;
        (*ptr)[1] = uint8_t(data.size());
        ptr->append(data);
        SuperClass::reload(ptr); // reuse the pointer
    }
}

ts::Descriptor::~Descriptor()
{
}


//----------------------------------------------------------------------------
// Get the extension descriptor id.
//----------------------------------------------------------------------------

ts::XDID ts::Descriptor::xdid() const
{
    const DID did = tag();
    if ((did == DID_MPEG_EXTENSION || did == DID_DVB_EXTENSION) && payloadSize() > 0) {
        return XDID(did, payload()[0]);
    }
    else {
        return XDID(did);
    }
}


//----------------------------------------------------------------------------
// Replace the payload of the descriptor.
//----------------------------------------------------------------------------

void ts::Descriptor::replacePayload(const void* addr, size_t size)
{
    if (size > 255) {
        // Payload size too long, invalidate descriptor
        clear();
    }
    else {
        // Erase previous payload.
        rwResize(2);
        // Add new payload
        rwAppend(addr, size);
        // Adjust descriptor size
        rwContent()[1] = uint8_t(size);
    }
}


//----------------------------------------------------------------------------
// Resize (truncate or extend) the payload of the descriptor.
//----------------------------------------------------------------------------

void ts::Descriptor::resizePayload(size_t new_size)
{
    if (new_size > 255) {
        // Payload size too long, invalidate descriptor
        clear();
    }
    else {
        // Resize and pas with zeroes if extended.
        rwResize(new_size + 2);
        // Adjust descriptor size
        rwContent()[1] = uint8_t(new_size);
    }
}


//----------------------------------------------------------------------------
// Deserialize the descriptor.
//----------------------------------------------------------------------------

ts::AbstractDescriptorPtr ts::Descriptor::deserializeImpl(DuckContext& duck, PSIRepository::DescriptorFactory fac) const
{
    if (fac != nullptr) {
        AbstractDescriptorPtr dp(fac());
        if (dp != nullptr) {
            // Deserialize from binary to object.
            dp->deserialize(duck, *this);
            if (dp->isValid()) {
                // Successfully deserialized.
                return dp;
            }
        }
    }
    return nullptr; // cannot deserialize
}

ts::AbstractDescriptorPtr ts::Descriptor::deserialize(DuckContext& duck, EDID edid) const
{
    return deserializeImpl(duck, PSIRepository::Instance().getDescriptor(edid).factory);
}

ts::AbstractDescriptorPtr ts::Descriptor::deserialize(DuckContext& duck, DescriptorContext& context) const
{
    return deserializeImpl(duck, PSIRepository::Instance().getDescriptor(xdid(), context).factory);
}


//----------------------------------------------------------------------------
// This method converts a descriptor to XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::Descriptor::toXML(DuckContext& duck, xml::Element* parent, DescriptorContext& context, bool forceGeneric) const
{
    // Filter invalid descriptors.
    if (!isValid()) {
        return nullptr;
    }

    // The XML node we will generate.
    xml::Element* node = nullptr;

    // Try to generate a specialized XML structure.
    if (!forceGeneric) {
        const AbstractDescriptorPtr dp(deserialize(duck, context));
        if (dp != nullptr) {
            // Serialize from object to XML.
            node = dp->toXML(duck, parent);
        }
    }

    // If we could not generate a typed node, generate a generic one.
    if (node == nullptr) {
        // Create the XML node.
        node = parent->addElement(AbstractDescriptor::XML_GENERIC_DESCRIPTOR);
        node->setIntAttribute(u"tag", tag(), true);
        node->addHexaText(payload(), payloadSize());
    }

    return node;
}


//----------------------------------------------------------------------------
// This method converts an XML node as a binary descriptor.
//----------------------------------------------------------------------------

bool ts::Descriptor::fromXML(DuckContext& duck, const xml::Element* node, TID tid)
{
    EDID unused;
    return fromXML(duck, unused, node, tid);
}

bool ts::Descriptor::fromXML(DuckContext& duck, EDID& edid, const xml::Element* node, TID tid)
{
    // Filter invalid parameters.
    invalidate();
    edid = EDID();
    if (node == nullptr) {
        // Not a valid XML name (not even an XML element).
        return false;
    }

    // If the table is specified and the XML descriptor is not allowed in this table, this is an error.
    if (!PSIRepository::Instance().isDescriptorAllowed(node->name(), tid)) {
        node->report().error(u"<%s>, line %d, is not allowed here, must be in %s",
                             node->name(),
                             node->lineNumber(),
                             PSIRepository::Instance().descriptorTables(duck, node->name()));
        return false;
    }

    // Try to get the descriptor factory for that kind of XML tag.
    const PSIRepository::DescriptorFactory fac = PSIRepository::Instance().getDescriptor(node->name()).factory;
    if (fac != nullptr) {
        // Create a descriptor instance of the right type.
        AbstractDescriptorPtr desc = fac();
        if (desc != nullptr) {
            desc->fromXML(duck, node);
            if (desc->isValid() && desc->serialize(duck, *this)) {
                // The descriptor was successfully serialized.
                edid = desc->edid();
                return true;
            }
        }
    }

    // Try to decode a generic descriptor.
    if (node->name().similar(AbstractDescriptor::XML_GENERIC_DESCRIPTOR)) {
        DID tag = 0xFF;
        ByteBlock payload;
        if (node->getIntAttribute<DID>(tag, u"tag", true, 0xFF, 0x00, 0xFF) && node->getHexaText(payload, 0, 255)) {
            // Build descriptor.
            ByteBlockPtr ptr(std::make_shared<ByteBlock>(2));
            (*ptr)[0] = tag;
            (*ptr)[1] = uint8_t(payload.size());
            ptr->append(payload);
            reload(ptr);
            return isValid();
        }
    }

    // The XML element name was not valid.
    node->report().error(u"<%s>, line %d, is not a valid descriptor", node->name(), node->lineNumber());
    return false;
}
