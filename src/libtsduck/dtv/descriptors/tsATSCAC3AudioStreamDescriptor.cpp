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

void ts::ATSCAC3AudioStreamDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    bbp->appendUInt8(uint8_t(((sample_rate_code & 0x07) << 5) | (bsid & 0x1F)));
    bbp->appendUInt8(uint8_t(((bit_rate_code & 0x3F) << 2) | (surround_mode & 0x03)));
    bbp->appendUInt8(uint8_t(((bsmod & 0x07) << 5) | ((num_channels & 0x0F) << 1) | (full_svc ? 0x01 : 0x00)));
    bbp->appendUInt8(0xFF); // langcod, deprecated
    if ((num_channels & 0x0F) == 0) {
        bbp->appendUInt8(0xFF); // langcod2, deprecated
    }
    if ((bsmod & 0x07) < 2) {
        bbp->appendUInt8(uint8_t(((mainid & 0x07) << 5) | ((priority & 0x03) << 2) | 0x07));
    }
    else {
        bbp->appendUInt8(asvcflags);
    }

    // Check if text shall be encoded in ISO Latin-1 (ISO 8859-1) or UTF-16.
    const bool latin1 = DVBCharTableSingleByte::RAW_ISO_8859_1.canEncode(text);

    // Encode the text. The resultant size must fit on 7 bits. The max size is then 127 characters with Latin-1 and 63 with UTF-16.
    const ByteBlock bb(latin1 ? DVBCharTableSingleByte::RAW_ISO_8859_1.encoded(text, 0, 127) : DVBCharTableUTF16::RAW_UNICODE.encoded(text, 0, 63));

    // Serialize the text.
    bbp->appendUInt8(uint8_t((bb.size() << 1) | (latin1 ? 0x01 : 0x00)));
    bbp->append(bb);

    // Serialize the languages.
    bbp->appendUInt8((language.empty() ? 0x00 : 0x80) | (language_2.empty() ? 0x00 : 0x40) | 0x3F);
    if (!language.empty()) {
        SerializeLanguageCode(*bbp, language);
    }
    if (!language_2.empty()) {
        SerializeLanguageCode(*bbp, language_2);
    }

    // Trailing info.
    bbp->append(additional_info);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ATSCAC3AudioStreamDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    clear();
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 3;

    if (!_is_valid) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    // Fixed initial size: 3 bytes.
    sample_rate_code = uint8_t((data[0] >> 5) & 0x07);
    bsid = uint8_t(data[0] & 0x1F);
    bit_rate_code = uint8_t((data[1] >> 2) & 0x3F);
    surround_mode = uint8_t(data[1] & 0x03);
    bsmod = uint8_t((data[2] >> 5) & 0x07);
    num_channels = uint8_t((data[2] >> 1) & 0x0F);
    full_svc = (data[2] & 0x01) != 0;
    data += 3; size -= 3;

    // End of descriptor allowed here
    if (size == 0) {
        return;
    }

    // Ignore langcode, deprecated
    data++; size--;

    // End of descriptor allowed here
    if (size == 0) {
        return;
    }

    // Ignore langcode2, deprecated
    if (num_channels == 0) {
        data++; size--;
    }

    // End of descriptor allowed here
    if (size == 0) {
        return;
    }

    // Decode one byte depending on bsmod.
    if (bsmod < 2) {
        mainid = uint8_t((data[0] >> 5) & 0x07);
        priority = uint8_t((data[0] >> 3) & 0x03);
    }
    else {
        asvcflags = data[0];
    }
    data++; size--;

    // End of descriptor allowed here
    if (size == 0) {
        return;
    }

    // Deserialize text. Can be ISO Latin-1 or UTF-16.
    const size_t textlen = (data[0] >> 1) & 0x7F;
    const bool latin1 = (data[0] & 0x01) != 0;
    data++; size--;
    if (size < textlen) {
        _is_valid = false; // text is truncated
        return;
    }
    _is_valid = latin1 ?
                DVBCharTableSingleByte::RAW_ISO_8859_1.decode(text, data, textlen) :
                DVBCharTableUTF16::RAW_UNICODE.decode(text, data, textlen);
    data += textlen; size -= textlen;

    // End of descriptor allowed here
    if (!_is_valid || size == 0) {
        return;
    }

    // Decode one byte flags.
    const bool has_language = (data[0] & 0x80) != 0;
    const bool has_language_2 = (data[0] & 0x40) != 0;
    data++; size--;
    _is_valid = size >= size_t((has_language ? 3 : 0) + (has_language_2 ? 3 : 0));

    // End of descriptor allowed here
    if (!_is_valid || size == 0) {
        return;
    }

    // Deserialize languages.
    if (has_language) {
        language = DeserializeLanguageCode(data);
        data += 3; size -= 3;
    }
    if (has_language_2) {
        language_2 = DeserializeLanguageCode(data);
        data += 3; size -= 3;
    }

    // Trailing info.
    additional_info.copy(data, size);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCAC3AudioStreamDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    if (size >= 3) {
        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        // Fixed initial size: 3 bytes.
        const uint8_t sample = uint8_t((data[0] >> 5) & 0x07);
        const uint8_t bsid = uint8_t(data[0] & 0x1F);
        const uint8_t bitrate = uint8_t((data[1] >> 2) & 0x3F);
        const uint8_t surround = uint8_t(data[1] & 0x03);
        const uint8_t bsmod = uint8_t((data[2] >> 5) & 0x07);
        const uint8_t channels = uint8_t((data[2] >> 1) & 0x0F);
        const bool full = (data[2] & 0x01) != 0;
        data += 3; size -= 3;

        strm << margin << UString::Format(u"Sample rate: %s", {NameFromSection(u"ATSCAC3SampleRateCode", sample, names::VALUE)}) << std::endl
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
                strm << margin << UString::Format(u"Main audio service id: %d", {mainid}) << std::endl
                     << margin << UString::Format(u"Priority: %d", {priority}) << std::endl;
            }
            else {
                const uint8_t asvcflags = data[0];
                strm << margin << UString::Format(u"Associated services flags: 0x%X", {asvcflags}) << std::endl;
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
            strm << margin << "Text: \"" << text << "\"" << std::endl;
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
            strm << margin << "Language: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }
        if (ok && has_lang2) {
            strm << margin << "Language 2: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
            data += 3; size -= 3;
        }

        // Trailing info.
        if (ok) {
            display.displayPrivateData(u"Additional information", data, size, indent);
            data += size; size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
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
