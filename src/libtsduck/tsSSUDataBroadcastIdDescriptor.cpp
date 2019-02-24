//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//
//  Representation of a data_broadcast_id_descriptor for system software update
//  (data_broadcast_id 0x000A).
//
//----------------------------------------------------------------------------

#include "tsSSUDataBroadcastIdDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u""   // No XML conversion.
#define MY_DID ts::DID_DATA_BROADCAST_ID
#define MY_STD ts::STD_DVB


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSUDataBroadcastIdDescriptor::SSUDataBroadcastIdDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries(),
    private_data()
{
    _is_valid = true;
}

ts::SSUDataBroadcastIdDescriptor::SSUDataBroadcastIdDescriptor (uint32_t oui, uint8_t update_type) :
    SSUDataBroadcastIdDescriptor()
{
    entries.push_back(Entry(oui, update_type));
    _is_valid = true;
}

ts::SSUDataBroadcastIdDescriptor::SSUDataBroadcastIdDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    SSUDataBroadcastIdDescriptor()
{
    deserialize(desc, charset);
}

ts::SSUDataBroadcastIdDescriptor::SSUDataBroadcastIdDescriptor(const DataBroadcastIdDescriptor& desc, const DVBCharset* charset) :
    SSUDataBroadcastIdDescriptor()
{
    _is_valid = desc.isValid() && desc.data_broadcast_id == 0x000A;
    if (_is_valid) {
        // Convert using serialization / deserialization.
        Descriptor bin;
        desc.serialize(bin, charset);
        deserialize(bin, charset);
    }
}


//----------------------------------------------------------------------------
// Convert to a data_broadcast_id_descriptor.
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::toDataBroadcastIdDescriptor(DataBroadcastIdDescriptor& desc, const DVBCharset* charset) const
{
    if (_is_valid) {
        // Convert using serialization / deserialization.
        Descriptor bin;
        serialize(bin, charset);
        desc.deserialize(bin, charset);
    }
    else {
        desc.invalidate();
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    bbp->appendUInt16 (0x000A); // data_broadcast_id for SSU
    bbp->enlarge (1); // placeholder for oui_data_length at offset 4
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt8((it->oui >> 16) & 0xFF);
        bbp->appendUInt16(it->oui & 0xFFFF);
        bbp->appendUInt8(0xF0 | (it->update_type & 0x0F));
        bbp->appendUInt8(it->update_version.set() ? (0xE0 | (it->update_version.value() & 0x1F)) : 0xDF);
        bbp->appendUInt8(uint8_t(it->selector.size()));
        bbp->append(it->selector);
    }
    (*bbp)[4] = uint8_t(bbp->size() - 5);  // update oui_data_length
    bbp->append(private_data);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d(bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 3 && GetUInt16 (desc.payload()) == 0x000A;
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

ts::xml::Element* ts::SSUDataBroadcastIdDescriptor::toXML(xml::Element* parent) const
{
    // There is no specific representation of this descriptor.
    // Convert to a data_broadcast_id_descriptor.
    DataBroadcastIdDescriptor desc;
    toDataBroadcastIdDescriptor(desc);
    return desc.toXML(parent);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::fromXML(const xml::Element* element)
{
    // There is no specific representation of this descriptor.
    // We cannot be called since there is no registration in the XML factory.
    element->report().error(u"Internal error, there is no XML representation for SSUDataBroadcastIdDescriptor");
    _is_valid = false;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSUDataBroadcastIdDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds)
{
    DataBroadcastIdDescriptor::DisplayDescriptor(display, did, payload, size, indent, tid, pds);
}
