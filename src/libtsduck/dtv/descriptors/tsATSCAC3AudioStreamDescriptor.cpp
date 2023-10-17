//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCAC3AudioStreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsDVBCharTableUTF16.h"
#include "tsDVBCharTableSingleByte.h"

#define MY_XML_NAME u"ATSC_AC3_audio_stream_descriptor"
#define MY_CLASS ts::ATSCAC3AudioStreamDescriptor
#define MY_DID ts::DID_ATSC_AC3
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCAC3AudioStreamDescriptor::ATSCAC3AudioStreamDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ATSCAC3AudioStreamDescriptor::ATSCAC3AudioStreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCAC3AudioStreamDescriptor()
{
    deserialize(duck, desc);
}

void ts::ATSCAC3AudioStreamDescriptor::clearContent()
{
    sample_rate_code = 0;
    bsid = 0;
    bit_rate_code = 0;
    surround_mode = 0;
    bsmod = 0;
    num_channels = 0;
    full_svc = false;
    mainid = 0;
    priority = 0;
    asvcflags = 0;
    text.clear();
    language.clear();
    language_2.clear();
    additional_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ATSCAC3AudioStreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(sample_rate_code, 3);
    buf.putBits(bsid, 5);
    buf.putBits(bit_rate_code, 6);
    buf.putBits(surround_mode, 2);
    buf.putBits(bsmod, 3);
    buf.putBits(num_channels, 4);
    buf.putBit(full_svc);
    buf.putUInt8(0xFF); // langcod, deprecated
    if (num_channels == 0) {
        buf.putUInt8(0xFF); // langcod2, deprecated
    }
    if (bsmod < 2) {
        buf.putBits(mainid, 3);
        buf.putBits(priority, 2);
        buf.putBits(0xFF, 3);
    }
    else {
        buf.putUInt8(asvcflags);
    }

    // Check if text shall be encoded in ISO Latin-1 (ISO 8859-1) or UTF-16.
    const bool latin1 = DVBCharTableSingleByte::RAW_ISO_8859_1.canEncode(text);

    // Encode the text. The resultant size must fit on 7 bits. The max size is then 127 characters with Latin-1 and 63 with UTF-16.
    const ByteBlock bb(latin1 ? DVBCharTableSingleByte::RAW_ISO_8859_1.encoded(text, 0, 127) : DVBCharTableUTF16::RAW_UNICODE.encoded(text, 0, 63));

    // Serialize the text.
    buf.putBits(bb.size(), 7);
    buf.putBit(latin1);
    buf.putBytes(bb);

    // Serialize the languages.
    buf.putBit(!language.empty());
    buf.putBit(!language_2.empty());
    buf.putBits(0xFF, 6);
    if (!language.empty()) {
        buf.putLanguageCode(language);
    }
    if (!language_2.empty()) {
        buf.putLanguageCode(language_2);
    }

    // Trailing info.
    buf.putBytes(additional_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ATSCAC3AudioStreamDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(sample_rate_code, 3);
    buf.getBits(bsid, 5);
    buf.getBits(bit_rate_code, 6);
    buf.getBits(surround_mode, 2);
    buf.getBits(bsmod, 3);
    buf.getBits(num_channels, 4);
    full_svc = buf.getBool();

    // End of descriptor allowed here
    if (buf.endOfRead()) {
        return;
    }

    // Ignore langcode, deprecated
    buf.skipBits(8);

    // End of descriptor allowed here
    if (buf.endOfRead()) {
        return;
    }

    // Ignore langcode2, deprecated
    if (num_channels == 0) {
        buf.skipBits(8);
    }

    // End of descriptor allowed here
    if (buf.endOfRead()) {
        return;
    }

    // Decode one byte depending on bsmod.
    if (bsmod < 2) {
        buf.getBits(mainid, 3);
        buf.getBits(priority, 2);
        buf.skipBits(3);
    }
    else {
        asvcflags = buf.getUInt8();
    }

    // End of descriptor allowed here
    if (buf.endOfRead()) {
        return;
    }

    // Deserialize text. Can be ISO Latin-1 or UTF-16.
    const size_t textlen = buf.getBits<size_t>(7);
    const bool latin1 = buf.getBool();
    buf.getString(text, textlen, latin1 ? static_cast<const Charset*>(&DVBCharTableSingleByte::RAW_ISO_8859_1) : static_cast<const Charset*>(&DVBCharTableUTF16::RAW_UNICODE));

    // End of descriptor allowed here
    if (buf.endOfRead()) {
        return;
    }

    // Decode one byte flags.
    const bool has_language = buf.getBool();
    const bool has_language_2 = buf.getBool();
    buf.skipBits(6);

    // End of descriptor allowed here
    if (buf.endOfRead()) {
        return;
    }

    // Deserialize languages.
    if (has_language) {
        buf.getLanguageCode(language);
    }
    if (has_language_2) {
        buf.getLanguageCode(language_2);
    }

    // Trailing info.
    buf.getBytes(additional_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCAC3AudioStreamDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        // Fixed initial size: 3 bytes.
        disp << margin << "Sample rate: " << DataName(MY_XML_NAME, u"SampleRateCode", buf.getBits<uint8_t>(3), NamesFlags::VALUE) << std::endl;
        disp << margin << UString::Format(u"AC-3 coding version: 0x%X (%<d)", {buf.getBits<uint8_t>(5)}) << std::endl;
        const uint8_t bitrate = buf.getBits<uint8_t>(6);
        disp << margin << "Bit rate: " << DataName(MY_XML_NAME, u"BitRateCode", bitrate & 0x1F, NamesFlags::VALUE) << ((bitrate & 0x20) == 0 ? "" : " max") << std::endl;
        disp << margin << "Surround mode: " << DataName(MY_XML_NAME, u"SurroundMode", buf.getBits<uint8_t>(2), NamesFlags::VALUE) << std::endl;
        const uint8_t bsmod = buf.getBits<uint8_t>(3);
        disp << margin << "Bitstream mode: " << DataName(MY_XML_NAME, u"BitStreamMode", bsmod, NamesFlags::VALUE) << std::endl;
        const uint8_t channels = buf.getBits<uint8_t>(4);
        disp << margin << "Num. channels: " << DataName(MY_XML_NAME, u"NumChannels", channels, NamesFlags::VALUE) << std::endl;
        disp << margin << UString::Format(u"Full service: %s", {buf.getBool()}) << std::endl;

        // Ignore langcode and langcode2, deprecated
        buf.skipBits(8);
        if (channels == 0) {
            buf.skipBits(8);
        }

        // Decode one byte depending on bsmod.
        if (buf.canRead()) {
            if (bsmod < 2) {
                disp << margin << UString::Format(u"Main audio service id: %d", {buf.getBits<uint8_t>(3)}) << std::endl;
                disp << margin << UString::Format(u"Priority: %d", {buf.getBits<uint8_t>(2)}) << std::endl;
                buf.skipBits(3);
            }
            else {
                disp << margin << UString::Format(u"Associated services flags: 0x%X", {buf.getUInt8()}) << std::endl;
            }
        }

        // Decode text.
        if (buf.canRead()) {
            const size_t textlen = buf.getBits<size_t>(7);
            const bool latin1 = buf.getBool();
            const Charset* charset = latin1 ? static_cast<const Charset*>(&DVBCharTableSingleByte::RAW_ISO_8859_1) : static_cast<const Charset*>(&DVBCharTableUTF16::RAW_UNICODE);
            disp << margin << "Text: \"" << buf.getString(textlen, charset) << "\"" << std::endl;
        }

        // Decode one byte flags.
        bool has_lang = false;
        bool has_lang2 = false;
        if (buf.canRead()) {
            has_lang = buf.getBool();
            has_lang2 = buf.getBool();
            buf.skipBits(6);
        }

        // Deserialize languages.
        if (has_lang) {
            disp << margin << "Language: \"" << buf.getLanguageCode() << "\"" << std::endl;
        }
        if (has_lang2) {
            disp << margin << "Language 2: \"" << buf.getLanguageCode() << "\"" << std::endl;
        }

        // Trailing info.
        disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ATSCAC3AudioStreamDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"sample_rate_code", sample_rate_code, true);
    root->setIntAttribute(u"bsid", bsid, true);
    root->setIntAttribute(u"bit_rate_code", bit_rate_code, true);
    root->setIntAttribute(u"surround_mode", surround_mode, true);
    root->setIntAttribute(u"bsmod", bsmod, true);
    root->setIntAttribute(u"num_channels", num_channels, true);
    root->setBoolAttribute(u"full_svc", full_svc);
    if ((bsmod & 0x07) < 2) {
        root->setIntAttribute(u"mainid", mainid, true);
        root->setIntAttribute(u"priority", priority, true);
    }
    else {
        root->setIntAttribute(u"asvcflags", asvcflags, true);
    }
    root->setAttribute(u"text", text, true);
    root->setAttribute(u"language", language, true);
    root->setAttribute(u"language_2", language_2, true);
    if (!additional_info.empty()) {
        root->addHexaTextChild(u"additional_info", additional_info);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ATSCAC3AudioStreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(sample_rate_code, u"sample_rate_code", true, 0, 0, 0x07) &&
           element->getIntAttribute(bsid, u"bsid", true, 0, 0, 0x1F) &&
           element->getIntAttribute(bit_rate_code, u"bit_rate_code", true, 0, 0, 0x3F) &&
           element->getIntAttribute(surround_mode, u"surround_mode", true, 0, 0, 0x03) &&
           element->getIntAttribute(bsmod, u"bsmod", true, 0, 0, 0x07) &&
           element->getIntAttribute(num_channels, u"num_channels", true, 0, 0, 0x0F) &&
           element->getBoolAttribute(full_svc, u"full_svc", true) &&
           element->getIntAttribute(mainid, u"mainid", bsmod < 2, 0, 0, 0x07) &&
           element->getIntAttribute(priority, u"priority", bsmod < 2, 0, 0, 0x03) &&
           element->getIntAttribute(asvcflags, u"asvcflags", bsmod >= 2, 0, 0, 0xFF) &&
           element->getAttribute(text, u"text") &&
           element->getAttribute(language, u"language") &&
           element->getAttribute(language_2, u"language_2") &&
           element->getHexaTextChild(additional_info, u"additional_info");
}
