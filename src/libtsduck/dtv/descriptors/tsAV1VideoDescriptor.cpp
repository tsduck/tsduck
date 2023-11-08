//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAV1VideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AV1_video_descriptor"
#define MY_CLASS ts::AV1VideoDescriptor
#define MY_DID ts::DID_AV1_VIDEO
#define MY_PDS ts::PDS_AOM
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AV1VideoDescriptor::AV1VideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

void ts::AV1VideoDescriptor::clearContent()
{
    version = 0;
    seq_profile = 0;
    seq_level_idx_0 = 0;
    seq_tier_0 = 0;
    high_bitdepth = false;
    twelve_bit = false;
    monochrome = false;
    chroma_subsampling_x = false;
    chroma_subsampling_y = false;
    chroma_sample_position = 0;
    HDR_WCG_idc = 0;
    initial_presentation_delay_minus_one.reset();
}

ts::AV1VideoDescriptor::AV1VideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    AV1VideoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AV1VideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(1);
    buf.putBits(version, 7);
    buf.putBits(seq_profile, 3);
    buf.putBits(seq_level_idx_0, 5);
    buf.putBits(seq_tier_0, 1);
    buf.putBit(high_bitdepth);
    buf.putBit(twelve_bit);
    buf.putBit(monochrome);
    buf.putBit(chroma_subsampling_x);
    buf.putBit(chroma_subsampling_y);
    buf.putBits(chroma_sample_position, 2);
    buf.putBits(HDR_WCG_idc, 2);
    buf.putBit(0);
    buf.putBit(initial_presentation_delay_minus_one.has_value());
    if (initial_presentation_delay_minus_one.has_value()) {
        buf.putBits(initial_presentation_delay_minus_one.value(), 4);
    }
    else {
        buf.putBits(0, 4);
    }

}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AV1VideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(1);
    buf.getBits(version, 7);
    buf.getBits(seq_profile, 3);
    buf.getBits(seq_level_idx_0, 5);
    buf.getBits(seq_tier_0, 1);
    high_bitdepth = buf.getBool();
    twelve_bit = buf.getBool();
    monochrome = buf.getBool();
    chroma_subsampling_x = buf.getBool();
    chroma_subsampling_y = buf.getBool();
    buf.getBits(chroma_sample_position, 2);
    buf.getBits(HDR_WCG_idc, 2);
    buf.skipBits(1);
    bool _initial_presentation_delay_present = buf.getBool();
    if (_initial_presentation_delay_present) {
        buf.getBits(initial_presentation_delay_minus_one, 4);
    }
    else {
        buf.skipBits(4);
    }
}


//----------------------------------------------------------------------------
// Display subsamplig format from signalled attributes
//----------------------------------------------------------------------------

ts::UString ts::AV1VideoDescriptor::SubsamplingFormat(bool subsampling_x, bool subsampling_y, bool monochrome)
{
    UString res(u"invalid");
    if (monochrome && subsampling_x && subsampling_y) {
        res = u"Monochrome 4:0:0";
    }
    else if (!monochrome && subsampling_x && subsampling_y) {
        res = u"YUV 4:2:0";
    }
    else if (!monochrome && subsampling_x && !subsampling_y) {
        res = u"YUV 4:2:2";
    }
    else if (!monochrome && !subsampling_x && !subsampling_y) {
        res = u"YUV 4:4:4";
    }
    return res;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AV1VideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        buf.skipReservedBits(1);
        disp << margin << "Version: " << int(buf.getBits<uint8_t>(7));
        disp << ", profile: " << int(buf.getBits<uint8_t>(3));
        disp << ", level: " << DataName(MY_XML_NAME, u"seq_level_idx", buf.getBits<uint8_t>(5), NamesFlags::VALUE | NamesFlags::DECIMAL);
        disp << ", tier: " << int(buf.getBit()) << std::endl;
        disp << margin << "High bitdepth: " << UString::YesNo(buf.getBit());
        disp << ", 12 bit: " << UString::YesNo(buf.getBit());
        bool monochrome = buf.getBit();
        bool subsampling_x = buf.getBit();
        bool subsampling_y = buf.getBit();
        disp << ", monochrome: " << UString::YesNo(monochrome) << ", chroma subsampling x=" << UString::YesNo(subsampling_x) << " y=" << UString::YesNo(subsampling_y);
        disp << ", --> " << SubsamplingFormat(subsampling_x, subsampling_y, monochrome) << std::endl;
        disp << margin << "Chroma sample position: " << DataName(MY_XML_NAME, u"chroma_sample_position", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL);
        disp << ", HDR WCG idc: " << DataName(MY_XML_NAME, u"hdr_wcg_idc", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        buf.skipReservedBits(1, 0);
        bool _initial_presentation_delay_present = buf.getBool();
        if (_initial_presentation_delay_present) {
            uint8_t ipd = buf.getBits<uint8_t>(4);
            disp << margin << UString::Format(u"Initial presentation delay %d (minus1=%d)", {ipd+1, ipd}) << std::endl;
        }
        else {
            buf.skipReservedBits(4, 0);
        }
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

const ts::Enumeration ts::AV1VideoDescriptor::ChromaSamplePosition({
    {u"unknown", 0},
    {u"vertical", 1},
    {u"colocated", 2},
});


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AV1VideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"seq_profile", seq_profile);
    root->setIntAttribute(u"seq_level_idx_0", seq_level_idx_0);
    root->setIntAttribute(u"seq_tier_0", seq_tier_0);
    root->setBoolAttribute(u"high_bitdepth", high_bitdepth);
    root->setBoolAttribute(u"twelve_bit", twelve_bit);
    root->setBoolAttribute(u"monochrome", monochrome);
    root->setBoolAttribute(u"chroma_subsampling_x", chroma_subsampling_x);
    root->setBoolAttribute(u"chroma_subsampling_y", chroma_subsampling_y);
    root->setEnumAttribute(ChromaSamplePosition, u"chroma_sample_position", chroma_sample_position);
    root->setIntAttribute(u"HDR_WCG_idc", HDR_WCG_idc);
    root->setOptionalIntAttribute(u"initial_presentation_delay_minus_one", initial_presentation_delay_minus_one);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AV1VideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    int csp = 99;
    bool ok =
        element->getIntAttribute(version, u"version", true, 1, 1, 1) &&
        element->getIntAttribute(seq_profile, u"seq_profile", true, 0, 0x00, 0x7) &&
        element->getIntAttribute(seq_level_idx_0, u"seq_level_idx_0", true, 0, 0x00, 0x1F) &&
        element->getIntAttribute(seq_tier_0, u"seq_tier_0", true, 0, 0, 1) &&
        element->getBoolAttribute(high_bitdepth, u"high_bitdepth", true) &&
        element->getBoolAttribute(twelve_bit, u"twelve_bit", true) &&
        element->getBoolAttribute(monochrome, u"monochrome", true) &&
        element->getBoolAttribute(chroma_subsampling_x, u"chroma_subsampling_x", true) &&
        element->getBoolAttribute(chroma_subsampling_y, u"chroma_subsampling_y", true) &&
        element->getEnumAttribute(csp, ChromaSamplePosition, u"chroma_sample_position", true, 0) &&
        element->getIntAttribute(HDR_WCG_idc, u"HDR_WCG_idc", true, 3, 0, 3) &&
        element->getOptionalIntAttribute(initial_presentation_delay_minus_one, u"initial_presentation_delay_minus_one", 0, 0xF);
    chroma_sample_position = uint8_t(csp);
    return ok;
}
