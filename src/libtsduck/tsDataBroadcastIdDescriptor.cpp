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
//  Representation of a generic data_broadcast_id_descriptor.
//  Specialized classes exist, depending on the data_broadcast_id
//
//----------------------------------------------------------------------------

#include "tsDataBroadcastIdDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"data_broadcast_id_descriptor"
#define MY_DID ts::DID_DATA_BROADCAST_ID

TS_XML_DESCRIPTOR_FACTORY(ts::DataBroadcastIdDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::DataBroadcastIdDescriptor, ts::EDID(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::DataBroadcastIdDescriptor::DisplayDescriptor, ts::EDID(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::DataBroadcastIdDescriptor::DataBroadcastIdDescriptor(uint16_t id) :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    data_broadcast_id(id),
    private_data()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::DataBroadcastIdDescriptor::DataBroadcastIdDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    data_broadcast_id(0),
    private_data()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::serialize (Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    bbp->appendUInt16 (data_broadcast_id);
    bbp->append (private_data);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::deserialize (const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 2;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        data_broadcast_id = GetUInt16 (data);
        private_data.copy (data + 2, size - 2);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 2) {
        uint16_t id = GetUInt16(data);
        data += 2; size -= 2;
        strm << margin << "Data broadcast id: " << names::DataBroadcastId(id, names::BOTH_FIRST) << std::endl;
        // The rest of the descriptor is the "id selector".
        DisplaySelectorBytes(display, data, size, indent, id);
        data += size; size = 0;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Static method to display a data broadcast selector bytes.
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::DisplaySelectorBytes(TablesDisplay& display, const uint8_t* data, size_t size, int indent, uint16_t dbid)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    // Interpretation depends in the data broadcast id.
    if (dbid == 0x000A && size >= 1) {
        // System Software Update (ETSI TS 102 006)
        // Id selector is a system_software_update_info structure.
        // OUI_data_length:
        uint8_t dlength = data[0];
        data += 1; size -= 1;
        if (dlength > size) {
            dlength = uint8_t(size);
        }
        // OUI loop:
        while (dlength >= 6) {
            // Get fixed part (6 bytes)
            uint32_t oui = GetUInt32(data - 1) & 0x00FFFFFF; // 24 bits
            uint8_t upd_type = data[3] & 0x0F;
            uint8_t upd_flag = (data[4] >> 5) & 0x01;
            uint8_t upd_version = data[4] & 0x1F;
            uint8_t slength = data[5];
            data += 6; size -= 6; dlength -= 6;
            // Get variable-length selector
            const uint8_t* sdata = data;
            if (slength > dlength) {
                slength = dlength;
            }
            data += slength; size -= slength; dlength -= slength;
            // Display
            strm << margin << "OUI: " << names::OUI(oui, names::FIRST) << std::endl
                 << margin << UString::Format(u"  Update type: 0x%X (", {upd_type});
            switch (upd_type) {
                case 0x00: strm << "proprietary update solution"; break;
                case 0x01: strm << "standard update carousel (no notification) via broadcast"; break;
                case 0x02: strm << "system software update with UNT via broadcast"; break;
                case 0x03: strm << "system software update using return channel with UNT"; break;
                default:   strm << "reserved"; break;
            }
            strm << ")" << std::endl << margin << "  Update version: ";
            if (upd_flag == 0) {
                strm << "none";
            }
            else {
                strm << UString::Format(u"%d (0x%02X)", {upd_version, upd_version});
            }
            strm << std::endl;
            if (slength > 0) {
                strm << margin << "  Selector data:" << std::endl
                     << UString::Dump(sdata, slength, UString::HEXA | UString::ASCII, indent + 2);
            }
        }
        // Extraneous data in OUI_loop:
        if (dlength > 0) {
            strm << margin << "Extraneous data in OUI loop:" << std::endl
                 << UString::Dump(data, dlength, UString::HEXA | UString::ASCII, indent);
            data += dlength; size -= dlength;
        }
        // Private data
        if (size > 0) {
            strm << margin << "Private data:" << std::endl
                 << UString::Dump(data, size, UString::HEXA | UString::ASCII, indent);
            // Unused, end of function.
            // data += size; size = 0;
        }
    }
    else if (size > 0) {
        // Generic "id selector".
        strm << margin << "Data Broadcast Id selector:" << std::endl
             << UString::Dump(data, size, UString::HEXA | UString::ASCII, indent);
        // Unused, end of function.
        // data += size; size = 0;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"data_broadcast_id", data_broadcast_id, true);
    if (!private_data.empty()) {
        root->addElement(u"selector_bytes")->addHexaText(private_data);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::DataBroadcastIdDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint16_t>(data_broadcast_id, u"data_broadcast_id", true, 0, 0x0000, 0xFFFF) &&
        element->getHexaTextChild(private_data, u"selector_bytes", false, 0, MAX_DESCRIPTOR_SIZE - 2);
}
