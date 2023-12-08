//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTimeSliceFECIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"time_slice_fec_identifier_descriptor"
#define MY_CLASS ts::TimeSliceFECIdentifierDescriptor
#define MY_DID ts::DID_TIME_SLICE_FEC_ID
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TimeSliceFECIdentifierDescriptor::TimeSliceFECIdentifierDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::TimeSliceFECIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(time_slicing);
    buf.putBits(mpe_fec, 2);
    buf.putBits(0xFF, 2);
    buf.putBits(frame_size, 3);
    buf.putUInt8(max_burst_duration);
    buf.putBits(max_average_rate, 4);
    buf.putBits(time_slice_fec_id, 4);
    buf.putBytes(id_selector_bytes);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TimeSliceFECIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    time_slicing = buf.getBool();
    buf.getBits(mpe_fec, 2);
    buf.skipBits(2);
    buf.getBits(frame_size, 3);
    max_burst_duration = buf.getUInt8();
    buf.getBits(max_average_rate, 4);
    buf.getBits(time_slice_fec_id, 4);
    buf.getBytes(id_selector_bytes);
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
            element->getIntAttribute(mpe_fec, u"mpe_fec", true, 0, 0, 0x03) &&
            element->getIntAttribute(frame_size, u"frame_size", true, 0, 0x00, 0x07) &&
            element->getIntAttribute(max_burst_duration, u"max_burst_duration", true) &&
            element->getIntAttribute(max_average_rate, u"max_average_rate", true, 0, 0x00, 0x0F) &&
            element->getIntAttribute(time_slice_fec_id, u"time_slice_fec_id", false, 0, 0x00, 0x0F) &&
            element->getHexaTextChild(id_selector_bytes, u"id_selector_bytes", false, 0, MAX_DESCRIPTOR_SIZE - 5);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TimeSliceFECIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "Use time slice: " << UString::TrueFalse(buf.getBool()) << std::endl;
        const uint8_t mpe_fec = buf.getBits<uint8_t>(2);
        disp << margin << "MPE FEC: ";
        switch (mpe_fec) {
            case 0:  disp << "none"; break;
            case 1:  disp << "Reed-Solomon(255, 191, 64)"; break;
            default: disp << UString::Format(u"reserved value 0x%X", {mpe_fec}); break;
        }
        disp << std::endl;
        buf.skipBits(2);
        const uint8_t frame_size = buf.getBits<uint8_t>(3);
        disp << margin << "Frame size: ";
        switch (frame_size) {
            case 0:  disp << "512 kbits, 256 rows"; break;
            case 1:  disp << "1024 kbits, 512 rows"; break;
            case 2:  disp << "1536 kbits, 768 rows"; break;
            case 3:  disp << "2048 kbits, 1024 rows"; break;
            default: disp << UString::Format(u"reserved value 0x%X", {frame_size}); break;
        }
        disp << std::endl;
        disp << margin << UString::Format(u"Max burst duration: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        const uint8_t max_average_rate = buf.getBits<uint8_t>(4);
        disp << margin << "Max average rate: ";
        switch (max_average_rate) {
            case 0:  disp << "16 kbps"; break;
            case 1:  disp << "32 kbps"; break;
            case 2:  disp << "64 kbps"; break;
            case 3:  disp << "128 kbps"; break;
            case 4:  disp << "256 kbps"; break;
            case 5:  disp << "512 kbps"; break;
            case 6:  disp << "1024 kbps"; break;
            case 7:  disp << "2048 kbps"; break;
            default: disp << UString::Format(u"reserved value 0x%X", {max_average_rate}); break;
        }
        disp << std::endl;
        disp << margin << UString::Format(u"Time slice FEC id: 0x%X (%<d)", {buf.getBits<uint8_t>(4)}) << std::endl;
        disp.displayPrivateData(u"Id selector bytes", buf, NPOS, margin);
    }
}
