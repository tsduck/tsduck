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

#include "tsPMT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"PMT"
#define MY_CLASS ts::PMT
#define MY_TID ts::TID_PMT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PMT::PMT(uint8_t version_, bool is_current_, uint16_t service_id_, PID pcr_pid_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    service_id(service_id_),
    pcr_pid(pcr_pid_),
    descs(this),
    streams(this)
{
}

ts::PMT::PMT(const PMT& other) :
    AbstractLongTable(other),
    service_id(other.service_id),
    pcr_pid(other.pcr_pid),
    descs(this, other.descs),
    streams(this, other.streams)
{
}

ts::PMT::PMT(DuckContext& duck, const BinaryTable& table) :
    PMT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

bool ts::PMT::isPrivate() const
{
    return false; // MPEG-defined
}

uint16_t ts::PMT::tableIdExtension() const
{
    return service_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::PMT::clearContent()
{
    service_id = 0;
    pcr_pid = PID_NULL;
    descs.clear();
    streams.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PMT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get fixed pard.
    service_id = section.tableIdExtension();
    pcr_pid = buf.getPID();

    // Get program-level descriptor list.
    buf.getDescriptorListWithLength(descs);

    // Get elementary streams description
    while (!buf.error() && !buf.endOfRead()) {
        const uint8_t type = buf.getUInt8();
        const PID pid = buf.getPID();
        Stream& str(streams[pid]);
        str.stream_type = type;
        buf.getDescriptorListWithLength(str.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PMT::serializePayload(BinaryTable& table, PSIBuffer& payload) const
{
    // Build the section. Note that a PMT is not allowed to use more than
    // one section, see ISO/IEC 13818-1:2000 2.4.4.8 & 2.4.4.9. For the sake
    // of completeness, we allow multi-section PMT for very large services.

    // Fixed part, to be repeated on all sections.
    payload.putPID(pcr_pid);
    payload.pushReadWriteState();

    // Insert program_info descriptor list (with leading length field).
    // Add new section when the descriptor list overflows.
    for (size_t start = 0;;) {
        start = payload.putPartialDescriptorListWithLength(descs, start);
        if (payload.error() || start >= descs.size()) {
            break;
        }
        else {
            addOneSection(table, payload);
        }
    }

    // Minimum size of a section: fixed part and empty program-level descriptor list.
    constexpr size_t payload_min_size = 4;

    // Add description of all elementary streams
    for (auto it = streams.begin(); it != streams.end(); ++it) {

        // Binary size of the stream entry.
        const size_t entry_size = 5 + it->second.descs.binarySize();

        // If the current entry does not fit into the section, create a new section, unless we are at the beginning of the section.
        if (entry_size > payload.remainingWriteBytes() && payload.currentWriteByteOffset() > payload_min_size) {
            addOneSection(table, payload);
            payload.putPartialDescriptorListWithLength(descs, 0, 0);
        }

        // Insert stream entry
        payload.putUInt8(it->second.stream_type);
        payload.putPID(it->first); // PID
        payload.putPartialDescriptorListWithLength(it->second.descs);
    }
}


//----------------------------------------------------------------------------
// Check if an elementary stream carries audio, video or subtitles.
//----------------------------------------------------------------------------

bool ts::PMT::Stream::isVideo() const
{
    return IsVideoST(stream_type) ||
        descs.search(DID_AVC_VIDEO) < descs.count() ||
        descs.search(DID_HEVC_VIDEO) < descs.count() ||
        descs.search(DID_MPEG4_VIDEO) < descs.count() ||
        descs.search(DID_J2K_VIDEO) < descs.count();
}

bool ts::PMT::Stream::isAudio() const
{
    // AC-3 or HE-AAC components may have "PES private data" stream type
    // but are identified by specific descriptors.

    return IsAudioST(stream_type) ||
        descs.search(DID_DTS) < descs.count() ||
        descs.search(DID_AC3) < descs.count() ||
        descs.search(DID_ENHANCED_AC3) < descs.count() ||
        descs.search(DID_AAC) < descs.count() ||
        descs.search(EDID::ExtensionDVB(EDID_AC4)) < descs.count() ||
        descs.search(EDID::ExtensionDVB(EDID_DTS_NEURAL)) < descs.count() ||
        descs.search(EDID::ExtensionDVB(EDID_DTS_HD_AUDIO)) < descs.count();
}

bool ts::PMT::Stream::isSubtitles() const
{
    // A subtitling descriptor always indicates subtitles.
    if (descs.search(DID_SUBTITLING) < descs.count()) {
        return true;
    }
    // A teletext descriptor may indicate subtitles
    for (size_t index = 0; (index = descs.search(DID_TELETEXT, index)) < descs.count(); ++index) {
        // Get descriptor payload
        const uint8_t* data = descs[index]->payload();
        size_t size = descs[index]->payloadSize();
        // Loop on all language entries, check if teletext type is a subtitle
        while (size >= 5) {
            uint8_t ttype = data[3] >> 3;
            if (ttype == 0x02 || ttype == 0x05) {
                return true; // teletext subtitles types
            }
            data += 5;
            size -= 5;
        }
    }
    // After all, no subtitle here...
    return false;
}


//----------------------------------------------------------------------------
// Look for a component tag in a stream_identifier_descriptor.
//----------------------------------------------------------------------------

bool ts::PMT::Stream::getComponentTag(uint8_t& tag) const
{
    // Loop on all stream_identifier_descriptors until a valid one is found.
    for (size_t i = descs.search(DID_STREAM_ID); i < descs.count(); i = descs.search(DID_STREAM_ID, i + 1)) {
        if (!descs[i].isNull() && descs[i]->payloadSize() >= 1) {
            // The payload of the stream_identifier_descriptor contains only one byte, the component tag.
            tag = descs[i]->payload()[0];
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Search the component PID for a given component tag.
//----------------------------------------------------------------------------

ts::PID ts::PMT::componentTagToPID(uint8_t tag) const
{
    // Loop on all components of the service.
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        const PID pid = it->first;
        const PMT::Stream& stream(it->second);
        // Loop on all stream_identifier_descriptors.
        for (size_t i = stream.descs.search(DID_STREAM_ID); i < stream.descs.count(); i = stream.descs.search(DID_STREAM_ID, i + 1)) {
            // The payload of the stream_identifier_descriptor contains only one byte, the component tag.
            if (!stream.descs[i].isNull() && stream.descs[i]->payloadSize() >= 1 && stream.descs[i]->payload()[0] == tag) {
                return pid;
            }
        }
    }
    return PID_NULL; // not found
}


//----------------------------------------------------------------------------
// Search the first video PID in the service.
//----------------------------------------------------------------------------

ts::PID ts::PMT::firstVideoPID() const
{
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        if (it->second.isVideo()) {
            return it->first;
        }
    }
    return PID_NULL; // not found
}


//----------------------------------------------------------------------------
// A static method to display a PMT section.
//----------------------------------------------------------------------------

void ts::PMT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    PSIBuffer buf(duck, section.payload(), section.payloadSize());

    // Fixed part.
    const PID pcr_pid = buf.getPID();
    strm << margin << UString::Format(u"Program: %d (0x%<X), PCR PID: ", {section.tableIdExtension()})
         << (pcr_pid == PID_NULL ? u"none" : UString::Format(u"%d (0x%<X)", {pcr_pid}))
         << std::endl;

    // Process and display "program info" descriptors.
    display.displayDescriptorListWithLength(section, buf, indent, u"Program information:");

    // Get elementary streams description
    while (!buf.error() && !buf.endOfRead()) {
        const uint8_t type = buf.getUInt8();
        const PID pid = buf.getPID();
        strm << margin << "Elementary stream: type " << names::StreamType(type, names::FIRST)
             << UString::Format(u", PID: %d (0x%<X)", {pid}) << std::endl;
        display.displayDescriptorListWithLength(section, buf, indent);
    }

    display.displayExtraData(buf, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PMT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"service_id", service_id, true);
    if (pcr_pid != PID_NULL) {
        root->setIntAttribute(u"PCR_PID", pcr_pid, true);
    }
    descs.toXML(duck, root);

    for (StreamMap::const_iterator it = streams.begin(); it != streams.end(); ++it) {
        xml::Element* e = root->addElement(u"component");
        e->setIntAttribute(u"elementary_PID", it->first, true);
        e->setIntAttribute(u"stream_type", it->second.stream_type, true);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::PMT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<PID>(pcr_pid, u"PCR_PID", false, PID_NULL, 0x0000, 0x1FFF) &&
        descs.fromXML(duck, children, element, u"component");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        PID pid = PID_NULL;
        ok = children[index]->getIntAttribute<PID>(pid, u"elementary_PID", true, 0, 0x0000, 0x1FFF) &&
             children[index]->getIntAttribute<uint8_t>(streams[pid].stream_type, u"stream_type", true, 0, 0x00, 0xFF) &&
             streams[pid].descs.fromXML(duck, children[index]);
    }
    return ok;
}
