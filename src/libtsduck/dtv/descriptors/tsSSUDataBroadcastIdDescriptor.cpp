//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsSSUDataBroadcastIdDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries(),
    private_data()
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


//----------------------------------------------------------------------------
// Convert to a data_broadcast_id_descriptor.
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::toDataBroadcastIdDescriptor(DuckContext& duck, DataBroadcastIdDescriptor& desc) const
{
    if (_is_valid) {
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

void ts::SSUDataBroadcastIdDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(0x000A);        // data_broadcast_id for SSU
    uint8_t* len = bbp->enlarge(1);   // placeholder for oui_data_length
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt8((it->oui >> 16) & 0xFF);
        bbp->appendUInt16(it->oui & 0xFFFF);
        bbp->appendUInt8(0xF0 | (it->update_type & 0x0F));
        bbp->appendUInt8(it->update_version.set() ? (0xE0 | (it->update_version.value() & 0x1F)) : 0xDF);
        bbp->appendUInt8(uint8_t(it->selector.size()));
        bbp->append(it->selector);
    }
    *len = uint8_t(bbp->data() + bbp->size() - len - 1);  // update oui_data_length
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 3 && GetUInt16 (desc.payload()) == 0x000A;
    entries.clear();
    private_data.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        size_t oui_length = data[2];
        data += 3;
        size -= 3;
        if (oui_length > size) {
            oui_length = size;
        }
        while (oui_length >= 6) {
            Entry entry (GetUInt32 (data - 1) & 0x00FFFFFF, data[3] & 0x0F);
            if ((data[4] & 0x20) != 0) {
                entry.update_version = data[4] & 0x1F;
            }
            uint8_t sel_length = data[5];
            data += 6;
            size -= 6;
            oui_length -= 6;
            if (sel_length > oui_length) {
                sel_length = uint8_t(oui_length);
            }
            entry.selector.copy (data, sel_length);
            data += sel_length;
            size -= sel_length;
            oui_length -= sel_length;
            entries.push_back (entry);
        }
        private_data.copy (data, size);
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

void ts::SSUDataBroadcastIdDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds)
{
    DataBroadcastIdDescriptor::DisplayDescriptor(display, did, payload, size, indent, tid, pds);
}
