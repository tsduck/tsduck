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

#include "tsSpliceInformationTable.h"
#include "tsBinaryTable.h"
#include "tsNames.h"
#include "tsxmlElement.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"splice_information_table"
#define MY_CLASS ts::SpliceInformationTable
#define MY_TID ts::TID_SCTE35_SIT
#define MY_STD ts::Standards::SCTE

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SpliceInformationTable::SpliceInformationTable() :
    AbstractTable(MY_TID, MY_XML_NAME, MY_STD),
    protocol_version(0),
    pts_adjustment(0),
    tier(0x0FFF),
    splice_command_type(SPLICE_NULL),
    splice_schedule(),
    splice_insert(),
    time_signal(),
    private_command(),
    descs(this)
{
}

ts::SpliceInformationTable::SpliceInformationTable(const SpliceInformationTable& other) :
    AbstractTable(other),
    protocol_version(other.protocol_version),
    pts_adjustment(other.pts_adjustment),
    tier(other.tier),
    splice_command_type(other.splice_command_type),
    splice_schedule(other.splice_schedule),
    splice_insert(other.splice_insert),
    time_signal(other.time_signal),
    private_command(other.private_command),
    descs(this, other.descs)
{
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

bool ts::SpliceInformationTable::isPrivate() const
{
    // Although not MPEG-defined, SCTE section are "non private".
    return false;
}


//----------------------------------------------------------------------------
// Clear all fields.
//----------------------------------------------------------------------------

void ts::SpliceInformationTable::clearContent()
{
    protocol_version = 0;
    pts_adjustment = 0;
    tier = 0x0FFF;
    splice_command_type = SPLICE_NULL;
    splice_schedule.clear();
    splice_insert.clear();
    time_signal.clear();
    private_command.identifier = 0;
    private_command.private_bytes.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Adjust PTS time values using the "PTS adjustment".
//----------------------------------------------------------------------------

void ts::SpliceInformationTable::adjustPTS()
{
    // Only splice_insert() commands need adjustment.
    if (splice_command_type == SPLICE_INSERT) {
        splice_insert.adjustPTS(pts_adjustment);
    }

    // Adjustment applied, don't do it again.
    pts_adjustment = 0;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::SpliceInformationTable::SpliceInformationTable(DuckContext& duck, const BinaryTable& table) :
    SpliceInformationTable()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SpliceInformationTable::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    clear();

    // This is a short table, must have only one section
    if (table.sectionCount() != 1) {
        return;
    }

    // Reference to single section
    const Section& sect(*table.sectionAt(0));
    const uint8_t* data = sect.payload();
    size_t remain = sect.payloadSize();

    // Payload layout: fixed part (11 bytes), variable part, CRC2 (4 bytes).
    // There is a CRC32 at the end of a SpliceInformationTable, even though we are in a short section.
    if (remain < 15) {
        return;
    }

    // Check CRC32 now
    const CRC32 comp_crc32(sect.content(), sect.size() - 4);
    const uint32_t sect_crc32 = GetUInt32(data + remain - 4);
    remain -= 4;

    if (sect_crc32 != comp_crc32) {
        return;
    }

    // Fixed part.
    protocol_version = data[0];
    bool encrypted = (data[1] & 0x80) != 0;
    pts_adjustment = (uint64_t(data[1] & 0x01) << 32) | uint64_t(GetUInt32(data + 2));
    tier = (GetUInt16(data + 7) >> 4) & 0x0FFF;
    const size_t command_length = GetUInt16(data + 8) & 0x0FFF;
    splice_command_type = data[10];
    data += 11; remain -= 11;

    // Encrypted sections cannot be deserialized.
    if (encrypted) {
        return;
    }

    // Decode splice command (and then 2 bytes for descriptor loop size).
    if (command_length + 2 > remain) {
        return;
    }
    switch (splice_command_type) {
        case SPLICE_NULL:
        case SPLICE_BANDWIDTH_RESERVATION:
            // These commands are empty.
            break;
        case SPLICE_SCHEDULE:
            if (splice_schedule.deserialize(data, command_length) < 0) {
                return;
            }
            break;
        case SPLICE_INSERT:
            if (splice_insert.deserialize(data, command_length) < 0) {
                return;
            }
            break;
        case SPLICE_TIME_SIGNAL:
            if (time_signal.deserialize(data, command_length) < 0) {
                return;
            }
            break;
        case SPLICE_PRIVATE_COMMAND:
            if (command_length < 4) {
                return;
            }
            private_command.identifier = GetUInt32(data);
            private_command.private_bytes.copy(data + 4, command_length - 4);
            break;
        default:
            // Invalid command.
            break;
    }
    data += command_length; remain -= command_length;

    // Process descriptor list.
    const uint16_t dlength = GetUInt16(data);
    data += 2; remain -= 2;
    if (dlength > remain) {
        return;
    }
    descs.add(data, dlength);

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SpliceInformationTable::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section header.
    ByteBlockPtr bb(new ByteBlock);
    bb->appendUInt8(MY_TID);
    bb->appendUInt16(0); // placeholder for section length

    // Splice info section fixed part.
    bb->appendUInt8(protocol_version);
    bb->appendUInt8(uint8_t(pts_adjustment >> 32) & 0x01);
    bb->appendUInt32(uint32_t(pts_adjustment));
    bb->appendUInt8(0); // cw_index
    bb->appendUInt16(uint16_t(tier << 4));
    bb->appendUInt8(0); // placeholder for command length
    bb->appendUInt8(splice_command_type);

    // Serialize the splice command.
    size_t start = bb->size();
    switch (splice_command_type) {
        case SPLICE_NULL:
        case SPLICE_BANDWIDTH_RESERVATION:
            // These commands are empty.
            break;
        case SPLICE_SCHEDULE:
            splice_schedule.serialize(*bb);
            break;
        case SPLICE_INSERT:
            splice_insert.serialize(*bb);
            break;
        case SPLICE_TIME_SIGNAL:
            time_signal.serialize(*bb);
            break;
        case SPLICE_PRIVATE_COMMAND:
            bb->appendUInt32(private_command.identifier);
            bb->append(private_command.private_bytes);
            break;
        default:
            // Invalid command.
            break;
    }

    // Adjust the command length.
    PutUInt16(bb->data() + 11, uint16_t(uint16_t((*bb)[11]) << 8) | (uint16_t(bb->size() - start) & 0x0FFF));

    // Descriptor loop.
    start = bb->size();
    bb->appendUInt16(0); // placeholder for descriptors length.
    size_t dlength = descs.serialize(*bb);
    PutUInt16(bb->data() + start, uint16_t(dlength));

    // Placeholder for CRC32.
    bb->appendUInt32(0);

    // Update section length.
    // section_syntax = 0, private_indicator = 0
    PutUInt16(bb->data() + 1, 0x3000 | ((bb->size() - 3) & 0x0FFF));

    // Compute and update CRC32.
    // This will not be done by the Section class since this is a short section.
    PutUInt32(bb->data() + bb->size() - 4, CRC32(bb->data(), bb->size() - 4).value());

    // Build the section.
    SectionPtr section(new Section(bb));
    table.addSection(section);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SpliceInformationTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"protocol_version", protocol_version, false);
    root->setIntAttribute(u"pts_adjustment", pts_adjustment, false);
    root->setIntAttribute(u"tier", tier, true);

    switch (splice_command_type) {
        case SPLICE_NULL: {
            root->addElement(u"splice_null");
            break;
        }
        case SPLICE_BANDWIDTH_RESERVATION:  {
            root->addElement(u"bandwidth_reservation");
            break;
        }
        case SPLICE_SCHEDULE: {
            splice_schedule.toXML(duck, root);
            break;
        }
        case SPLICE_INSERT: {
            splice_insert.toXML(duck, root);
            break;
        }
        case SPLICE_TIME_SIGNAL: {
            xml::Element* cmd = root->addElement(u"time_signal");
            if (time_signal.set()) {
                cmd->setIntAttribute(u"pts_time", time_signal.value(), false);
            }
            break;
        }
        case SPLICE_PRIVATE_COMMAND: {
            xml::Element* cmd = root->addElement(u"private_command");
            cmd->setIntAttribute(u"identifier", private_command.identifier, true);
            if (!private_command.private_bytes.empty()) {
                cmd->addHexaText(private_command.private_bytes);
            }
            break;
        }
        default: {
            // Invalid command.
            break;
        }
    }

    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SpliceInformationTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector command;
    bool ok =
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute<uint64_t>(pts_adjustment, u"pts_adjustment", false, 0) &&
        element->getIntAttribute<uint16_t>(tier, u"tier", false, 0x0FFF, 0, 0x0FFF) &&
        descs.fromXML(duck, command, element, u"splice_null,splice_schedule,splice_insert,time_signal,bandwidth_reservation,private_command");

    if (ok && command.size() != 1) {
        element->report().error(u"Specify exactly one splice command in <%s>, line %d", {element->name(), element->lineNumber()});
        return false;
    }

    if (ok) {
        assert(command.size() == 1);
        const xml::Element* const cmd = command[0];
        if (cmd->name() == u"splice_null") {
            splice_command_type = SPLICE_NULL;
        }
        else if (cmd->name() == u"splice_schedule") {
            splice_command_type = SPLICE_SCHEDULE;
            splice_schedule.fromXML(duck, cmd);
            ok = splice_schedule.isValid();
        }
        else if (cmd->name() == u"splice_insert") {
            splice_command_type = SPLICE_INSERT;
            splice_insert.fromXML(duck, cmd);
            ok = splice_insert.isValid();
        }
        else if (cmd->name() == u"time_signal") {
            splice_command_type = SPLICE_TIME_SIGNAL;
            ok = cmd->getOptionalIntAttribute<uint64_t>(time_signal, u"pts_time", 0, PTS_DTS_MASK);
        }
        else if (cmd->name() == u"bandwidth_reservation") {
            splice_command_type = SPLICE_BANDWIDTH_RESERVATION;
        }
        else if (cmd->name() == u"private_command") {
            splice_command_type = SPLICE_PRIVATE_COMMAND;
            ok = cmd->getIntAttribute<uint32_t>(private_command.identifier, u"identifier", true) &&
                 cmd->getHexaText(private_command.private_bytes);
        }
        else {
            // should not get there.
            return false;
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// A static method to display a SpliceInformationTable section.
//----------------------------------------------------------------------------

void ts::SpliceInformationTable::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    // Payload layout: fixed part (11 bytes), variable part, CRC2 (4 bytes).
    // There is a CRC32 at the end of a SpliceInformationTable, even though we are in a short section.

    if (size < 15) {
        display.displayExtraData(data, size, indent);
        return;
    }

    // Check CRC32 now, display it later.
    const CRC32 comp_crc32(section.content(), section.size() - 4);
    const uint32_t sect_crc32 = GetUInt32(data + size - 4);
    size -= 4;

    // Fixed part
    const uint8_t protocol_version = data[0];
    const uint8_t encrypted_packet = (data[1] >> 7) & 0x01;
    const uint8_t encryption_algo = (data[1] >> 1) & 0x3F;
    const uint64_t pts_adjustment = (uint64_t(data[1] & 0x01) << 32) | uint64_t(GetUInt32(data + 2));
    const uint8_t cw_index = data[6];
    const uint16_t tier = (GetUInt16(data + 7) >> 4) & 0x0FFF;
    size_t cmd_length = GetUInt16(data + 8) & 0x0FFF;
    const uint8_t cmd_type = data[10];
    data += 11; size -= 11;

    strm << margin << UString::Format(u"Protocol version: 0x%X (%d)", {protocol_version, protocol_version}) << std::endl
         << margin << "Encryption: ";
    if (encrypted_packet == 0) {
        strm << "none";
    }
    else {
        strm << UString::Format(u"0x%X (%d)", {encryption_algo, encryption_algo});
        switch (encryption_algo) {
            case 0: strm << ", none"; break;
            case 1: strm << ", DES-ECB"; break;
            case 2: strm << ", DES-CBC"; break;
            case 3: strm << ", TDES-ECB"; break;
            default: break;
        }
    }
    strm << std::endl
         << margin << UString::Format(u"PTS adjustment: 0x%09X (%d)", {pts_adjustment, pts_adjustment}) << std::endl
         << margin << UString::Format(u"CW index: 0x%X (%d), tier: 0x%03X (%d)", {cw_index, cw_index, tier, tier}) << std::endl;

    if (encrypted_packet) {
        // The encrypted part starts at the command type.
        strm << margin << "Encrypted command, cannot display" << std::endl;
    }
    else {
        // Unencrypted packet, can display everything.
        strm << margin << UString::Format(u"Command type: %s, size: %d bytes", {NameFromSection(u"SpliceCommandType", cmd_type, names::HEXA_FIRST), cmd_length}) << std::endl;

        // Display the command body. Format some commands, simply dump others.
        if (cmd_length > size) {
            cmd_length = size;
        }
        switch (cmd_type) {
            case SPLICE_SCHEDULE: {
                SpliceSchedule cmd;
                const int done = cmd.deserialize(data, cmd_length);
                if (done >= 0) {
                    assert(size_t(done) <= cmd_length);
                    cmd.display(display, indent);
                    data += done; size -= done; cmd_length -= done;
                }
                break;
            }
            case SPLICE_INSERT: {
                SpliceInsert cmd;
                const int done = cmd.deserialize(data, cmd_length);
                if (done >= 0) {
                    assert(size_t(done) <= cmd_length);
                    cmd.display(display, indent);
                    data += done; size -= done; cmd_length -= done;
                }
                break;
            }
            case SPLICE_TIME_SIGNAL: {
                SpliceTime cmd;
                const int done = cmd.deserialize(data, cmd_length);
                if (done >= 0) {
                    strm << margin << "Time: " << cmd.toString() << std::endl;
                    data += done; size -= done; cmd_length -= done;
                }
                break;
            }
            case SPLICE_PRIVATE_COMMAND: {
                if (cmd_length >= 4) {
                    const uint32_t cmd = GetUInt32(data);
                    strm << margin << UString::Format(u"Command identifier: 0x%0X (%'d)", {cmd, cmd}) << std::endl;
                    data += 4; size -= 4; cmd_length -= 4;
                }
                break;
            }
            default:
                // Invalid command.
                break;
        }
        if (cmd_length > 0) {
            // Unexpected command or unexpected command size.
            strm << margin << "Remaining command content:" << std::endl
                 << UString::Dump(data, cmd_length, UString::HEXA | UString::ASCII | UString::OFFSET, indent + 2);
        }
        data += cmd_length; size -= cmd_length;

        // Splice descriptors.
        if (size >= 2) {
            size_t dl_length = GetUInt16(data);
            data += 2; size -= 2;
            if (dl_length > size) {
                dl_length = size;
            }
            display.displayDescriptorList(section, data, dl_length, indent);
            data += dl_length; size -= dl_length;
        }
    }

    // Final CRC32
    strm << margin << UString::Format(u"CRC32: 0x%X ", {sect_crc32});
    if (sect_crc32 == comp_crc32) {
        strm << "(OK)";
    }
    else {
        strm << UString::Format(u"(WRONG, expected 0x%X)", {comp_crc32.value()});
    }
    strm << std::endl;
}


//----------------------------------------------------------------------------
// A static method to extract a SpliceInsert command from a section.
//----------------------------------------------------------------------------

bool ts::SpliceInformationTable::ExtractSpliceInsert(SpliceInsert& command, const Section& section)
{
    // Payload layout: fixed part (11 bytes), variable part, CRC2 (4 bytes).
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (!section.isValid() || section.tableId() != MY_TID || size < 15) {
        // Not a valid section.
        return false;
    }

    // Check CRC32.
    if (CRC32(section.content(), section.size() - 4) != GetUInt32(data + size - 4)) {
        // Invalid CRC in section.
        return false;
    }
    size -= 4;

    // Fixed part
    if ((data[1] & 0x80) != 0) {
        // Encrypted command, cannot get it.
        return false;
    }

    // PTS adjustment for all time fields.
    const uint64_t pts_adjustment = (uint64_t(data[1] & 0x01) << 32) | uint64_t(GetUInt32(data + 2));

    // Locate splice command.
    size_t cmd_length = GetUInt16(data + 8) & 0x0FFF;
    const uint8_t cmd_type = data[10];
    data += 11; size -= 11;

    if (cmd_length > size || cmd_type != SPLICE_INSERT || command.deserialize(data, cmd_length) < 0) {
        // Invalid length or not a valid SpliceInsert
        return false;
    }

    // SpliceInsert command successfully found.
    command.adjustPTS(pts_adjustment);
    return true;
}
