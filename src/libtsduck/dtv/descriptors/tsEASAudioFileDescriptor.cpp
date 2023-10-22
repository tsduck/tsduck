//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEASAudioFileDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EASAudioFileDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(uint8_t(entries.size()));
    for (const auto& it : entries) {
        buf.pushWriteSequenceWithLeadingLength(8); // loop_length
        buf.putBit(!it.file_name.empty());
        buf.putBits(it.audio_format, 7);
        if (!it.file_name.empty()) {
            buf.putUTF8WithLength(it.file_name);
        }
        buf.putUInt8(it.audio_source);
        if (it.audio_source == 0x01) {
            buf.putUInt16(it.program_number);
            buf.putUInt32(it.carousel_id);
            buf.putUInt16(it.application_id);
        }
        else if (it.audio_source == 0x02) {
            buf.putUInt16(it.program_number);
            buf.putUInt32(it.download_id);
            buf.putUInt32(it.module_id);
            buf.putUInt16(it.application_id);
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

void ts::EASAudioFileDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const size_t number_of_audio_sources = buf.getUInt8();
        disp << margin << UString::Format(u"Number of audio sources: %d", {number_of_audio_sources}) << std::endl;
        for (size_t i = 0; i < number_of_audio_sources && buf.canReadBytes(1); ++i) {
            buf.pushReadSizeFromLength(8); // loop_length
            if (buf.canReadBytes(1)) {
                const bool file_name_present = buf.getBool();
                disp << margin << "- Audio format: " << DataName(MY_XML_NAME, u"Format", buf.getBits<uint8_t>(7), NamesFlags::VALUE) << std::endl;
                if (file_name_present && buf.canReadBytes(1)) {
                    disp << margin << "  File name: \"" << buf.getUTF8WithLength() << "\"" << std::endl;
                }
                if (buf.canReadBytes(1)) {
                    const uint8_t audio_source = buf.getUInt8();
                    disp << margin << "  Audio source: " << DataName(MY_XML_NAME, u"Source", audio_source, NamesFlags::VALUE) << std::endl;
                    if (audio_source == 0x01 && buf.canReadBytes(8)) {
                        disp << margin << UString::Format(u"  Program number: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                        disp << margin << UString::Format(u"  Carousel id: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
                        disp << margin << UString::Format(u"  Application id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                    }
                    else if (audio_source == 0x02 && buf.canReadBytes(12)) {
                        disp << margin << UString::Format(u"  Program number: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                        disp << margin << UString::Format(u"  Download id: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
                        disp << margin << UString::Format(u"  Module id: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
                        disp << margin << UString::Format(u"  Application id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                    }
                }
            }
            // Unused part of loop instance, if any.
            disp.displayPrivateData(u"Extraneous data", buf, NPOS, margin + u"  ");
            buf.popState(); // end of loop_length;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EASAudioFileDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"file");
        e->setIntAttribute(u"audio_format", it.audio_format, true);
        if (!it.file_name.empty()) {
            e->setAttribute(u"file_name", it.file_name);
        }
        e->setIntAttribute(u"audio_source", it.audio_source, true);
        if (it.audio_source == 0x01) {
            e->setIntAttribute(u"program_number", it.program_number, true);
            e->setIntAttribute(u"carousel_id", it.carousel_id, true);
            e->setIntAttribute(u"application_id", it.application_id, true);
        }
        else if (it.audio_source == 0x02) {
            e->setIntAttribute(u"program_number", it.program_number, true);
            e->setIntAttribute(u"download_id", it.download_id, true);
            e->setIntAttribute(u"module_id", it.module_id, true);
            e->setIntAttribute(u"application_id", it.application_id, true);
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
