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

#include "tsSSULinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

// This is not a fully registered descriptor. This is just a specific case of linkage_descriptor.
// It has no specific XML representation. It cannot be converted from XML because it has no specific
// syntax. It can be converted to XML, as a <linkage_descriptor>.

#define MY_XML_NAME u"linkage_descriptor"
#define MY_DID ts::DID_LINKAGE
#define MY_STD ts::Standards::DVB


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSULinkageDescriptor::SSULinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    entries(),
    private_data()
{
}

ts::SSULinkageDescriptor::SSULinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service, uint32_t oui) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    entries(),
    private_data()
{
    entries.push_back(Entry(oui));
}

ts::SSULinkageDescriptor::SSULinkageDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSULinkageDescriptor()
{
    deserialize(duck, desc);
}

void ts::SSULinkageDescriptor::clearContent()
{
    ts_id = 0;
    onetw_id = 0;
    service_id = 0;
    entries.clear();
    private_data.clear();
}

ts::SSULinkageDescriptor::SSULinkageDescriptor(DuckContext& duck, const ts::LinkageDescriptor& desc) :
    SSULinkageDescriptor()
{
    if (!desc.isValid() || desc.linkage_type != LINKAGE_SSU) {
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
// Convert to a linkage_descriptor.
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::toLinkageDescriptor(DuckContext& duck, ts::LinkageDescriptor& desc) const
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

void ts::SSULinkageDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(ts_id);
    bbp->appendUInt16(onetw_id);
    bbp->appendUInt16(service_id);
    bbp->appendUInt8(LINKAGE_SSU);
    uint8_t* len = bbp->enlarge(1); // placeholder for oui_data_length
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt8((it->oui >> 16) & 0xFF);
        bbp->appendUInt16(it->oui & 0xFFFF);
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

void ts::SSULinkageDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 8;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        ts_id = GetUInt16 (data);
        onetw_id = GetUInt16 (data + 2);
        service_id = GetUInt16 (data + 4);
        entries.clear();
        private_data.clear();
        uint8_t linkage_type = data[6];
        _is_valid = linkage_type == LINKAGE_SSU;
        if (_is_valid) {
            size_t oui_length = data[7];
            data += 8;
            size -= 8;
            if (oui_length > size) {
                oui_length = size;
            }
            while (oui_length >= 4) {
                Entry entry (GetUInt32 (data - 1) & 0x00FFFFFF);
                uint8_t sel_length = data[3];
                data += 4;
                size -= 4;
                oui_length -= 4;
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
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::buildXML(DuckContext& duck, xml::Element* parent) const
{
    // There is no specific representation of this descriptor.
    // Convert to a linkage_descriptor.
    LinkageDescriptor desc;
    toLinkageDescriptor(duck, desc);
    desc.buildXML(duck, parent);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SSULinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    // There is no specific representation of this descriptor.
    // We cannot be called since there is no registration in the XML factory.
    element->report().error(u"Internal error, there is no XML representation for SSULinkageDescriptor");
    return false;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds)
{
    LinkageDescriptor::DisplayDescriptor(display, did, payload, size, indent, tid, pds);
}
