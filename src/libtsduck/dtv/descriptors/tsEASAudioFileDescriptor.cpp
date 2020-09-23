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
#include "tsPSIBuffer.h"
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

void ts::EASAudioFileDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(uint8_t(entries.size()));
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        buf.pushWriteSequenceWithLeadingLength(8); // loop_length
        buf.putBit(!it->file_name.empty());
        buf.putBits(it->audio_format, 7);
        if (!it->file_name.empty()) {
            buf.putUTF8WithLength(it->file_name);
        }
        buf.putUInt8(it->audio_source);
        if (it->audio_source == 0x01) {
            buf.putUInt16(it->program_number);
            buf.putUInt32(it->carousel_id);
            buf.putUInt16(it->application_id);
        }
        else if (it->audio_source == 0x02) {
            buf.putUInt16(it->program_number);
            buf.putUInt32(it->download_id);
            buf.putUInt32(it->module_id);
            buf.putUInt16(it->application_id);
        }
        buf.popState(); // update loop_length;
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EASAudioFileDescriptor::deserializePayload(PSIBuffer& buf)
{
    const size_t number_of_audio_sources = buf.getUInt8();
    for (size_t i = 0; i < number_of_audio_sources && buf.canRead(); ++i) {
        Entry entry;
        buf.pushReadSizeFromLength(8); // loop_length
        const bool file_name_present = buf.getBool();
        buf.getBits(entry.audio_format, 7);
        if (file_name_present) {
            buf.getUTF8WithLength(entry.file_name);
        }
        entry.audio_source = buf.getUInt8();
        if (entry.audio_source == 0x01) {
            entry.program_number = buf.getUInt16();
            entry.carousel_id = buf.getUInt32();
            entry.application_id = buf.getUInt16();
        }
        else if (entry.audio_source == 0x02) {
            entry.program_number = buf.getUInt16();
            entry.download_id = buf.getUInt32();
            entry.module_id = buf.getUInt32();
            entry.application_id = buf.getUInt16();
        }
        buf.popState(); // end of loop_length;
        entries.push_back(entry);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EASAudioFileDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size > 0) {
        // Number of audio sources.
        size_t count = data[0];
        data++; size--;
        disp << margin << UString::Format(u"Number of audio sources: %d", {count}) << std::endl;

        while (count-- > 0) {
            size_t loop_length = size == 0 ? 0 : data[0];
            if (loop_length < 2 || size < 1 + loop_length) {
                break; // loop instance does not fit here
            }
            const bool file_name_present = (data[1] & 0x80) != 0;
            const uint8_t audio_format = data[1] & 0x7F;
            data += 2; size -= 2;
            loop_length--;

            disp << margin << "- Audio format: " << NameFromSection(u"EASAudioFormat", audio_format, names::VALUE) << std::endl;

            if (file_name_present) {
                if (loop_length == 0 || 1 + size_t(data[0]) > loop_length) {
                    break;
                }
                const size_t file_name_length = data[0];
                disp << margin << "  File name: \"" << std::string(reinterpret_cast<const char*>(data + 1), file_name_length) << "\"" << std::endl;
                data += 1 + file_name_length; size -= 1 + file_name_length;
                loop_length -= 1 + file_name_length;
            }
            if (loop_length < 1) {
                break;
            }

            const uint8_t audio_source = data[0];
            data++; size--;
            loop_length--;

            disp << margin << "  Audio source: " << NameFromSection(u"EASAudioSource", audio_source, names::VALUE) << std::endl;

            if (audio_source == 0x01) {
                if (loop_length < 8) {
                    break;
                }
                disp << margin << UString::Format(u"  Program number: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                     << margin << UString::Format(u"  Carousel id: 0x%X (%d)", {GetUInt32(data + 2), GetUInt32(data + 2)}) << std::endl
                     << margin << UString::Format(u"  Application id: 0x%X (%d)", {GetUInt16(data + 6), GetUInt16(data + 6)}) << std::endl;
                data += 8; size -= 8;
                loop_length -= 8;
            }
            else if (audio_source == 0x02) {
                if (loop_length < 12) {
                    break;
                }
                disp << margin << UString::Format(u"  Program number: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                     << margin << UString::Format(u"  Download id: 0x%X (%d)", {GetUInt32(data + 2), GetUInt32(data + 2)}) << std::endl
                     << margin << UString::Format(u"  Module id: 0x%X (%d)", {GetUInt32(data + 6), GetUInt32(data + 6)}) << std::endl
                     << margin << UString::Format(u"  Application id: 0x%X (%d)", {GetUInt16(data + 10), GetUInt16(data + 10)}) << std::endl;
                data += 12; size -= 12;
                loop_length -= 12;
            }

            // Unused part of loop instance, if any.
            disp.displayExtraData(data, loop_length, margin + u"  ");
            data += loop_length; size -= loop_length;
        }
    }

    disp.displayExtraData(data, size, margin);
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
        ok = children[i]->getIntAttribute(entry.audio_format, u"audio_format", true, 0, 0, 127) &&
             children[i]->getAttribute(entry.file_name, u"file_name", false) &&
             children[i]->getIntAttribute(entry.audio_source, u"audio_source", true);
        if (ok) {
            if (entry.audio_source == 0x01) {
                ok = children[i]->getIntAttribute(entry.program_number, u"program_number", true) &&
                     children[i]->getIntAttribute(entry.carousel_id, u"carousel_id", true) &&
                     children[i]->getIntAttribute(entry.application_id, u"application_id", true);
            }
            else if (entry.audio_source == 0x02) {
                ok = children[i]->getIntAttribute(entry.program_number, u"program_number", true) &&
                     children[i]->getIntAttribute(entry.download_id, u"download_id", true) &&
                     children[i]->getIntAttribute(entry.module_id, u"module_id", true) &&
                     children[i]->getIntAttribute(entry.application_id, u"application_id", true);
            }
        }
        entries.push_back(entry);
    }
    return ok;
}
