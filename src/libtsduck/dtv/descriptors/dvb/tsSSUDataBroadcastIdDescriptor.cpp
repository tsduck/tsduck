//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSSUDataBroadcastIdDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

// This is not a fully registered descriptor. This is just a specific case of data_broadcast_id_descriptor.
// It has no specific XML representation. It cannot be converted from XML because it has no specific
// syntax. It can be converted to XML, as a <data_broadcast_id_descriptor>.

#define MY_XML_NAME u"data_broadcast_id_descriptor"
#define MY_DID ts::DID_DATA_BROADCAST_ID
#define MY_STD ts::Standards::DVB


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSUDataBroadcastIdDescriptor::SSUDataBroadcastIdDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::SSUDataBroadcastIdDescriptor::SSUDataBroadcastIdDescriptor (uint32_t oui, uint8_t update_type) :
    SSUDataBroadcastIdDescriptor()
{
    entries.push_back(Entry(oui, update_type));
}

ts::SSUDataBroadcastIdDescriptor::SSUDataBroadcastIdDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSUDataBroadcastIdDescriptor()
{
    deserialize(duck, desc);
}

void ts::SSUDataBroadcastIdDescriptor::clearContent()
{
    entries.clear();
    private_data.clear();
}

ts::SSUDataBroadcastIdDescriptor::SSUDataBroadcastIdDescriptor(DuckContext& duck, const DataBroadcastIdDescriptor& desc) :
    SSUDataBroadcastIdDescriptor()
{
    if (!desc.isValid() || desc.data_broadcast_id != 0x000A) {
        invalidate();
    }
    else {
        // Convert using serialization / deserialization.
        Descriptor bin;
        desc.serialize(duck, bin);
        deserialize(duck, bin);
    }
}

ts::SSUDataBroadcastIdDescriptor::Entry::Entry(uint32_t oui_, uint8_t upd_) :
    oui(oui_),
    update_type(upd_)
{
}


//----------------------------------------------------------------------------
// Convert to a data_broadcast_id_descriptor.
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::toDataBroadcastIdDescriptor(DuckContext& duck, DataBroadcastIdDescriptor& desc) const
{
    if (isValid()) {
        // Convert using serialization / deserialization.
        Descriptor bin;
        serialize(duck, bin);
        desc.deserialize(duck, bin);
    }
    else {
        desc.invalidate();
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(0x000A); // data_broadcast_id for SSU
    buf.pushWriteSequenceWithLeadingLength(8); // OUI_data_length
    for (const auto& it : entries) {
        buf.putUInt24(it.oui);
        buf.putBits(0xFF, 4);
        buf.putBits(it.update_type, 4);
        buf.putBits(0xFF, 2);
        buf.putBit(it.update_version.has_value());
        buf.putBits(it.update_version.value_or(0xFF), 5);
        buf.putUInt8(uint8_t(it.selector.size()));
        buf.putBytes(it.selector);
    }
    buf.popState(); // update OUI_data_length
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::deserializePayload(PSIBuffer& buf)
{
    if (buf.getUInt16() != 0x000A || buf.error()) {
        // Not the right type of data_broadcast_id_descriptor
        invalidate();
    }
    else {
        buf.pushReadSizeFromLength(8); // OUI_data_length
        while (buf.canRead()) {
            Entry entry;
            entry.oui = buf.getUInt24();
            buf.skipBits(4);
            buf.getBits(entry.update_type, 4);
            buf.skipBits(2);
            if (buf.getBool()) {
                buf.getBits(entry.update_version, 5);
            }
            else {
                buf.skipBits(5);
            }
            const uint8_t len = buf.getUInt8();
            buf.getBytes(entry.selector, len);
            entries.push_back (entry);
        }
        buf.popState(); // end of OUI_data_length
        buf.getBytes(private_data);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::buildXML(DuckContext& duck, xml::Element* parent) const
{
    // There is no specific representation of this descriptor.
    // Convert to a data_broadcast_id_descriptor.
    DataBroadcastIdDescriptor desc;
    toDataBroadcastIdDescriptor(duck, desc);
    desc.buildXML(duck, parent);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SSUDataBroadcastIdDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    // There is no specific representation of this descriptor.
    // We cannot be called since there is no registration in the XML factory.
    element->report().error(u"Internal error, there is no XML representation for SSUDataBroadcastIdDescriptor");
    return false;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    DataBroadcastIdDescriptor::DisplayDescriptor(disp, buf, margin, did, tid, pds);
}
