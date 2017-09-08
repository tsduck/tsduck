//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Representation of a linkage_descriptor for system software update
//  (linkage type 0x09).
//
//----------------------------------------------------------------------------

#include "tsSSULinkageDescriptor.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::SSULinkageDescriptor, "SSU_linkage_descriptor");


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::SSULinkageDescriptor::SSULinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service) :
    AbstractDescriptor(DID_LINKAGE, "SSU_linkage_descriptor"),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    entries(),
    private_data()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor with one OUI
//----------------------------------------------------------------------------

ts::SSULinkageDescriptor::SSULinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service, uint32_t oui) :
    AbstractDescriptor(DID_LINKAGE, "SSU_linkage_descriptor"),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    entries(),
    private_data()
{
    entries.push_back(Entry(oui));
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::SSULinkageDescriptor::SSULinkageDescriptor(const Descriptor& desc) :
    AbstractDescriptor(DID_LINKAGE, "SSU_linkage_descriptor"),
    ts_id(0),
    onetw_id(0),
    service_id(0),
    entries(),
    private_data()
{
    deserialize(desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::serialize(Descriptor& desc) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    bbp->appendUInt16 (ts_id);
    bbp->appendUInt16 (onetw_id);
    bbp->appendUInt16 (service_id);
    bbp->appendUInt8 (LINKAGE_SSU);
    bbp->enlarge (1); // placeholder for oui_data_length at offset 9
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt8 ((it->oui >> 16) & 0xFF);
        bbp->appendUInt16 (it->oui & 0xFFFF);
        bbp->appendUInt8 (uint8_t(it->selector.size()));
        bbp->append (it->selector);
    }
    (*bbp)[9] = uint8_t(bbp->size() - 10);  // update oui_data_length
    bbp->append (private_data);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::deserialize (const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 8;

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

ts::XML::Element* ts::SSULinkageDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    return 0; // TODO @@@@
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SSULinkageDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    // TODO @@@@
}
