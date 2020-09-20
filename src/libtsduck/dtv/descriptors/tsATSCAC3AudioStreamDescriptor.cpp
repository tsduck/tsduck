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

#include "tsATSCAC3AudioStreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
#include "tsDVBCharTableUTF16.h"
#include "tsDVBCharTableSingleByte.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    sample_rate_code(0),
    bsid(0),
    bit_rate_code(0),
    surround_mode(0),
    bsmod(0),
    num_channels(0),
    full_svc(false),
    mainid(0),
    priority(0),
    asvcflags(0),
    text(),
    language(),
    language_2(),
    additional_info()
{
}

ts::ATSCAC3AudioStreamDescriptor::ATSCAC3AudioStreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCAC3AudioStreamDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

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
    sample_rate_code = buf.getBits<uint8_t>(3);
    bsid = buf.getBits<uint8_t>(5);
    bit_rate_code = buf.getBits<uint8_t>(6);
    surround_mode = buf.getBits<uint8_t>(2);
    bsmod = buf.getBits<uint8_t>(3);
    num_channels = buf.getBits<uint8_t>(4);
    full_svc = buf.getBit() != 0;

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
        mainid = buf.getBits<uint8_t>(3);
        priority = buf.getBits<uint8_t>(2);
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
    const bool latin1 = buf.getBit() != 0;
    buf.getString(text, textlen, latin1 ? static_cast<const Charset*>(&DVBCharTableSingleByte::RAW_ISO_8859_1) : static_cast<const Charset*>(&DVBCharTableUTF16::RAW_UNICODE));

    // End of descriptor allowed here
    if (buf.endOfRead()) {
        return;
    }

    // Decode one byte flags.
    const bool has_language = buf.getBit() != 0;
    const bool has_language_2 = buf.getBit() != 0;
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

void ts::ATSCAC3AudioStreamDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size >= 3) {
        // Fixed initial size: 3 bytes.
        const uint8_t sample = uint8_t((data[0] >> 5) & 0x07);
        const uint8_t bsid = uint8_t(data[0] & 0x1F);
        const uint8_t bitrate = uint8_t((data[1] >> 2) & 0x3F);
        const uint8_t surround = uint8_t(data[1] & 0x03);
        const uint8_t bsmod = uint8_t((data[2] >> 5) & 0x07);
        const uint8_t channels = uint8_t((data[2] >> 1) & 0x0F);
        const bool full = (data[2] & 0x01) != 0;
        data += 3; size -= 3;

        disp << margin << UString::Format(u"Sample rate: %s", {NameFromSection(u"ATSCAC3SampleRateCode", sample, names::VALUE)}) << std::endl
             << margin << UString::Format(u"AC-3 coding version: 0x%X (%d)", {bsid, bsid}) << std::endl
             << margin << UString::Format(u"Bit rate: %s%s", {NameFromSection(u"ATSCAC3BitRateCode", bitrate & 0x1F, names::VALUE), (bitrate & 0x20) == 0 ? u"" : u" max"}) << std::endl
             << margin << UString::Format(u"Surround mode: %s", {NameFromSection(u"ATSCAC3SurroundMode", surround, names::VALUE)}) << std::endl
             << margin << UString::Format(u"Bitstream mode: %s", {NameFromSection(u"AC3BitStreamMode", bsmod, names::VALUE)}) << std::endl
             << margin << UString::Format(u"Num. channels: %s", {NameFromSection(u"ATSCAC3NumChannels", channels, names::VALUE)}) << std::endl
             << margin << UString::Format(u"Full service: %s", {full}) << std::endl;

        // Ignore langcode and langcode2, deprecated
        if (size > 0) {
            data++; size--;
        }
        if (channels == 0 && size > 0) {
            data++; size--;
        }

        // Decode one byte depending on bsmod.
        if (size >= 1) {
            if (bsmod < 2) {
                const uint8_t mainid = uint8_t((data[0] >> 5) & 0x07);
                const uint8_t priority = uint8_t((data[0] >> 3) & 0x03);
                disp << margin << UString::Format(u"Main audio service id: %d", {mainid}) << std::endl
                     << margin << UString::Format(u"Priority: %d", {priority}) << std::endl;
            }
            else {
                const uint8_t asvcflags = data[0];
                disp << margin << UString::Format(u"Associated services flags: 0x%X", {asvcflags}) << std::endl;
            }
            data++; size--;
        }

        // Decode text.
        if (size >= 1) {
            size_t textlen = (data[0] >> 1) & 0x7F;
            const bool latin1 = (data[0] & 0x01) != 0;
            data++; size--;
            if (size < textlen) {
                textlen = size;
            }
            UString text;
            if (latin1) {
                DVBCharTableSingleByte::RAW_ISO_8859_1.decode(text, data, textlen);
            }
            else {
                DVBCharTableUTF16::RAW_UNICODE.decode(text, data, textlen);
            }
            data += textlen; size -= textlen;
            disp << margin << "Text: \"" << text << "\"" << std::endl;
        }

        // Decode one byte flags.
        bool has_lang = false;
        bool has_lang2 = false;
        if (size >= 1) {
            has_lang = (data[0] & 0x80) != 0;
            has_lang2 = (data[0] & 0x40) != 0;
            data++; size--;
        }
        bool ok = size >= size_t((has_lang ? 3 : 0) + (has_lang2 ? 3 : 0));

        // Deserialize languages.
        if (ok && has_lang) {
            disp << margin << "Language: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }
        if (ok && has_lang2) {
            disp << margin << "Language 2: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }

        // Trailing info.
        if (ok) {
            disp.displayPrivateData(u"Additional information", data, size, margin);
            data += size; size = 0;
        }
    }

    disp.displayExtraData(data, size, margin);
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
    return element->getIntAttribute<uint8_t>(sample_rate_code, u"sample_rate_code", true, 0, 0, 0x07) &&
           element->getIntAttribute<uint8_t>(bsid, u"bsid", true, 0, 0, 0x1F) &&
           element->getIntAttribute<uint8_t>(bit_rate_code, u"bit_rate_code", true, 0, 0, 0x3F) &&
           element->getIntAttribute<uint8_t>(surround_mode, u"surround_mode", true, 0, 0, 0x03) &&
           element->getIntAttribute<uint8_t>(bsmod, u"bsmod", true, 0, 0, 0x07) &&
           element->getIntAttribute<uint8_t>(num_channels, u"num_channels", true, 0, 0, 0x0F) &&
           element->getBoolAttribute(full_svc, u"full_svc", true) &&
           element->getIntAttribute<uint8_t>(mainid, u"mainid", bsmod < 2, 0, 0, 0x07) &&
           element->getIntAttribute<uint8_t>(priority, u"priority", bsmod < 2, 0, 0, 0x03) &&
           element->getIntAttribute<uint8_t>(asvcflags, u"asvcflags", bsmod >= 2, 0, 0, 0xFF) &&
           element->getAttribute(text, u"text") &&
           element->getAttribute(language, u"language") &&
           element->getAttribute(language_2, u"language_2") &&
           element->getHexaTextChild(additional_info, u"additional_info");
}
