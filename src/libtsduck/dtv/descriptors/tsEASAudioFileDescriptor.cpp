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

#include "tsEASAudioFileDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"EAS_audio_file_descriptor"
#define MY_CLASS ts::EASAudioFileDescriptor
#define MY_DID ts::DID_EAS_AUDIO_FILE
#define MY_TID ts::TID_SCTE18_EAS
#define MY_STD ts::Standards::SCTE

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EASAudioFileDescriptor::EASAudioFileDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
{
}

ts::EASAudioFileDescriptor::EASAudioFileDescriptor(DuckContext& duck, const Descriptor& desc) :
    EASAudioFileDescriptor()
{
    deserialize(duck, desc);
}

void ts::EASAudioFileDescriptor::clearContent()
{
    entries.clear();
}

ts::EASAudioFileDescriptor::Entry::Entry() :
    file_name(),
    audio_format(0),
    audio_source(0),
    program_number(0),
    carousel_id(0),
    download_id(0),
    module_id(0),
    application_id(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EASAudioFileDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(entries.size()));
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        const size_t loop_length_index = bbp->size();
        bbp->appendUInt8(0); // place-holder for loop_length
        bbp->appendUInt8((it->file_name.empty() ? 0x00 : 0x80) | (it->audio_format & 0x7F));
        if (!it->file_name.empty()) {
            const std::string utf8(it->file_name.toUTF8());
            bbp->appendUInt8(uint8_t(utf8.size()));
            bbp->append(utf8);
        }
        bbp->appendUInt8(it->audio_source);
        if (it->audio_source == 0x01) {
            bbp->appendUInt16(it->program_number);
            bbp->appendUInt32(it->carousel_id);
            bbp->appendUInt16(it->application_id);
        }
        else if (it->audio_source == 0x02) {
            bbp->appendUInt16(it->program_number);
            bbp->appendUInt32(it->download_id);
            bbp->appendUInt32(it->module_id);
            bbp->appendUInt16(it->application_id);
        }
        // Update loop_length;
        (*bbp)[loop_length_index] = uint8_t(bbp->size() - loop_length_index - 1);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EASAudioFileDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    entries.clear();
    _is_valid = false;

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    if (!desc.isValid() || desc.tag() != tag() || size == 0) {
        return;
    }

    // Number of audio sources.
    size_t count = data[0];
    data++; size--;

    while (count-- > 0) {
        Entry entry;
        size_t loop_length = size == 0 ? 0 : data[0];
        if (loop_length < 2 || size < 1 + loop_length) {
            return; // loop instance does not fit here
        }
        const bool file_name_present = (data[1] & 0x80) != 0;
        entry.audio_format = data[1] & 0x7F;
        data += 2; size -= 2;
        loop_length--;
        if (file_name_present) {
            if (loop_length == 0 || 1 + size_t(data[0]) > loop_length) {
                return;
            }
            const size_t file_name_length = data[0];
            entry.file_name.assignFromUTF8(reinterpret_cast<const char*>(data + 1), file_name_length);
            data += 1 + file_name_length; size -= 1 + file_name_length;
            loop_length -= 1 + file_name_length;
        }
        if (loop_length < 1) {
            return;
        }
        entry.audio_source = data[0];
        data++; size--;
        loop_length--;
        if (entry.audio_source == 0x01) {
            if (loop_length < 8) {
                return;
            }
            entry.program_number = GetUInt16(data);
            entry.carousel_id = GetUInt32(data + 2);
            entry.application_id = GetUInt16(data + 6);
            data += 8; size -= 8;
            loop_length -= 8;
        }
        else if (entry.audio_source == 0x02) {
            if (loop_length < 12) {
                return;
            }
            entry.program_number = GetUInt16(data);
            entry.download_id = GetUInt32(data + 2);
            entry.module_id = GetUInt32(data + 6);
            entry.application_id = GetUInt16(data + 10);
            data += 12; size -= 12;
            loop_length -= 12;
        }
        // Skip unused part of loop instance, if any.
        data += loop_length; size -= loop_length;
        entries.push_back(entry);
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EASAudioFileDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    if (size > 0) {
        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        // Number of audio sources.
        size_t count = data[0];
        data++; size--;
        strm << margin << UString::Format(u"Number of audio sources: %d", {count}) << std::endl;

        while (count-- > 0) {
            size_t loop_length = size == 0 ? 0 : data[0];
            if (loop_length < 2 || size < 1 + loop_length) {
                break; // loop instance does not fit here
            }
            const bool file_name_present = (data[1] & 0x80) != 0;
            const uint8_t audio_format = data[1] & 0x7F;
            data += 2; size -= 2;
            loop_length--;

            strm << margin << "- Audio format: " << NameFromSection(u"EASAudioFormat", audio_format, names::VALUE) << std::endl;

            if (file_name_present) {
                if (loop_length == 0 || 1 + size_t(data[0]) > loop_length) {
                    break;
                }
                const size_t file_name_length = data[0];
                strm << margin << "  File name: \"" << std::string(reinterpret_cast<const char*>(data + 1), file_name_length) << "\"" << std::endl;
                data += 1 + file_name_length; size -= 1 + file_name_length;
                loop_length -= 1 + file_name_length;
            }
            if (loop_length < 1) {
                break;
            }

            const uint8_t audio_source = data[0];
            data++; size--;
            loop_length--;

            strm << margin << "  Audio source: " << NameFromSection(u"EASAudioSource", audio_source, names::VALUE) << std::endl;

            if (audio_source == 0x01) {
                if (loop_length < 8) {
                    break;
                }
                strm << margin << UString::Format(u"  Program number: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                     << margin << UString::Format(u"  Carousel id: 0x%X (%d)", {GetUInt32(data + 2), GetUInt32(data + 2)}) << std::endl
                     << margin << UString::Format(u"  Application id: 0x%X (%d)", {GetUInt16(data + 6), GetUInt16(data + 6)}) << std::endl;
                data += 8; size -= 8;
                loop_length -= 8;
            }
            else if (audio_source == 0x02) {
                if (loop_length < 12) {
                    break;
                }
                strm << margin << UString::Format(u"  Program number: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                     << margin << UString::Format(u"  Download id: 0x%X (%d)", {GetUInt32(data + 2), GetUInt32(data + 2)}) << std::endl
                     << margin << UString::Format(u"  Module id: 0x%X (%d)", {GetUInt32(data + 6), GetUInt32(data + 6)}) << std::endl
                     << margin << UString::Format(u"  Application id: 0x%X (%d)", {GetUInt16(data + 10), GetUInt16(data + 10)}) << std::endl;
                data += 12; size -= 12;
                loop_length -= 12;
            }

            // Unused part of loop instance, if any.
            display.displayExtraData(data, loop_length, indent + 2);
            data += loop_length; size -= loop_length;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EASAudioFileDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"file");
        e->setIntAttribute(u"audio_format", it->audio_format, true);
        if (!it->file_name.empty()) {
            e->setAttribute(u"file_name", it->file_name);
        }
        e->setIntAttribute(u"audio_source", it->audio_source, true);
        if (it->audio_source == 0x01) {
            e->setIntAttribute(u"program_number", it->program_number, true);
            e->setIntAttribute(u"carousel_id", it->carousel_id, true);
            e->setIntAttribute(u"application_id", it->application_id, true);
        }
        else if (it->audio_source == 0x02) {
            e->setIntAttribute(u"program_number", it->program_number, true);
            e->setIntAttribute(u"download_id", it->download_id, true);
            e->setIntAttribute(u"module_id", it->module_id, true);
            e->setIntAttribute(u"application_id", it->application_id, true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::EASAudioFileDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"file");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute<uint8_t>(entry.audio_format, u"audio_format", true, 0, 0, 127) &&
             children[i]->getAttribute(entry.file_name, u"file_name", false) &&
             children[i]->getIntAttribute<uint8_t>(entry.audio_source, u"audio_source", true);
        if (ok) {
            if (entry.audio_source == 0x01) {
                ok = children[i]->getIntAttribute<uint16_t>(entry.program_number, u"program_number", true) &&
                     children[i]->getIntAttribute<uint32_t>(entry.carousel_id, u"carousel_id", true) &&
                     children[i]->getIntAttribute<uint16_t>(entry.application_id, u"application_id", true);
            }
            else if (entry.audio_source == 0x02) {
                ok = children[i]->getIntAttribute<uint16_t>(entry.program_number, u"program_number", true) &&
                     children[i]->getIntAttribute<uint32_t>(entry.download_id, u"download_id", true) &&
                     children[i]->getIntAttribute<uint32_t>(entry.module_id, u"module_id", true) &&
                     children[i]->getIntAttribute<uint16_t>(entry.application_id, u"application_id", true);
            }
        }
        entries.push_back(entry);
    }
    return ok;
}
