//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPMT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

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
    streams(this, true)
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

ts::PMT::Stream::Stream(const AbstractTable* table, uint8_t type) :
    EntryWithDescriptors(table),
    stream_type(type)
{
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
    // Get fixed part.
    service_id = section.tableIdExtension();
    pcr_pid = buf.getPID();

    // Get program-level descriptor list.
    buf.getDescriptorListWithLength(descs);

    // Get elementary streams description
    while (buf.canRead()) {
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

void ts::PMT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Build the section. Note that a PMT is not allowed to use more than
    // one section, see ISO/IEC 13818-1:2000 2.4.4.8 & 2.4.4.9. For the sake
    // of completeness, we allow multi-section PMT for very large services.

    // Fixed part, to be repeated on all sections.
    buf.putPID(pcr_pid);
    buf.pushState();

    // Insert program_info descriptor list (with leading length field).
    // Add new section when the descriptor list overflows.
    for (size_t start = 0;;) {
        start = buf.putPartialDescriptorListWithLength(descs, start);
        if (buf.error() || start >= descs.size()) {
            break;
        }
        else {
            addOneSection(table, buf);
        }
    }

    // Minimum size of a section: fixed part and empty program-level descriptor list.
    constexpr size_t payload_min_size = 4;

    // Order of serialization.
    std::vector<PID> pids;
    streams.getOrder(pids);

    // Add description of all elementary streams.
    for (PID pid : pids) {
        const auto& stream(streams[pid]);

        // Binary size of the stream entry.
        const size_t entry_size = 5 + stream.descs.binarySize();

        // If the current entry does not fit into the section, create a new section, unless we are at the beginning of the section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            addOneSection(table, buf);
            buf.putPartialDescriptorListWithLength(descs, 0, 0);
        }

        // Insert stream entry
        buf.putUInt8(stream.stream_type);
        buf.putPID(pid);
        buf.putPartialDescriptorListWithLength(stream.descs);
    }
}


//----------------------------------------------------------------------------
// Check if an elementary stream carries video.
//----------------------------------------------------------------------------

bool ts::PMT::Stream::isVideo(const DuckContext& duck) const
{
    return StreamTypeIsVideo(stream_type) || CodecTypeIsVideo(getCodec(duck));
}


//----------------------------------------------------------------------------
// Check if an elementary stream carries audio.
//----------------------------------------------------------------------------

bool ts::PMT::Stream::isAudio(const DuckContext& duck) const
{
    // Check obvious audio stream types.
    if (StreamTypeIsAudio(stream_type)) {
        return true;
    }

    // Check code type.
    const CodecType codec = getCodec(duck);
    if (codec != CodecType::UNDEFINED) {
        return CodecTypeIsAudio(codec);
    }

    // Look for ISDB audio component (unspecified codec).
    if (bool(duck.standards() & Standards::ISDB) && descs.search(DID_ISDB_AUDIO_COMP) < descs.count()) {
        return true;
    }

    return false;
}


//----------------------------------------------------------------------------
// Check if an elementary stream carries subtitles.
//----------------------------------------------------------------------------

bool ts::PMT::Stream::isSubtitles(const DuckContext& duck) const
{
    const bool atsc = bool(duck.standards() & Standards::ATSC);

    for (size_t index = 0; index < descs.count(); ++index) {
        const DescriptorPtr& dsc(descs[index]);
        if (!dsc.isNull() && dsc->isValid()) {
            const DID did = dsc->tag();
            if (did == DID_SUBTITLING || (atsc && did == DID_ATSC_CAPTION)) {
                // Always indicate a subtitle stream.
                return true;
            }
            if (did == DID_TELETEXT || did == DID_VBI_TELETEXT) {
                // A teletext descriptor may indicate subtitles, need to check the teletext type.
                const uint8_t* data = dsc->payload();
                size_t size = dsc->payloadSize();
                // Loop on all language entries, check if teletext type is a subtitle
                while (size >= 5) {
                    const uint8_t ttype = data[3] >> 3;
                    if (ttype == 0x02 || ttype == 0x05) {
                        return true; // teletext subtitles types
                    }
                    data += 5;
                    size -= 5;
                }
            }
        }
    }

    return false;
}


//----------------------------------------------------------------------------
// Get the PID class of the stream.
//----------------------------------------------------------------------------

