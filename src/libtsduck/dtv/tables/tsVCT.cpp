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

#include "tsVCT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VCT::VCT(TID tid, const UChar* xml_name, Standards standards, uint8_t version_, bool is_current_) :
    AbstractLongTable(tid, xml_name, standards, version_, is_current_),
    protocol_version(0),
    transport_stream_id(0),
    channels(this),
    descs(this)
{
}

ts::VCT::VCT(const VCT& other) :
    AbstractLongTable(other),
    protocol_version(other.protocol_version),
    transport_stream_id(other.transport_stream_id),
    channels(this, other.channels),
    descs(this, other.descs)
{
}

ts::VCT::Channel::Channel(const AbstractTable* table) :
    EntryWithDescriptors(table),
    short_name(),
    major_channel_number(0),
    minor_channel_number(0),
    modulation_mode(0),
    carrier_frequency(0),
    channel_TSID(0),
    program_number(0),
    ETM_location(0),
    access_controlled(false),
    hidden(false),
    hide_guide(false),
    service_type(0),
    source_id(0),
    path_select(0),
    out_of_band(false)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::VCT::tableIdExtension() const
{
    return transport_stream_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::VCT::clearContent()
{
    protocol_version = 0;
    transport_stream_id = 0;
    channels.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Set all known values in a Service object.
//----------------------------------------------------------------------------

void ts::VCT::Channel::setService(Service& service) const
{
    service.setId(program_number);
    service.setTSId(channel_TSID);
    service.setName(short_name);
    service.setMajorIdATSC(major_channel_number);
    service.setMinorIdATSC(minor_channel_number);
    service.setTypeATSC(service_type);
    service.setCAControlled(access_controlled);
}


//----------------------------------------------------------------------------
// Search a service by name or id.
//----------------------------------------------------------------------------

ts::VCT::ChannelList::const_iterator ts::VCT::findService(uint16_t id, bool same_ts) const
{
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        if (!same_ts || it->second.channel_TSID == transport_stream_id) {
            if (id == it->second.program_number) {
                return it;
            }
        }
    }

    // Service not found
    return channels.end();
}

ts::VCT::ChannelList::const_iterator ts::VCT::findService(uint16_t major, uint16_t minor, bool same_ts) const
{
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        if (!same_ts || it->second.channel_TSID == transport_stream_id) {
            if (major == it->second.major_channel_number && minor == it->second.minor_channel_number) {
                return it;
            }
        }
    }

    // Service not found
    return channels.end();
}

ts::VCT::ChannelList::const_iterator ts::VCT::findService(const UString& name, bool exact_match, bool same_ts) const
{
    // Search using various interpretations of "name".
    Service service(name);
    return findServiceInternal(service, exact_match, same_ts);
}

bool ts::VCT::findService(Service& service, bool exact_match, bool same_ts) const
{
    return findServiceInternal(service, exact_match, same_ts) != channels.end();
}

