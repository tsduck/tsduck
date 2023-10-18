//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNPTReferenceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"NPT_reference_descriptor"
#define MY_CLASS ts::NPTReferenceDescriptor
#define MY_DID ts::DID_NPT_REFERENCE
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NPTReferenceDescriptor::NPTReferenceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::NPTReferenceDescriptor::NPTReferenceDescriptor(DuckContext& duck, const Descriptor& desc) :
    NPTReferenceDescriptor()
{
    deserialize(duck, desc);
}

void ts::NPTReferenceDescriptor::clearContent()
{
    post_discontinuity = false;
    content_id = 0;
    STC_reference = 0;
    NPT_reference = 0;
    scale_numerator = 0;
    scale_denominator = 0;
}


//----------------------------------------------------------------------------
// Recompute the NPT/STC scale using another NPT_reference_descriptor.
//----------------------------------------------------------------------------

void ts::NPTReferenceDescriptor::computeScale(const ts::NPTReferenceDescriptor& other_reference, bool force)
{
    // See ISO/IEC 13818-6, 8.1.2.
    if (force || scale_numerator == 0 || scale_denominator == 0) {
        if (NPT_reference > other_reference.NPT_reference) {
            scale_numerator = uint16_t(NPT_reference - other_reference.NPT_reference);
            scale_denominator = uint16_t(STC_reference - other_reference.STC_reference);
        }
        else {
            scale_numerator = uint16_t(other_reference.NPT_reference - NPT_reference);
            scale_denominator = uint16_t(other_reference.STC_reference - STC_reference);
        }
    }
}


//----------------------------------------------------------------------------
// Time stamp conversions
//----------------------------------------------------------------------------

uint64_t ts::NPTReferenceDescriptor::nptToPCR(uint64_t npt) const
{
    return nptToSTC(npt) * SYSTEM_CLOCK_SUBFACTOR;
}

uint64_t ts::NPTReferenceDescriptor::pcrToNPT(uint64_t pcr) const
{
    return stcToNPT(pcr / SYSTEM_CLOCK_SUBFACTOR);
}

uint64_t ts::NPTReferenceDescriptor::stcToNPT(uint64_t stc) const
{
    // See ISO/IEC 13818-6, 8.1.1 and 8.1.2.
    return scale_denominator == 0 ? 0 : NPT_reference + ((scale_numerator * (stc - STC_reference)) / scale_denominator);
}

uint64_t ts::NPTReferenceDescriptor::nptToSTC(uint64_t npt) const
{
    // See stcToNPT()
    return scale_numerator == 0 ? 0 : STC_reference + ((scale_denominator * (npt - NPT_reference)) / scale_numerator);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NPTReferenceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(post_discontinuity);
    buf.putBits(content_id, 7);
    buf.putBits(0xFF, 7);
    buf.putBits(STC_reference, 33);
    buf.putBits(0xFFFFFFFF, 31);
    buf.putBits(NPT_reference, 33);
    buf.putUInt16(scale_numerator);
    buf.putUInt16(scale_denominator);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NPTReferenceDescriptor::deserializePayload(PSIBuffer& buf)
{
    post_discontinuity = buf.getBool();
    buf.getBits(content_id, 7);
    buf.skipBits(7);
    buf.getBits(STC_reference, 33);
    buf.skipBits(31);
    buf.getBits(NPT_reference, 33);
    scale_numerator = buf.getUInt16();
    scale_denominator = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NPTReferenceDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(18)) {
        disp << margin << "Post discontinuity: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << UString::Format(u"Content id: 0x%X (%<d)", {buf.getBits<uint8_t>(7)}) << std::endl;
        buf.skipBits(7);
        disp << margin << UString::Format(u"STC reference: 0x%09X (%<d)", {buf.getBits<uint64_t>(33)}) << std::endl;
        buf.skipBits(31);
        disp << margin << UString::Format(u"NPT reference: 0x%09X (%<d)", {buf.getBits<uint64_t>(33)}) << std::endl;
        disp << margin << UString::Format(u"NPT/STC scale: %d", {buf.getUInt16()});
        disp << UString::Format(u"/%d", {buf.getUInt16()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NPTReferenceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"post_discontinuity", post_discontinuity);
    root->setIntAttribute(u"content_id", content_id, true);
    root->setIntAttribute(u"STC_reference", STC_reference, true);
    root->setIntAttribute(u"NPT_reference", NPT_reference, true);
    root->setIntAttribute(u"scale_numerator", scale_numerator, false);
    root->setIntAttribute(u"scale_denominator", scale_denominator, false);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NPTReferenceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(post_discontinuity, u"post_discontinuity", false, false) &&
           element->getIntAttribute(content_id, u"content_id", false, 0x7F, 0x00, 0x7F) &&
           element->getIntAttribute(STC_reference, u"STC_reference", true, 0, 0, 0x00000001FFFFFFFF) &&
           element->getIntAttribute(NPT_reference, u"NPT_reference", true, 0, 0, 0x00000001FFFFFFFF) &&
           element->getIntAttribute(scale_numerator, u"scale_numerator", true) &&
           element->getIntAttribute(scale_denominator, u"scale_denominator", true);
}
