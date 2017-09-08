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
//  Representation of a local_time_offset_descriptor
//
//----------------------------------------------------------------------------

#include "tsLocalTimeOffsetDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsBCD.h"
#include "tsMJD.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::LocalTimeOffsetDescriptor, "local_time_offset_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::LocalTimeOffsetDescriptor, ts::EDID(ts::DID_LOCAL_TIME_OFFSET));
TS_ID_DESCRIPTOR_DISPLAY(ts::LocalTimeOffsetDescriptor::DisplayDescriptor, ts::EDID(ts::DID_LOCAL_TIME_OFFSET));


//----------------------------------------------------------------------------
// Default constructors
//----------------------------------------------------------------------------

ts::LocalTimeOffsetDescriptor::LocalTimeOffsetDescriptor() :
    AbstractDescriptor(DID_LOCAL_TIME_OFFSET, "local_time_offset_descriptor"),
    regions()
{
    _is_valid = true;
}

ts::LocalTimeOffsetDescriptor::Region::Region() :
    country(),
    region_id(0),
    time_offset(0),
    next_change(),
    next_time_offset(0)
{
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::LocalTimeOffsetDescriptor::LocalTimeOffsetDescriptor(const Descriptor& desc) :
    AbstractDescriptor(DID_LOCAL_TIME_OFFSET, "local_time_offset_descriptor"),
    regions()
{
    deserialize(desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::serialize(Descriptor& desc) const
{
    ByteBlockPtr bbp(new ByteBlock(2));
    CheckNonNull(bbp.pointer());

    for (RegionVector::const_iterator it = regions.begin(); it != regions.end(); ++it) {
        if (it->country.length() != 3) {
            desc.invalidate();
            return;
        }
        bbp->append(it->country);
        bbp->appendUInt8(uint8_t(it->region_id << 2) | 0x02 | (it->time_offset < 0 ? 0x01 : 0x00));
        bbp->appendUInt8(EncodeBCD(::abs(it->time_offset) / 60));
        bbp->appendUInt8(EncodeBCD(::abs(it->time_offset) % 60));
        EncodeMJD(it->next_change, reinterpret_cast<uint8_t*>(bbp->enlarge(MJD_SIZE)), MJD_SIZE);
        bbp->appendUInt8(EncodeBCD(::abs(it->next_time_offset) / 60));
        bbp->appendUInt8(EncodeBCD(::abs(it->next_time_offset) % 60));
    }

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d(bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::deserialize(const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() % 13 == 0;
    regions.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        while (size >= 13) {
            Region region;

            region.country = std::string(reinterpret_cast<const char*>(data), 3);
            region.region_id = data[3] >> 2;
            const uint8_t polarity = data[3] & 0x01;
            int hours = DecodeBCD(data[4]);
            int minutes = DecodeBCD(data[5]);
            region.time_offset = (polarity ? -1 : 1) * ((hours * 60) + minutes);
            DecodeMJD(data + 6, 5, region.next_change);
            hours = DecodeBCD(data[11]);
            minutes = DecodeBCD(data[12]);
            region.next_time_offset = (polarity ? -1 : 1) * ((hours * 60) + minutes);

            data += 13;
            size -= 13;

            regions.push_back(region);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 3) {
        // Country code is a 3-byte string
        strm << margin << "Country code: " << Printable(data, 3) << std::endl;
        data += 3; size -= 3;
        if (size >= 1) {
            uint8_t region_id = *data >> 2;
            uint8_t polarity = *data & 0x01;
            data += 1; size -= 1;
            strm << margin << "Region id: " << int(region_id)
                 << Format(" (0x%02X)", int(region_id))
                 << ", polarity: " << (polarity ? "west" : "east")
                 << " of Greenwich" << std::endl;
            if (size >= 2) {
                strm << margin << "Local time offset: " << (polarity ? "-" : "")
                     << Format("%02d:%02d", DecodeBCD(data[0]), DecodeBCD(data[1])) << std::endl;
                data += 2; size -= 2;
                if (size >= 5) {
                    Time next_change;
                    DecodeMJD(data, 5, next_change);
                    data += 5; size -= 5;
                    strm << margin << "Next change: " << next_change.format(Time::DATE | Time::TIME) << std::endl;
                    if (size >= 2) {
                        strm << margin << "Next time offset: " << (polarity ? "-" : "") 
                             << Format("%02d:%02d", DecodeBCD(data[0]), DecodeBCD(data[1]))
                             << std::endl;
                        data += 2; size -= 2;
                    }
                }
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::LocalTimeOffsetDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    for (RegionVector::const_iterator it = regions.begin(); it != regions.end(); ++it) {
        XML::Element* e = xml.addElement(root, "region");
        xml.setAttribute(e, "country_code", it->country);
        xml.setIntAttribute(e, "country_region_id", it->region_id);
        xml.setIntAttribute(e, "local_time_offset", it->time_offset);
        xml.setDateTimeAttribute(e, "time_of_change", it->next_change);
        xml.setIntAttribute(e, "next_time_offset", it->next_time_offset);
    }
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::LocalTimeOffsetDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    regions.clear();
    XML::ElementVector children;
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getChildren(children, element, "region");

    for (size_t index = 0; _is_valid && index < children.size(); ++index) {
        Region region;
        _is_valid =
            xml.getAttribute(region.country, children[index], "country_code", true, "", 3, 3) &&
            xml.getIntAttribute<unsigned int>(region.region_id, children[index], "country_region_id", true, 0, 0, 63) &&
            xml.getIntAttribute<int>(region.time_offset, children[index], "local_time_offset", true, 0, -780, 780) &&
            xml.getDateTimeAttribute(region.next_change, children[index], "time_of_change", true) &&
            xml.getIntAttribute<int>(region.next_time_offset, children[index], "next_time_offset", true, 0, -780, 780);
        if (_is_valid) {
            regions.push_back(region);
        }
    }
}