ts::VCT::ChannelList::const_iterator ts::VCT::findServiceInternal(Service& service, bool exact_match, bool same_ts) const
{
    ChannelList::const_iterator srv = channels.end();
    if (service.hasId()) {
        // Search by service id.
        srv = findService(service.getId(), same_ts);
    }
    else if (service.hasMajorIdATSC() && service.hasMinorIdATSC()) {
        // Search by major.minor id.
        srv = findService(service.getMajorIdATSC(), service.getMinorIdATSC(), same_ts);
    }
    else if (service.hasName()) {
        // Search by service name.
        const UString name(service.getName());
        for (srv = channels.begin(); srv != channels.end(); ++srv) {
            if (!same_ts || srv->second.channel_TSID == transport_stream_id) {
                if ((exact_match && name == srv->second.short_name) || (!exact_match && name.similar(srv->second.short_name))) {
                    break;
                }
            }
        }
    }

    if (srv != channels.end()) {
        // Service found, set known fields.
        srv->second.setService(service);
    }

    return srv;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VCT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    protocol_version = 0;
    transport_stream_id = 0;
    descs.clear();
    channels.clear();

    // Loop on all sections.
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        transport_stream_id = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();
        if (remain < 2) {
            return; // invalid table, too short
        }

        // Get fixed fields.
        protocol_version = data[0];
        uint8_t num_channels = data[1];
        data += 2; remain -= 2;

        // Loop on all channel definitions.
        while (num_channels > 0 && remain >= 32) {
            // Add a new Channel at the end of the list.
            // We do not need to search for a similar channel to extend
            // because A/65 specifies that a channel shall fit in one section.
            Channel& ch(channels.newEntry());

            // The short name is at most 7 UTF-16 characters.
            for (size_t i = 0; i < 7; i++) {
                const uint16_t c = GetUInt16(data + 2*i);
                if (c == 0) {
                    break; // padding zeroes
                }
                else {
                    ch.short_name.push_back(UChar(c));
                }
            }

            // Other channel attributes.
            const uint32_t num = GetUInt24(data + 14);
            ch.major_channel_number = (num >> 10) & 0x03FF;
            ch.minor_channel_number = num & 0x03FF;
            ch.modulation_mode = data[17];
            ch.carrier_frequency = GetUInt32(data + 18);
            ch.channel_TSID = GetUInt16(data + 22);
            ch.program_number = GetUInt16(data + 24);
            const uint8_t flags = data[26];
            ch.ETM_location = (flags >> 6) & 0x03;
            ch.access_controlled = (flags & 0x20) != 0;
            ch.hidden = (flags & 0x10) != 0;
            if (_table_id == TID_CVCT) {
                // The following two bits are used in CVCT only.
                ch.path_select = (flags >> 3) & 0x01;
                ch.out_of_band = (flags & 0x04) != 0;
            }
            else {
                // Unused field in other forms of VCT.
                ch.path_select = 0;
                ch.out_of_band = false;
            }
            ch.hide_guide = (flags & 0x02) != 0;
            ch.service_type = data[27] & 0x3F;
            ch.source_id = GetUInt16(data + 28);

            // Descriptors for this channel.
            size_t info_length = GetUInt16(data + 30) & 0x03FF; // 10 bits only
            data += 32; remain -= 32;
            info_length = std::min(info_length, remain);
            ch.descs.add(data, info_length);
            data += info_length; remain -= info_length;
            num_channels--;
        }
        if (num_channels > 0 || remain < 2) {
            return; // truncated table.
        }

        // Get global descriptor list
        size_t info_length = GetUInt16(data) & 0x03FF; // 10 bits only
        data += 2; remain -= 2;
        info_length = std::min(info_length, remain);
        descs.add(data, info_length);
        data += info_length; remain -= info_length;
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Private method: Add a new section to a table being serialized.
// Section number is incremented. Data and remain are reinitialized.
//----------------------------------------------------------------------------

void ts::VCT::addSection(BinaryTable& table,
                                 int& section_number,
                                 uint8_t* payload,
                                 uint8_t*& data,
                                 size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,                 // is_private_section
                                 transport_stream_id,  // tid_ext
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number),   //last_section_number
                                 payload,
                                 data - payload)); // payload_size,

    // Reinitialize pointers.
    remain += data - payload;
    data = payload;
    section_number++;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VCT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections one by one.
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;
    uint8_t* data = payload;
    size_t remain = sizeof(payload);
    size_t channel_index = 0;  // index in list of channels in the VCT
    size_t next_desc = 0;  // next global descriptor to serialize.

    // Loop on the creation of sections until at least one secrion is created and
    // all channels are serialized and all global descriptors are serialized.
    while (section_number == 0 || channel_index < channels.size() || next_desc < descs.count()) {

        // Add fixed fields.
        data[0] = protocol_version;

        // Placeholder for num_channels_in_section
        uint8_t num_channels = 0;
        data += 2; remain -= 2;

        // Loop on channel definitions.
        while (channel_index < channels.size() && remain >= 34) {

            // Save current position in payload.
            uint8_t* const saved_data = data;
            const size_t saved_remain = remain;

            // Fixed fields for this channel.
            const Channel& ch(channels[channel_index]);

            // The short name is at most 7 UTF-16 characters, padded with zeroes.
            for (size_t i = 0; i < 7; i++) {
                PutUInt16(data + 2*i, i < ch.short_name.size() ? uint16_t(ch.short_name[i]) : 0);
            }

            // Other channel attributes.
            PutUInt24(data + 14, 0xF000 | ((ch.major_channel_number & 0x03FF) << 10) | (ch.minor_channel_number & 0x03FF));
            PutUInt8(data + 17, ch.modulation_mode);
            PutUInt32(data + 18, ch.carrier_frequency);
            PutUInt16(data + 22, ch.channel_TSID);
            PutUInt16(data + 24, ch.program_number);
            PutUInt8(data + 26, uint8_t(ch.ETM_location << 6) |
                                (ch.access_controlled ? 0x20 : 0x00) |
                                (ch.hidden ? 0x10 : 0x00) |
                                (_table_id != TID_CVCT ? 0x08 : uint8_t((ch.path_select & 0x01) << 3)) |
                                (_table_id != TID_CVCT || ch.out_of_band ? 0x04 : 0x00) |
                                (ch.hide_guide ? 0x02 : 0x00) |
                                0x01);
            PutUInt8(data + 27, 0xC0 | ch.service_type);
            PutUInt16(data + 28, ch.source_id);

            // Now try to serialize all descriptors from the channel.
            // Reserve 2 extra bytes at end, for the rest of the section.
            // Warning: the VCT has an unusual 10-bit size for the descriptor list length.
            data += 30;
            remain -= 32; // including 2 extra bytes at end
            const size_t next_index = ch.descs.lengthSerialize(data, remain, 0, 0x003F, 10);
            remain += 2; // restore space for the 2 extra bytes at end

            if (num_channels == 0 || next_index >= ch.descs.count()) {
                // This is the first channel in section or all descriptors for the channel were serialized.
                // Fine, keep this channel in the section.
                num_channels++;  // number of channels in this section
                channel_index++; // index in list of channels in VCT
            }
            else {
                // This is not the first channel in the section and all descriptors could not fit.
                // Drop this channel for this section, will be stored in next section.
                // Stop the current section here.
                data = saved_data;
                remain = saved_remain;
                break;
            }
        }

        // Now store the number of channels in this section.
        payload[1] = num_channels;

        // Store all or some global descriptors.
        // Warning: the VCT has an unusual 10-bit size for the descriptor list length.
        next_desc = descs.lengthSerialize(data, remain, next_desc, 0x003F, 10);

        // Add a new section in the table
        addSection(table, section_number, payload, data, remain);
    }
}


