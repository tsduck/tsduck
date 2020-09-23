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

#include "tsDTSDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"DTS_descriptor"
#define MY_CLASS ts::DTSDescriptor
#define MY_DID ts::DID_DTS
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTSDescriptor::DTSDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    sample_rate_code(0),
    bit_rate_code(0),
    nblks(0),
    fsize(0),
    surround_mode(0),
    lfe(false),
    extended_surround(0),
    additional_info()
{
}

void ts::DTSDescriptor::clearContent()
{
    sample_rate_code = 0;
    bit_rate_code = 0;
    nblks = 0;
    fsize = 0;
    surround_mode = 0;
    lfe = false;
    extended_surround = 0;
    additional_info.clear();
}

ts::DTSDescriptor::DTSDescriptor(DuckContext& duck, const Descriptor& desc) :
    DTSDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DTSDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(sample_rate_code, 4);
    buf.putBits(bit_rate_code, 6);
    buf.putBits(nblks, 7);
    buf.putBits(fsize, 14);
    buf.putBits(surround_mode, 6);
    buf.putBit(lfe);
    buf.putBits(extended_surround, 2);
    buf.putBytes(additional_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DTSDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(sample_rate_code, 4);
    buf.getBits(bit_rate_code, 6);
    buf.getBits(nblks, 7);
    buf.getBits(fsize, 14);
    buf.getBits(surround_mode, 6);
    lfe = buf.getBool();
    buf.getBits(extended_surround, 2);
    buf.getBytes(additional_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTSDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size >= 5) {
        uint8_t sample_rate_code = (data[0] >> 4) & 0x0F;
        uint8_t bit_rate_code = (GetUInt16(data) >> 6) & 0x3F;
        uint8_t nblks = (GetUInt16(data + 1) >> 7) & 0x7F;
        uint16_t fsize = (GetUInt16(data + 2) >> 1) & 0x3FFF;
        uint8_t surround_mode = (GetUInt16(data + 3) >> 3) & 0x3F;
        bool lfe_flag = ((data[4] >> 2) & 0x01) != 0;
        uint8_t extended_surround_flag = data[4] & 0x03;
        data += 5; size -= 5;

        disp << margin << "Sample rate code: " << names::DTSSampleRateCode(sample_rate_code) << std::endl
             << margin << "Bit rate code: " << names::DTSBitRateCode(bit_rate_code) << std::endl
             << margin << "NBLKS: " << int(nblks) << std::endl
             << margin << "FSIZE: " << int(fsize) << std::endl
             << margin << "Surround mode: " << names::DTSSurroundMode(surround_mode) << std::endl
             << margin << "LFE (Low Frequency Effect) audio channel: " << UString::OnOff(lfe_flag) << std::endl
             << margin << "Extended surround flag: " << names::DTSExtendedSurroundMode(extended_surround_flag) << std::endl;

        disp.displayPrivateData(u"Additional information", data, size, margin);
    }
    else {
        disp.displayExtraData(data, size, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DTSDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"sample_rate_code", sample_rate_code, true);
    root->setIntAttribute(u"bit_rate_code", bit_rate_code, true);
    root->setIntAttribute(u"nblks", nblks, true);
    root->setIntAttribute(u"fsize", fsize, true);
    root->setIntAttribute(u"surround_mode", surround_mode, true);
    root->setBoolAttribute(u"lfe", lfe);
    root->setIntAttribute(u"extended_surround", extended_surround, true);
    root->addHexaTextChild(u"additional_info", additional_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DTSDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute(sample_rate_code, u"sample_rate_code", true, 0x00, 0x00, 0x0F) &&
            element->getIntAttribute(bit_rate_code, u"bit_rate_code", true, 0x00, 0x00, 0x3F) &&
            element->getIntAttribute(nblks, u"nblks", true, 0x00, 0x05, 0x7F) &&
            element->getIntAttribute(fsize, u"fsize", true, 0x0000, 0x005F, 0x2000) &&
            element->getIntAttribute(surround_mode, u"surround_mode", true, 0x00, 0x00, 0x3F) &&
            element->getBoolAttribute(lfe, u"lfe", false, false) &&
            element->getIntAttribute(extended_surround, u"extended_surround", false, 0x00, 0x00, 0x03) &&
            element->getHexaTextChild(additional_info, u"additional_info", false, 0, MAX_DESCRIPTOR_SIZE - 7);
}
