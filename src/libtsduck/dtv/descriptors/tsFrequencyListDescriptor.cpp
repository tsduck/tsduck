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

#include "tsFrequencyListDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsBCD.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"frequency_list_descriptor"
#define MY_CLASS ts::FrequencyListDescriptor
#define MY_DID ts::DID_FREQUENCY_LIST
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::FrequencyListDescriptor::FrequencyListDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    coding_type(0),
    frequencies()
{
}

ts::FrequencyListDescriptor::FrequencyListDescriptor(DuckContext& duck, const Descriptor& desc) :
    FrequencyListDescriptor()
{
    deserialize(duck, desc);
}

void ts::FrequencyListDescriptor::clearContent()
{
    coding_type = 0;
    frequencies.clear();
}


//----------------------------------------------------------------------------
// Enumeration description of coding types.
//----------------------------------------------------------------------------

const ts::Enumeration ts::FrequencyListDescriptor::CodingTypeEnum({
    {u"undefined",   ts::FrequencyListDescriptor::UNDEFINED},
    {u"satellite",   ts::FrequencyListDescriptor::SATELLITE},
    {u"cable",       ts::FrequencyListDescriptor::CABLE},
    {u"terrestrial", ts::FrequencyListDescriptor::TERRESTRIAL},
});


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::FrequencyListDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(0xFC | coding_type);
    for (auto it = frequencies.begin(); it != frequencies.end(); ++it) {
        switch (coding_type) {
            case TERRESTRIAL: // binary coding in 10 Hz unit
                bbp->appendUInt32(uint32_t(*it / 10));
                break;
            case SATELLITE: // 8-digit BCD coding in 10 kHz units
                bbp->appendBCD(uint32_t(*it / 10000), 8);
                break;
            case CABLE:  // 8-digit BCD coding in 100 Hz units
                bbp->appendBCD(uint32_t(*it / 100), 8);
                break;
            case UNDEFINED: // assume binary coding in Hz.
            default:
                bbp->appendUInt32(uint32_t(*it));
                break;
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Decode a frequency at a 4-byte data area.
//----------------------------------------------------------------------------

uint64_t ts::FrequencyListDescriptor::DecodeFrequency(uint8_t coding_type, const uint8_t* data)
{
    switch (coding_type) {
        case TERRESTRIAL: // binary coding in 10 Hz unit
            return 10 * uint64_t(GetUInt32(data));
        case SATELLITE: // 8-digit BCD coding in 10 kHz units
            return 10000 * uint64_t(DecodeBCD(data, 8));
        case CABLE:  // 8-digit BCD coding in 100 Hz units
            return 100 * uint64_t(DecodeBCD(data, 8));
        case UNDEFINED: // assume binary coding in Hz.
        default:
            return GetUInt32(data);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::FrequencyListDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size % 4 == 1;
    frequencies.clear();

    if (_is_valid) {
        coding_type = data[0] & 0x03;
        data++; size--;
        frequencies.reserve(size / 4);
        while (size >= 4) {
            frequencies.push_back(DecodeFrequency(coding_type, data));
            data += 4; size -= 4;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::FrequencyListDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t type = data[0] & 0x03;
        data++; size--;
        strm << margin << UString::Format(u"Coding type: %d (%s)", {type, CodingTypeEnum.name(type)}) << std::endl;
        while (size >= 4) {
            strm << margin << UString::Format(u"Centre frequency: %'d Hz", {DecodeFrequency(type, data)}) << std::endl;
            data += 4; size -= 4;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::FrequencyListDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setEnumAttribute(CodingTypeEnum, u"coding_type", coding_type);
    for (auto it = frequencies.begin(); it != frequencies.end(); ++it) {
        root->addElement(u"centre_frequency")->setIntAttribute(u"value", *it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::FrequencyListDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntEnumAttribute(coding_type, CodingTypeEnum, u"coding_type", true) &&
        element->getChildren(children, u"centre_frequency", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint64_t freq = 0;
        ok = children[i]->getIntAttribute<uint64_t>(freq, u"value", true);
        frequencies.push_back(freq);
    }
    return ok;
}
