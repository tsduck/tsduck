//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsATSCEAC3AudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_EAC3_audio_descriptor"
#define MY_CLASS ts::ATSCEAC3AudioDescriptor
#define MY_DID ts::DID_ATSC_ENHANCED_AC3
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCEAC3AudioDescriptor::ATSCEAC3AudioDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    mixinfoexists(false),
    full_service(false),
    audio_service_type(0),
    number_of_channels(0),
    bsid(),
    priority(),
    mainid(),
    asvc(),
    substream1(),
    substream2(),
    substream3(),
    language(),
    language_2(),
    substream1_lang(),
    substream2_lang(),
    substream3_lang(),
    additional_info()
{
}

ts::ATSCEAC3AudioDescriptor::ATSCEAC3AudioDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCEAC3AudioDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::ATSCEAC3AudioDescriptor::clearContent()
{
    mixinfoexists = false;
    full_service = false;
    audio_service_type = 0;
    number_of_channels = 0;
    bsid.clear();
    priority.clear();
    mainid.clear();
    asvc.clear();
    substream1.clear();
    substream2.clear();
    substream3.clear();
    language.clear();
    language_2.clear();
    substream1_lang.clear();
    substream2_lang.clear();
    substream3_lang.clear();
    additional_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ATSCEAC3AudioDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(1);
    buf.putBit(bsid.set());
    buf.putBit(mainid.set() && priority.set());
    buf.putBit(asvc.set());
    buf.putBit(mixinfoexists);
    buf.putBit(substream1.set());
    buf.putBit(substream2.set());
    buf.putBit(substream3.set());
    buf.putBit(1);
    buf.putBit(full_service);
    buf.putBits(audio_service_type, 3);
    buf.putBits(number_of_channels, 3);
    buf.putBit(!language.empty());
    buf.putBit(!language_2.empty());
    buf.putBit(1);
    buf.putBits(bsid.value(0x00), 5);
    if (mainid.set() && priority.set()) {
        buf.putBits(0xFF, 3);
        buf.putBits(priority.value(), 2);
        buf.putBits(mainid.value(), 3);
    }
    if (asvc.set()) {
        buf.putUInt8(asvc.value());
    }
    if (substream1.set()) {
        buf.putUInt8(substream1.value());
    }
    if (substream2.set()) {
        buf.putUInt8(substream2.value());
    }
    if (substream3.set()) {
        buf.putUInt8(substream3.value());
    }
    if (!language.empty()) {
        buf.putLanguageCode(language);
    }
    if (!language_2.empty()) {
        buf.putLanguageCode(language_2);
    }
    if (substream1.set()) {
        buf.putLanguageCode(substream1_lang);
    }
    if (substream2.set()) {
        buf.putLanguageCode(substream2_lang);
    }
    if (substream3.set()) {
        buf.putLanguageCode(substream3_lang);
    }
    buf.putBytes(additional_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ATSCEAC3AudioDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(1);
    const bool bsid_flag = buf.getBool();
    const bool mainid_flag = buf.getBool();
    const bool asvc_flag = buf.getBool();
    mixinfoexists = buf.getBool();
    const bool substream1_flag = buf.getBool();
    const bool substream2_flag = buf.getBool();
    const bool substream3_flag = buf.getBool();
    buf.skipBits(1);
    full_service = buf.getBool();
    buf.getBits(audio_service_type, 3);
    buf.getBits(number_of_channels, 3);

    // End of descriptor allowed here
    if (buf.endOfRead()) {
        return;
    }

    // Decode one byte depending on bsid.
    const bool language_flag = buf.getBool();
    const bool language_2_flag = buf.getBool();
    buf.skipBits(1);
    if (bsid_flag) {
        buf.getBits(bsid, 5);
    }
    else {
        buf.skipBits(5);
    }

    if (mainid_flag) {
        buf.skipBits(3);
        buf.getBits(priority, 2);
        buf.getBits(mainid, 3);
    }
    if (asvc_flag) {
        asvc = buf.getUInt8();
    }
    if (substream1_flag) {
        substream1 = buf.getUInt8();
    }
    if (substream2_flag) {
        substream2 = buf.getUInt8();
    }
    if (substream3_flag) {
        substream3 = buf.getUInt8();
    }
    if (language_flag) {
        buf.getLanguageCode(language);
    }
    if (language_2_flag) {
        buf.getLanguageCode(language_2);
    }
    if (substream1_flag) {
        buf.getLanguageCode(substream1_lang);
    }
    if (substream2_flag) {
        buf.getLanguageCode(substream2_lang);
    }
    if (substream3_flag) {
        buf.getLanguageCode(substream3_lang);
    }
    buf.getBytes(additional_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCEAC3AudioDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        // Fixed initial size: 2 bytes.
        buf.skipBits(1);
        const bool bsid_flag = buf.getBool();
        const bool mainid_flag = buf.getBool();
        const bool asvc_flag = buf.getBool();
        const bool mixinfo = buf.getBool();
        const bool sub1_flag = buf.getBool();
        const bool sub2_flag = buf.getBool();
        const bool sub3_flag = buf.getBool();
        bool lang_flag = false;
        bool lang2_flag = false;

        buf.skipBits(1);
        disp << margin << UString::Format(u"Mixinfo exists: %s", {mixinfo}) << std::endl;
        disp << margin << UString::Format(u"Full service: %s", {buf.getBool()}) << std::endl;
        disp << margin << "Audio service type: " << DataName(MY_XML_NAME, u"ServiceType", buf.getBits<uint8_t>(3), NamesFlags::VALUE) << std::endl;
        disp << margin << "Num. channels: " << DataName(MY_XML_NAME, u"NumChannels", buf.getBits<uint8_t>(3), NamesFlags::VALUE) << std::endl;

        if (buf.canRead()) {
            lang_flag = buf.getBool();
            lang2_flag = buf.getBool();
            buf.skipBits(1);
            if (bsid_flag) {
                disp << margin << UString::Format(u"Bit stream id (bsid): 0x%X (%<d)", {buf.getBits<uint8_t>(5)}) << std::endl;
            }
            else {
                buf.skipBits(5);
            }
        }
        if (mainid_flag && buf.canRead()) {
            buf.skipBits(3);
            disp << margin << UString::Format(u"Priority: %d", {buf.getBits<uint8_t>(2)}) << std::endl;
            disp << margin << UString::Format(u"Main id: 0x%X (%<d)", {buf.getBits<uint8_t>(3)}) << std::endl;
        }
        if (asvc_flag && buf.canRead()) {
            disp << margin << UString::Format(u"Associated service (asvc): 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
        if (sub1_flag && buf.canRead()) {
            disp << margin << UString::Format(u"Substream 1: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
        if (sub2_flag && buf.canRead()) {
            disp << margin << UString::Format(u"Substream 2: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
        if (sub3_flag && buf.canRead()) {
            disp << margin << UString::Format(u"Substream 3: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
        if (lang_flag && buf.canReadBytes(3)) {
            disp << margin << "Language: \"" << buf.getLanguageCode() << "\"" << std::endl;
        }
        if (lang2_flag && buf.canReadBytes(3)) {
            disp << margin << "Language 2: \"" << buf.getLanguageCode() << "\"" << std::endl;
        }
        if (sub1_flag && buf.canReadBytes(3)) {
            disp << margin << "Substream 1 language: \"" << buf.getLanguageCode() << "\"" << std::endl;
        }
        if (sub2_flag && buf.canReadBytes(3)) {
            disp << margin << "Substream 2 language: \"" << buf.getLanguageCode() << "\"" << std::endl;
        }
        if (sub3_flag && buf.canReadBytes(3)) {
            disp << margin << "Substream 3 language: \"" << buf.getLanguageCode() << "\"" << std::endl;
        }
        disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ATSCEAC3AudioDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"mixinfoexists", mixinfoexists);
    root->setBoolAttribute(u"full_service", full_service);
    root->setIntAttribute(u"audio_service_type", audio_service_type, true);
    root->setIntAttribute(u"number_of_channels", number_of_channels, true);
    root->setOptionalIntAttribute(u"bsid", bsid, true);
    root->setOptionalIntAttribute(u"priority", priority, true);
    root->setOptionalIntAttribute(u"mainid", mainid, true);
    root->setOptionalIntAttribute(u"asvc", asvc, true);
    root->setOptionalIntAttribute(u"substream1", substream1, true);
    root->setOptionalIntAttribute(u"substream2", substream2, true);
    root->setOptionalIntAttribute(u"substream3", substream3, true);
    root->setAttribute(u"language", language, true);
    root->setAttribute(u"language_2", language_2, true);
    root->setAttribute(u"substream1_lang", substream1_lang, true);
    root->setAttribute(u"substream2_lang", substream2_lang, true);
    root->setAttribute(u"substream3_lang", substream3_lang, true);
    if (!additional_info.empty()) {
        root->addHexaTextChild(u"additional_info", additional_info);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ATSCEAC3AudioDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getBoolAttribute(mixinfoexists, u"mixinfoexists", true) &&
            element->getBoolAttribute(full_service, u"full_service", true) &&
            element->getIntAttribute(audio_service_type, u"audio_service_type", true, 0, 0, 0x07) &&
            element->getIntAttribute(number_of_channels, u"number_of_channels", true, 0, 0, 0x07) &&
            element->getOptionalIntAttribute(bsid, u"bsid", 0, 0x1F) &&
            element->getOptionalIntAttribute(priority, u"priority", 0, 0x03) &&
            element->getOptionalIntAttribute(mainid, u"mainid", 0, 0x07) &&
            element->getOptionalIntAttribute(asvc, u"asvc") &&
            element->getOptionalIntAttribute(substream1, u"substream1") &&
            element->getOptionalIntAttribute(substream2, u"substream2") &&
            element->getOptionalIntAttribute(substream3, u"substream3") &&
            element->getAttribute(language, u"language", false, UString(), 0, 3) &&
            element->getAttribute(language_2, u"language_2", false, UString(), 0, 3) &&
            element->getAttribute(substream1_lang, u"substream1_lang", false, UString(), 0, 3) &&
            element->getAttribute(substream2_lang, u"substream2_lang", false, UString(), 0, 3) &&
            element->getAttribute(substream3_lang, u"substream3_lang", false, UString(), 0, 3) &&
            element->getHexaTextChild(additional_info, u"additional_info");
}