ts::PIDClass ts::PMT::Stream::getClass(const DuckContext& duck) const
{
    if (isVideo(duck)) {
        return PIDClass::VIDEO;
    }
    else if (isAudio(duck)) {
        return PIDClass::AUDIO;
    }
    else if (isSubtitles(duck)) {
        return PIDClass::SUBTITLES;
    }
    else {
        return PIDClass::DATA;
    }
}


//----------------------------------------------------------------------------
// Try to determine the codec which is used in the stream.
//----------------------------------------------------------------------------

ts::CodecType ts::PMT::Stream::getCodec(const DuckContext& duck) const
{
    const bool atsc = bool(duck.standards() & Standards::ATSC);

    // Try classes of stream types.
    if (StreamTypeIsAVC(stream_type)) {
        return CodecType::AVC;
    }
    else if (StreamTypeIsHEVC(stream_type)) {
        return CodecType::HEVC;
    }
    else if (StreamTypeIsVVC(stream_type)) {
        return CodecType::VVC;
    }

    // Try specific values of stream type.
    switch (stream_type) {
        case ST_MPEG1_AUDIO:
            return CodecType::MPEG1_AUDIO;
        case ST_MPEG1_VIDEO:
            return CodecType::MPEG1_VIDEO;
        case ST_MPEG2_AUDIO:
            return CodecType::MPEG2_AUDIO;
        case ST_MPEG2_VIDEO:
        case ST_MPEG2_3D_VIEW:
            return CodecType::MPEG2_VIDEO;
        case ST_MPEG4_AUDIO:
        case ST_MPEG4_AUDIO_RAW:
            return CodecType::HEAAC; // ISO 14496-3
        case ST_MPEG4_VIDEO:
            return CodecType::MPEG4_VIDEO;
        case ST_AAC_AUDIO:
            return CodecType::AAC;
        case ST_J2K_VIDEO:
            return CodecType::J2K;
        case ST_AC3_AUDIO:
            if (atsc) {
                return CodecType::AC3;
            }
            break;
        case ST_EAC3_AUDIO:
            if (atsc) {
                return CodecType::EAC3;
            }
            break;
        default:
            break;
    }

    // Look up descriptors until one indicates something useful.
    for (size_t index = 0; index < descs.count(); ++index) {
        const PDS pds = descs.privateDataSpecifier(index);
        const DescriptorPtr& dsc(descs[index]);
        if (!dsc.isNull() && dsc->isValid()) {
            switch (dsc->tag()) {
                case DID_AVC_VIDEO:
                    return CodecType::AVC;
                case DID_HEVC_VIDEO:
                    return CodecType::HEVC;
                case DID_VVC_VIDEO:
                    return CodecType::VVC;
                case DID_EVC_VIDEO:
                    return CodecType::EVC;
                case DID_MPEG4_VIDEO:
                    return CodecType::MPEG4_VIDEO;
                case DID_J2K_VIDEO:
                    return CodecType::J2K;
                case DID_DTS:
                    return CodecType::DTS;
                case DID_AC3:
                    return CodecType::AC3;
                case DID_ENHANCED_AC3:
                    return CodecType::EAC3;
                case DID_AAC:
                case DID_MPEG2_AAC_AUDIO:
                    return CodecType::AAC;
                case DID_MPEG4_AUDIO:
                case DID_MPEG4_AUDIO_EXT:
                    return CodecType::HEAAC; // ISO 14496-3
                case DID_SUBTITLING:
                    return CodecType::DVB_SUBTITLES;
                case DID_TELETEXT:
                case DID_VBI_TELETEXT:
                    return CodecType::TELETEXT;
                case DID_AVS3_VIDEO:
                    if (pds == PDS_AVS) {
                        return CodecType::AVS3;
                    }
                    break;
                case DID_MPEG_EXTENSION: {
                    // Lookup extended tag.
                    if (dsc->payloadSize() >= 1) {
                        switch (dsc->payload()[0]) {
                            case MPEG_EDID_HEVC_TIM_HRD:
                            case MPEG_EDID_HEVC_OP_POINT:
                            case MPEG_EDID_HEVC_HIER_EXT:
                                return CodecType::HEVC;
                            case MPEG_EDID_VVC_TIM_HRD:
                                return CodecType::VVC;
                            case MPEG_EDID_EVC_TIM_HRD:
                                return CodecType::EVC;
                            case MPEG_EDID_LCEVC_VIDEO:
                            case MPEG_EDID_LCEVC_LINKAGE:
                                return CodecType::LCEVC;
                            default:
                                break;
                        }
                    }
                    break;
                }
                case DID_DVB_EXTENSION: {
                    // Lookup extended tag.
                    if (dsc->payloadSize() >= 1) {
                        switch (dsc->payload()[0]) {
                            case EDID_DTS_NEURAL:
                                return CodecType::DTS;
                            case EDID_DTS_HD_AUDIO:
                                return CodecType::DTSHD;
                            case EDID_AC4:
                                return CodecType::AC4;
                            case EDID_VVC_SUBPICTURES:
                                return CodecType::VVC;
                            default:
                                break;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    // Finally unknown.
    return CodecType::UNDEFINED;
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
    for (const auto& it : streams) {
        const PID pid = it.first;
        const PMT::Stream& stream(it.second);
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

ts::PID ts::PMT::firstVideoPID(const DuckContext& duck) const
{
    for (const auto& it : streams) {
        if (it.second.isVideo(duck)) {
            return it.first;
        }
    }
    return PID_NULL; // not found
}


//----------------------------------------------------------------------------
// Search the first format identifier in a registration descriptor.
//----------------------------------------------------------------------------

uint32_t ts::PMT::registrationId(PID pid) const
{
    size_t index = 0;

    // Search in specified component.
    const auto it = streams.find(pid);
    if (it != streams.end() && (index = it->second.descs.search(DID_REGISTRATION)) != it->second.descs.count() && it->second.descs[index]->payloadSize() >= 4) {
        return GetUInt32(it->second.descs[index]->payload());
    }

    // Search at program level.
    if ((index = descs.search(DID_REGISTRATION)) != descs.count() && descs[index]->payloadSize() >= 4) {
        return GetUInt32(descs[index]->payload());
    }

    // Not found.
    return REGID_NULL;
}


//----------------------------------------------------------------------------
// A static method to display a PMT section.
//----------------------------------------------------------------------------

void ts::PMT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    const PID pcr_pid = buf.getPID();
    disp << margin << UString::Format(u"Program: %d (0x%<X), PCR PID: ", {section.tableIdExtension()})
         << (pcr_pid == PID_NULL ? u"none" : UString::Format(u"%d (0x%<X)", {pcr_pid}))
         << std::endl;

    // Process and display "program info" descriptors. Get registration id.
    disp.duck().resetRegistrationIds();
    disp.displayDescriptorListWithLength(section, buf, margin, u"Program information:");

    // Get elementary streams description
    while (buf.canRead()) {
        const uint8_t type = buf.getUInt8();
        const PID pid = buf.getPID();
        disp << margin << "Elementary stream: type " << names::StreamType(type, NamesFlags::FIRST, disp.duck().lastRegistrationId())
             << UString::Format(u", PID: %d (0x%<X)", {pid}) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin);
    }
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

    // Order of serialization.
    std::vector<PID> pids;
    streams.getOrder(pids);

    // Add description of all elementary streams.
    for (PID pid : pids) {
        const auto& stream(streams[pid]);
        xml::Element* e = root->addElement(u"component");
        e->setIntAttribute(u"elementary_PID", pid, true);
        e->setIntAttribute(u"stream_type", stream.stream_type, true);
        stream.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::PMT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<PID>(pcr_pid, u"PCR_PID", false, PID_NULL, 0x0000, 0x1FFF) &&
        descs.fromXML(duck, children, element, u"component");

    for (auto e : children) {
        PID pid = PID_NULL;
        ok = e->getIntAttribute<PID>(pid, u"elementary_PID", true, 0, 0x0000, 0x1FFF);
        if (ok) {
            if (Contains(streams, pid)) {
                element->report().error(u"line %d: in <%s>, duplicated <%s> for PID 0x%X (%<d)", {e->lineNumber(), element->name(), e->name(), pid});
                ok = false;
            }
            else {
                ok = e->getIntAttribute(streams[pid].stream_type, u"stream_type", true) && streams[pid].descs.fromXML(duck, e);
            }
        }
    }
    return ok;
}
