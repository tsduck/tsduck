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

#include "tsATSCEAC3AudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

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

void ts::ATSCEAC3AudioDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(0x80 |
                     (bsid.set() ? 0x40 : 0x00) |
                     (mainid.set() && priority.set() ? 0x20 : 0x00) |
                     (asvc.set() ? 0x10 : 0x00) |
                     (mixinfoexists ? 0x08 : 0x00) |
                     (substream1.set() ? 0x04 : 0x00) |
                     (substream2.set() ? 0x02 : 0x00) |
                     (substream3.set() ? 0x01 : 0x00));
    bbp->appendUInt8(0x80 |
                     (full_service ? 0x40 : 0x00) |
                     uint8_t((audio_service_type & 0x07) << 3) |
                     (number_of_channels & 0x07));
    bbp->appendUInt8((language.empty() ? 0x00 : 0x80) |
                     (language_2.empty() ? 0x00 : 0x40) |
                     0x20 |
                     (bsid.value(0x00) & 0x1F));
    if (mainid.set() && priority.set()) {
        bbp->appendUInt8(0xE0 |
                         uint8_t((priority.value() & 0x03) << 3) |
                         (mainid.value() & 0x07));
    }
    if (asvc.set()) {
        bbp->appendUInt8(asvc.value());
    }
    if (substream1.set()) {
        bbp->appendUInt8(substream1.value());
    }
    if (substream2.set()) {
        bbp->appendUInt8(substream2.value());
    }
    if (substream3.set()) {
        bbp->appendUInt8(substream3.value());
    }
    if (!language.empty() && !SerializeLanguageCode(*bbp, language)) {
        return; // invalid language code size
    }
    if (!language_2.empty() && !SerializeLanguageCode(*bbp, language_2)) {
        return; // invalid language code size
    }
    if (substream1.set() && !SerializeLanguageCode(*bbp, substream1_lang)) {
        return; // invalid language code size
    }
    if (substream2.set() && !SerializeLanguageCode(*bbp, substream2_lang)) {
        return; // invalid language code size
    }
    if (substream3.set() && !SerializeLanguageCode(*bbp, substream3_lang)) {
        return; // invalid language code size
    }
    bbp->append(additional_info);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ATSCEAC3AudioDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    clear();
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 2;

    if (!_is_valid) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    // Fixed initial size: 2 bytes.
    const bool bsid_flag = (data[0] & 0x40) != 0;
    const bool mainid_flag = (data[0] & 0x20) != 0;
    const bool asvc_flag = (data[0] & 0x10) != 0;
    mixinfoexists = (data[0] & 0x08) != 0;
    const bool substream1_flag = (data[0] & 0x04) != 0;
    const bool substream2_flag = (data[0] & 0x02) != 0;
    const bool substream3_flag = (data[0] & 0x01) != 0;
    full_service = (data[1] & 0x40) != 0;
    audio_service_type = (data[1] >> 3) & 0x07;
    number_of_channels = data[1] & 0x07;
    data += 2; size -= 2;

    // End of descriptor allowed here
    if (size == 0) {
        return;
    }

    // Decode one byte depending on bsid.
    const bool language_flag = (data[0] & 0x80) != 0;
    const bool language_2_flag = (data[0] & 0x40) != 0;
    if (bsid_flag) {
        bsid = data[0] & 0x1F;
    }
    data++; size--;

    if (mainid_flag && size > 0) {
        priority = (data[0] >> 3) & 0x03;
        mainid = data[0] & 0x07;
        data++; size--;
    }
    if (asvc_flag && size > 0) {
        asvc = data[0];
        data++; size--;
    }
    if (substream1_flag && size > 0) {
        substream1 = data[0];
        data++; size--;
    }
    if (substream2_flag && size > 0) {
        substream2 = data[0];
        data++; size--;
    }
    if (substream3_flag && size > 0) {
        substream3 = data[0];
        data++; size--;
    }
    if (language_flag) {
        if (size < 3) {
            _is_valid = false;
        }
        else {
            language = DeserializeLanguageCode(data);
            data += 3; size -= 3;
        }
    }
    if (_is_valid && language_2_flag) {
        if (size < 3) {
            _is_valid = false;
        }
        else {
            language_2 = DeserializeLanguageCode(data);
            data += 3; size -= 3;
        }
    }
    if (_is_valid && substream1_flag) {
        if (size < 3) {
            _is_valid = false;
        }
        else {
            substream1_lang = DeserializeLanguageCode(data);
            data += 3; size -= 3;
        }
    }
    if (_is_valid && substream2_flag) {
        if (size < 3) {
            _is_valid = false;
        }
        else {
            substream2_lang = DeserializeLanguageCode(data);
            data += 3; size -= 3;
        }
    }
    if (_is_valid && substream3_flag) {
        if (size < 3) {
            _is_valid = false;
        }
        else {
            substream3_lang = DeserializeLanguageCode(data);
            data += 3; size -= 3;
        }
    }
    if (_is_valid) {
        additional_info.copy(data, size);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCEAC3AudioDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    if (size >= 2) {

        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        // Fixed initial size: 2 bytes.
        const bool bsid_flag = (data[0] & 0x40) != 0;
        const bool mainid_flag = (data[0] & 0x20) != 0;
        const bool asvc_flag = (data[0] & 0x10) != 0;
        const bool mixinfo = (data[0] & 0x08) != 0;
        const bool sub1_flag = (data[0] & 0x04) != 0;
        const bool sub2_flag = (data[0] & 0x02) != 0;
        const bool sub3_flag = (data[0] & 0x01) != 0;
        const bool full = (data[1] & 0x40) != 0;
        const uint8_t svc_type = (data[1] >> 3) & 0x07;
        const uint8_t channels = data[1] & 0x07;
        const bool lang_flag = size > 2 && (data[2] & 0x80) != 0;   // in next byte
        const bool lang2_flag = size > 2 && (data[2] & 0x40) != 0;  // in next byte
        data += 2; size -= 2;

        strm << margin << UString::Format(u"Mixinfo exists: %s", {mixinfo}) << std::endl
             << margin << UString::Format(u"Full service: %s", {full}) << std::endl
             << margin << UString::Format(u"Audio service type: %s", {NameFromSection(u"EAC3AudioServiceType", svc_type, names::VALUE)}) << std::endl
             << margin << UString::Format(u"Num. channels: %s", {NameFromSection(u"ATSCEAC3NumChannels", channels, names::VALUE)}) << std::endl;

        if (size > 0) {
            if (bsid_flag) {
                const uint8_t id = data[0] & 0x1F;
                strm << margin << UString::Format(u"Bit stream id (bsid): 0x%X (%d)", {id, id}) << std::endl;
            }
            data++; size--;
        }
        if (mainid_flag && size > 0) {
            const uint8_t id = data[0] & 0x07;
            strm << margin << UString::Format(u"Priority: %d", {(data[0] >> 3) & 0x03}) << std::endl
                 << margin << UString::Format(u"Main id: 0x%X (%d)", {id, id}) << std::endl;
            data++; size--;
        }
        if (asvc_flag && size > 0) {
            strm << margin << UString::Format(u"Associated service (asvc): 0x%X (%d)", {data[0], data[0]}) << std::endl;
            data++; size--;
        }
        if (sub1_flag && size > 0) {
            strm << margin << UString::Format(u"Substream 1: 0x%X (%d)", {data[0], data[0]}) << std::endl;
            data++; size--;
        }
        if (sub2_flag && size > 0) {
            strm << margin << UString::Format(u"Substream 2: 0x%X (%d)", {data[0], data[0]}) << std::endl;
            data++; size--;
        }
        if (sub3_flag && size > 0) {
            strm << margin << UString::Format(u"Substream 3: 0x%X (%d)", {data[0], data[0]}) << std::endl;
            data++; size--;
        }
        if (lang_flag && size >= 3) {
            strm << margin << "Language: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }
        if (lang2_flag && size >= 3) {
            strm << margin << "Language 2: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }
        if (sub1_flag && size >= 3) {
            strm << margin << "Substream 1 language: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }
        if (sub2_flag && size >= 3) {
            strm << margin << "Substream 2 language: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }
        if (sub3_flag && size >= 3) {
            strm << margin << "Substream 3 language: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }
        if (size > 0) {
            display.displayPrivateData(u"Additional information", data, size, indent);
            data += size; size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
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
            element->getIntAttribute<uint8_t>(audio_service_type, u"audio_service_type", true, 0, 0, 0x07) &&
            element->getIntAttribute<uint8_t>(number_of_channels, u"number_of_channels", true, 0, 0, 0x07) &&
            element->getOptionalIntAttribute<uint8_t>(bsid, u"bsid", 0, 0x1F) &&
            element->getOptionalIntAttribute<uint8_t>(priority, u"priority", 0, 0x03) &&
            element->getOptionalIntAttribute<uint8_t>(mainid, u"mainid", 0, 0x07) &&
            element->getOptionalIntAttribute<uint8_t>(asvc, u"asvc") &&
            element->getOptionalIntAttribute<uint8_t>(substream1, u"substream1") &&
            element->getOptionalIntAttribute<uint8_t>(substream2, u"substream2") &&
            element->getOptionalIntAttribute<uint8_t>(substream3, u"substream3") &&
            element->getAttribute(language, u"language", false, UString(), 0, 3) &&
            element->getAttribute(language_2, u"language_2", false, UString(), 0, 3) &&
            element->getAttribute(substream1_lang, u"substream1_lang", false, UString(), 0, 3) &&
            element->getAttribute(substream2_lang, u"substream2_lang", false, UString(), 0, 3) &&
            element->getAttribute(substream3_lang, u"substream3_lang", false, UString(), 0, 3) &&
            element->getHexaTextChild(additional_info, u"additional_info");
}
