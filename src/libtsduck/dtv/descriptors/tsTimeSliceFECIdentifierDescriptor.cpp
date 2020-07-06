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

#include "tsTimeSliceFECIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"time_slice_fec_identifier_descriptor"
#define MY_CLASS ts::TimeSliceFECIdentifierDescriptor
#define MY_DID ts::DID_TIME_SLICE_FEC_ID
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TimeSliceFECIdentifierDescriptor::TimeSliceFECIdentifierDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    time_slicing(false),
    mpe_fec(0),
    frame_size(0),
    max_burst_duration(0),
    max_average_rate(0),
    time_slice_fec_id(0),
    id_selector_bytes()
{
}

void ts::TimeSliceFECIdentifierDescriptor::clearContent()
{
    time_slicing = false;
    mpe_fec = 0;
    frame_size = 0;
    max_burst_duration = 0;
    max_average_rate = 0;
    time_slice_fec_id = 0;
    id_selector_bytes.clear();
}

ts::TimeSliceFECIdentifierDescriptor::TimeSliceFECIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    TimeSliceFECIdentifierDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TimeSliceFECIdentifierDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((time_slicing ? 0x80 : 0x00) |
                     uint8_t((mpe_fec & 0x03) << 5) |
                     0x18 |
                     (frame_size & 0x07));
    bbp->appendUInt8(max_burst_duration);
    bbp->appendUInt8(uint8_t((max_average_rate & 0x0F) << 4) |
                     (time_slice_fec_id & 0x0F));
    bbp->append(id_selector_bytes);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TimeSliceFECIdentifierDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    id_selector_bytes.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 3;

    if (_is_valid) {
        time_slicing = (data[0] & 0x80) != 0;
        mpe_fec = (data[0] >> 5) & 0x03;
        frame_size = data[0] & 0x07;
        max_burst_duration = data[1];
        max_average_rate = (data[2] >> 4) & 0x0F;
        time_slice_fec_id = data[2] & 0x0F;
        id_selector_bytes.copy(data + 3, size - 3);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TimeSliceFECIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"time_slicing", time_slicing);
    root->setIntAttribute(u"mpe_fec", mpe_fec, true);
    root->setIntAttribute(u"frame_size", frame_size, true);
    root->setIntAttribute(u"max_burst_duration", max_burst_duration, true);
    root->setIntAttribute(u"max_average_rate", max_average_rate, true);
    root->setIntAttribute(u"time_slice_fec_id", time_slice_fec_id, true);
    root->addHexaTextChild(u"id_selector_bytes", id_selector_bytes, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TimeSliceFECIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getBoolAttribute(time_slicing, u"time_slicing", true) &&
            element->getIntAttribute<uint8_t>(mpe_fec, u"mpe_fec", true, 0, 0, 0x03) &&
            element->getIntAttribute<uint8_t>(frame_size, u"frame_size", true, 0, 0x00, 0x07) &&
            element->getIntAttribute<uint8_t>(max_burst_duration, u"max_burst_duration", true) &&
            element->getIntAttribute<uint8_t>(max_average_rate, u"max_average_rate", true, 0, 0x00, 0x0F) &&
            element->getIntAttribute<uint8_t>(time_slice_fec_id, u"time_slice_fec_id", false, 0, 0x00, 0x0F) &&
            element->getHexaTextChild(id_selector_bytes, u"id_selector_bytes", false, 0, MAX_DESCRIPTOR_SIZE - 5);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TimeSliceFECIdentifierDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 3) {
        const bool time_slicing = (data[0] & 0x80) != 0;
        const uint8_t mpe_fec = (data[0] >> 5) & 0x03;
        const uint8_t frame_size = data[0] & 0x07;
        const uint8_t max_burst_duration = data[1];
        const uint8_t max_average_rate = (data[2] >> 4) & 0x0F;
        const uint8_t time_slice_fec_id = data[2] & 0x0F;
        data += 3; size -= 3;

        strm << margin << "Use time slice: " << UString::TrueFalse(time_slicing) << std::endl
             << margin << "MPE FEC: ";
        switch (mpe_fec) {
            case 0:  strm << "none"; break;
            case 1:  strm << "Reed-Solomon(255, 191, 64)"; break;
            default: strm << UString::Format(u"reserved value 0x%X", {mpe_fec}); break;
        }
        strm << std::endl << margin << "Frame size: ";
        switch (frame_size) {
            case 0:  strm << "512 kbits, 256 rows"; break;
            case 1:  strm << "1024 kbits, 512 rows"; break;
            case 2:  strm << "1536 kbits, 768 rows"; break;
            case 3:  strm << "2048 kbits, 1024 rows"; break;
            default: strm << UString::Format(u"reserved value 0x%X", {frame_size}); break;
        }
        strm << std::endl
             << margin << UString::Format(u"Max burst duration: 0x%X (%d)", {max_burst_duration, max_burst_duration}) << std::endl
             << margin << "Max average rate: ";
        switch (max_average_rate) {
            case 0:  strm << "16 kbps"; break;
            case 1:  strm << "32 kbps"; break;
            case 2:  strm << "64 kbps"; break;
            case 3:  strm << "128 kbps"; break;
            case 4:  strm << "256 kbps"; break;
            case 5:  strm << "512 kbps"; break;
            case 6:  strm << "1024 kbps"; break;
            case 7:  strm << "2048 kbps"; break;
            default: strm << UString::Format(u"reserved value 0x%X", {max_average_rate}); break;
        }
        strm << std::endl
             << margin << UString::Format(u"Time slice FEC id: 0x%X (%d)", {time_slice_fec_id, time_slice_fec_id}) << std::endl;
        display.displayPrivateData(u"Id selector bytes", data, size, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}
