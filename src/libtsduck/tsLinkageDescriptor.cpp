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
//  Representation of a generic linkage_descriptor.
//  Specialized classes exist, depending on the linkage type.
//
//----------------------------------------------------------------------------

#include "tsLinkageDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::LinkageDescriptor::LinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service, uint8_t ltype) :
    AbstractDescriptor(DID_LINKAGE),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    linkage_type(ltype),
    private_data()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::LinkageDescriptor::LinkageDescriptor(const Descriptor& desc) :
    AbstractDescriptor(DID_LINKAGE),
    ts_id(0),
    onetw_id(0),
    service_id(0),
    linkage_type(0),
    private_data()
{
    deserialize(desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::serialize(Descriptor& desc) const
{
    ByteBlockPtr bbp(new ByteBlock(2));
    CheckNonNull(bbp.pointer());

    bbp->appendUInt16(ts_id);
    bbp->appendUInt16(onetw_id);
    bbp->appendUInt16(service_id);
    bbp->appendUInt8(linkage_type);
    bbp->append(private_data);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d(bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::deserialize(const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 7;
    private_data.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        ts_id = GetUInt16 (data);
        onetw_id = GetUInt16 (data + 2);
        service_id = GetUInt16 (data + 4);
        linkage_type = data[6];
        private_data.copy (data + 7, size - 7);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 7) {

        // Fixed part
        uint16_t tsid = GetUInt16(data);
        uint16_t onid = GetUInt16(data + 2);
        uint16_t servid = GetUInt16(data + 4);
        uint8_t ltype = data[6];
        data += 7; size -= 7;
        strm << margin << "Transport stream id: " << tsid << Format(" (0x%04X)", int(tsid)) << std::endl
             << margin << "Original network Id: " << onid << Format(" (0x%04X)", int(onid)) << std::endl
             << margin << "Service id: " << servid << Format(" (0x%04X)", int(servid)) << std::endl
             << margin << "Linkage type: " << Format("0x%02X", int(ltype))
             << ", " << names::LinkageType(ltype) << std::endl;

        // Variable part
        if (ltype == 0x08 && size >= 1) {
            // Mobile hand-over
            uint8_t hand_over = *data >> 4;
            uint8_t origin = *data & 0x01;
            data += 1; size -= 1;
            const char *name;
            switch (hand_over) {
                case 0x01: name = "identical service in neighbour country"; break;
                case 0x02: name = "local variation of same service"; break;
                case 0x03: name = "associated service"; break;
                default:   name = "unknown"; break;
            }
            strm << margin << "Hand-over type: " << Format("0x%02X", int(hand_over))
                 << ", " << name << ", Origin: " << (origin ? "SDT" : "NIT") << std::endl;
            if ((hand_over == 0x01 || hand_over == 0x02 || hand_over == 0x03) && size >= 2) {
                uint16_t nwid = GetUInt16(data);
                data += 2; size -= 2;
                strm << margin << "Network id: " << nwid << Format(" (0x%04X)", int(nwid)) << std::endl;
            }
            if (origin == 0x00 && size >= 2) {
                uint16_t org_servid = GetUInt16(data);
                data += 2; size -= 2;
                strm << margin << "Original service id: " << org_servid << Format(" (0x%04X)", int(org_servid)) << std::endl;
            }
        }
        else if (ltype == 0x09 && size >= 1) {
            // System Software Update (ETSI TS 102 006)
            uint8_t dlength = data[0];
            data += 1; size -= 1;
            if (dlength > size) {
                dlength = uint8_t(size);
            }
            while (dlength >= 4) {
                uint32_t oui = GetUInt32(data - 1) & 0x00FFFFFF; // 24 bits
                uint8_t slength = data[3];
                data += 4; size -= 4; dlength -= 4;
                const uint8_t* sdata = data;
                if (slength > dlength) {
                    slength = dlength;
                }
                data += slength; size -= slength; dlength -= slength;
                strm << margin << Format("OUI: 0x%06X (", int(oui)) << names::OUI(oui) << ")" << std::endl;
                if (slength > 0) {
                    strm << margin << "Selector data:" << std::endl
                         << Hexa(sdata, slength, hexa::HEXA | hexa::ASCII, indent);
                }
            }
        }
        else if (ltype == 0x0A && size >= 1) {
            // TS with System Software Update BAT or NIT (ETSI TS 102 006)
            uint8_t ttype = data[0];
            data += 1; size -= 1;
            strm << margin << "SSU table type: ";
            switch (ttype) {
                case 0x01: strm << "NIT"; break;
                case 0x02: strm << "BAT"; break;
                default:   strm << Format("0x%02x", int(ttype)); break;
            }
            strm << std::endl;
        }

        // Remaining private data
        if (size > 0) {
            strm << margin << "Private data:" << std::endl
                 << Hexa(data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
            data += size; size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::LinkageDescriptor::toXML(XML& xml, XML::Document& doc) const
{
    return 0; // TODO @@@@
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    // TODO @@@@
}