//----------------------------------------------------------------------------
// A static method to display a VCT section.
//----------------------------------------------------------------------------

void ts::VCT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 2) {
        // Fixed part.
        uint16_t num_channels = data[1];
        strm << margin << UString::Format(u"Transport stream id: 0x%X (%d)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl
             << margin << UString::Format(u"Protocol version: %d, number of channels: %d", {data[0], num_channels}) << std::endl;
        data += 2; size -= 2;

        // Loop on all channel definitions.
        while (num_channels > 0 && size >= 32) {
            // The short name is at most 7 UTF-16 characters.
            UString name;
            for (size_t i = 0; i < 7; i++) {
                const uint16_t c = GetUInt16(data + 2*i);
                if (c == 0) {
                    break; // padding zeroes
                }
                else {
                    name.push_back(UChar(c));
                }
            }
            const uint32_t num = GetUInt24(data + 14);
            const uint8_t flags = data[26];
            strm << margin << UString::Format(u"- Channel %d.%d, short name: \"%s\"", {(num >> 10) & 0x03FF, num & 0x03FF, name}) << std::endl
                 << margin << UString::Format(u"  Modulation: %s, frequency: %'d", {NameFromSection(u"ATSCModulationModes", data[17]), GetUInt32(data + 18)}) << std::endl
                 << margin << UString::Format(u"  TS id: 0x%X (%d), program number: 0x%X (%d)", {GetUInt16(data + 22), GetUInt16(data + 22), GetUInt16(data + 24), GetUInt16(data + 24)}) << std::endl
                 << margin << UString::Format(u"  ETM location: %d, access controlled: %s", {(flags >> 6) & 0x03, UString::YesNo((flags & 0x20) != 0)}) << std::endl;
            if (section.tableId() == TID_CVCT) {
                // The following two bits are used in CVCT only.
                strm << margin << UString::Format(u"  Path select: %d, out of band: %s", {(flags >> 3) & 0x01, UString::YesNo((flags & 0x04) != 0)}) << std::endl;
            }
            strm << margin << UString::Format(u"  Hidden: %s, hide guide: %s", {UString::YesNo((flags & 0x10) != 0), UString::YesNo((flags & 0x02) != 0)}) << std::endl
                 << margin << UString::Format(u"  Service type: %s, source id: 0x%X (%d)", {NameFromSection(u"ATSCServiceType", data[27] & 0x3F), GetUInt16(data + 28), GetUInt16(data + 28)}) << std::endl;

            // Descriptors for this channel. Use fake PDS for ATSC.
            size_t info_length = GetUInt16(data + 30) & 0x03FF; // 10 bits only
            data += 32; size -= 32;
            info_length = std::min(info_length, size);
            display.displayDescriptorList(section, data, info_length, indent + 2);
            data += info_length; size -= info_length;
            num_channels--;
        }
        if (num_channels == 0 && size >= 2) {
            // Common descriptors. Use fake PDS for ATSC.
            size_t info_length = GetUInt16(data) & 0x03FF; // 10 bits only
            data += 2; size -= 2;
            info_length = std::min(info_length, size);
            if (info_length > 0) {
                strm << margin << "- Global descriptors:" << std::endl;
                display.displayDescriptorList(section, data, info_length, indent + 2);
                data += info_length; size -= info_length;
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML values for modulation mode and service_type.
//----------------------------------------------------------------------------

const ts::Enumeration ts::VCT::ModulationModeEnum({
    {u"analog",  0x01},
    {u"64-QAM",  0x02},
    {u"256-QAM", 0x03},
    {u"8-VSB",   0x04},
    {u"16-VSB",  0x05},
});

const ts::Enumeration ts::VCT::ServiceTypeEnum({
    {u"analog",   0x01},
    {u"dtv",      0x02},
    {u"audio",    0x03},
    {u"data",     0x04},
    {u"software", 0x05},
});


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VCT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"protocol_version", protocol_version);
    descs.toXML(duck, root);

    for (auto it = channels.begin(); it != channels.end(); ++it) {
        xml::Element* e = root->addElement(u"channel");
        e->setAttribute(u"short_name", it->second.short_name);
        e->setIntAttribute(u"major_channel_number", it->second.major_channel_number, false);
        e->setIntAttribute(u"minor_channel_number", it->second.minor_channel_number, false);
        e->setEnumAttribute(ModulationModeEnum, u"modulation_mode", it->second.modulation_mode);
        e->setIntAttribute(u"carrier_frequency", it->second.carrier_frequency, false);
        e->setIntAttribute(u"channel_TSID", it->second.channel_TSID, true);
        e->setIntAttribute(u"program_number", it->second.program_number, true);
        e->setIntAttribute(u"ETM_location", it->second.ETM_location, false);
        e->setBoolAttribute(u"access_controlled", it->second.access_controlled);
        e->setBoolAttribute(u"hidden", it->second.hidden);
        if (_table_id == TID_CVCT) {
            // CVCT-specific fields.
            e->setIntAttribute(u"path_select", it->second.path_select, false);
            e->setBoolAttribute(u"out_of_band", it->second.out_of_band);
        }
        e->setBoolAttribute(u"hide_guide", it->second.hide_guide);
        e->setEnumAttribute(ServiceTypeEnum, u"service_type", it->second.service_type);
        e->setIntAttribute(u"source_id", it->second.source_id, true);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VCT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute<uint16_t>(transport_stream_id, u"transport_stream_id", true) &&
        descs.fromXML(duck, children, element, u"channel");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        // Add a new Channel at the end of the list.
        Channel& ch(channels.newEntry());
        ok = children[index]->getAttribute(ch.short_name, u"short_name", true, UString(), 0, 7) &&
             children[index]->getIntAttribute<uint16_t>(ch.major_channel_number, u"major_channel_number", true, 0, 0, 0x03FF) &&
             children[index]->getIntAttribute<uint16_t>(ch.minor_channel_number, u"minor_channel_number", true, 0, 0, 0x03FF) &&
             children[index]->getIntEnumAttribute(ch.modulation_mode, ModulationModeEnum, u"modulation_mode", true) &&
             children[index]->getIntAttribute<uint32_t>(ch.carrier_frequency, u"carrier_frequency", false, 0) &&
             children[index]->getIntAttribute<uint16_t>(ch.channel_TSID, u"channel_TSID", true) &&
             children[index]->getIntAttribute<uint16_t>(ch.program_number, u"program_number", true) &&
             children[index]->getIntAttribute<uint8_t>(ch.ETM_location, u"ETM_location", false, 0, 0x00, 0x03) &&
             children[index]->getBoolAttribute(ch.access_controlled, u"access_controlled", false, false) &&
             children[index]->getBoolAttribute(ch.hidden, u"hidden", false, false) &&
             children[index]->getBoolAttribute(ch.hide_guide, u"hide_guide", false, false) &&
             children[index]->getIntEnumAttribute<uint8_t>(ch.service_type, ServiceTypeEnum, u"service_type", false, ATSC_STYPE_DTV) &&
             children[index]->getIntAttribute<uint16_t>(ch.source_id, u"source_id", true) &&
             ch.descs.fromXML(duck, children[index]);

        if (ok && _table_id == TID_CVCT) {
            // CVCT-specific fields.
            ok = children[index]->getIntAttribute<uint8_t>(ch.path_select, u"path_select", false, 0, 0, 1) &&
                 children[index]->getBoolAttribute(ch.out_of_band, u"out_of_band", false, false);
        }
    }
    return ok;
}
