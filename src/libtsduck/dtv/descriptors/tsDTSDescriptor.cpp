//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTSDescriptor.h"
#include "tsDescriptor.h"
#include "tsNamesFile.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DTS_descriptor"
#define MY_CLASS ts::DTSDescriptor
#define MY_DID ts::DID_DTS
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTSDescriptor::DTSDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::DTSDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(5)) {
        disp << margin << "Sample rate code: " << DataName(MY_XML_NAME, u"SampleRate", buf.getBits<uint8_t>(4)) << std::endl;
        disp << margin << "Bit rate code: " << DataName(MY_XML_NAME, u"BitRate", buf.getBits<uint8_t>(6)) << std::endl;
        disp << margin << "NBLKS: " << buf.getBits<uint16_t>(7) << std::endl;
        disp << margin << "FSIZE: " << buf.getBits<uint16_t>(14) << std::endl;
        disp << margin << "Surround mode: " << DataName(MY_XML_NAME, u"SurroundMode", buf.getBits<uint8_t>(6)) << std::endl;
        disp << margin << "LFE (Low Frequency Effect) audio channel: " << UString::OnOff(buf.getBool()) << std::endl;
        disp << margin << "Extended surround flag: " << DataName(MY_XML_NAME, u"ExtendedSurroundMode", buf.getBits<uint8_t>(2)) << std::endl;
        disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
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
