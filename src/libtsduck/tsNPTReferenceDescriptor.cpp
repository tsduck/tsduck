//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsNPTReferenceDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"NPT_reference_descriptor"
#define MY_DID ts::DID_NPT_REFERENCE

TS_XML_DESCRIPTOR_FACTORY(ts::NPTReferenceDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::NPTReferenceDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::NPTReferenceDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::NPTReferenceDescriptor::NPTReferenceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    post_discontinuity(false),
    content_id(0),
    STC_reference(0),
    NPT_reference(0),
    scale_numerator(0),
    scale_denominator(0)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::NPTReferenceDescriptor::NPTReferenceDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    NPTReferenceDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Recompute the NPT/STC scale using another NPT_reference_descriptor.
//----------------------------------------------------------------------------

void ts::NPTReferenceDescriptor::computeScale(const ts::NPTReferenceDescriptor& other_reference, bool force)
{
    // See ISO/IEC 13818-6, 8.1.2.
    if (force || scale_numerator == 0 || scale_denominator == 0) {
        if (NPT_reference > other_reference.NPT_reference) {
            scale_numerator = NPT_reference - other_reference.NPT_reference;
            scale_denominator = STC_reference - other_reference.STC_reference;
        }
        else {
            scale_numerator = other_reference.NPT_reference - NPT_reference;
            scale_denominator = other_reference.STC_reference - STC_reference;
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

void ts::NPTReferenceDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((post_discontinuity ? 0x80 : 0x00) | (content_id & 0x7F));
    bbp->appendUInt40(TS_UCONST64(0x000000FE00000000) | STC_reference);
    bbp->appendUInt64(TS_UCONST64(0xFFFFFFFE00000000) | NPT_reference);
    bbp->appendUInt16(scale_numerator);
    bbp->appendUInt16(scale_denominator);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NPTReferenceDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() == 18;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        post_discontinuity = (data[0] & 0x80) != 0;
        content_id = data[0] & 0x7F;
        STC_reference = GetUInt40(data + 1) & TS_UCONST64(0x00000001FFFFFFFF);
        NPT_reference = GetUInt64(data + 6) & TS_UCONST64(0x00000001FFFFFFFF);
        scale_numerator = GetUInt16(data + 14);
        scale_denominator = GetUInt16(data + 16);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NPTReferenceDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 18) {
        const uint64_t stc = GetUInt40(data + 1) & TS_UCONST64(0x00000001FFFFFFFF);
        const uint64_t npt = GetUInt64(data + 6) & TS_UCONST64(0x00000001FFFFFFFF);
        strm << margin << "Post discontinuity: " << UString::TrueFalse((data[0] & 0x80) != 0) << std::endl
             << margin << UString::Format(u"Content id: 0x%X (%d)", {data[0] & 0x7F, data[0] & 0x7F}) << std::endl
             << margin << UString::Format(u"STC reference: 0x%09X (%d)", {stc, stc}) << std::endl
             << margin << UString::Format(u"NPT reference: 0x%09X (%d)", {npt, npt}) << std::endl
             << margin << UString::Format(u"NPT/STC scale: %d/%d", {GetUInt16(data + 14), GetUInt16(data + 16)}) << std::endl;
        data += 18; size -= 18;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NPTReferenceDescriptor::buildXML(xml::Element* root) const
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

void ts::NPTReferenceDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getBoolAttribute(post_discontinuity, u"post_discontinuity", false, false) &&
        element->getIntAttribute<uint8_t>(content_id, u"content_id", false, 0x7F, 0x00, 0x7F) &&
        element->getIntAttribute<uint64_t>(STC_reference, u"STC_reference", true, 0, 0, TS_UCONST64(0x00000001FFFFFFFF)) &&
        element->getIntAttribute<uint64_t>(NPT_reference, u"NPT_reference", true, 0, 0, TS_UCONST64(0x00000001FFFFFFFF)) &&
        element->getIntAttribute<uint16_t>(scale_numerator, u"scale_numerator", true) &&
        element->getIntAttribute<uint16_t>(scale_denominator, u"scale_denominator", true);
}
