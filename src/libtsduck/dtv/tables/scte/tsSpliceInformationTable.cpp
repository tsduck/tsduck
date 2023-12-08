//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSpliceInformationTable.h"
#include "tsBinaryTable.h"
#include "tsxmlElement.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"

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

size_t ts::SpliceInformationTable::maxPayloadSize() const
{
    // Although declared as a "non-private section" in the MPEG sense, the
    // SpliceInformationTable section can use up to 4096 bytes in SCTE 35.
    return MAX_PRIVATE_SHORT_SECTION_PAYLOAD_SIZE;
}

bool ts::SpliceInformationTable::useTrailingCRC32() const
{
    // A splice_information_table is a short section with a CRC32.
    return true;
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
    time_signal.reset();
    private_command.identifier = 0;
    private_command.private_bytes.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Adjust PTS time values using the "PTS adjustment".
//----------------------------------------------------------------------------

void ts::SpliceInformationTable::adjustPTS()
{
    // Ignore null or invalid adjustment.
    if (pts_adjustment == 0 || pts_adjustment > PTS_DTS_MASK) {
        return;
    }

    // Only splice_insert() and time_signal() commands need adjustment.
    if (splice_command_type == SPLICE_INSERT) {
        splice_insert.adjustPTS(pts_adjustment);
    }
    else if (splice_command_type == SPLICE_TIME_SIGNAL) {
        // Adjust time signal time.
        if (time_signal.has_value() && time_signal.value() <= PTS_DTS_MASK) {
            time_signal = (time_signal.value() + pts_adjustment) & PTS_DTS_MASK;
        }
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

void ts::SpliceInformationTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // A splice_information_table section is a short section with a CRC32.
    // But it has already been checked and removed from the buffer since useTrailingCRC32() returns true.

    // Fixed part.
    protocol_version = buf.getUInt8();
    const bool encrypted = buf.getBool();
    buf.skipBits(6); // skip encryption_algorithm
    buf.getBits(pts_adjustment, 33);
    buf.skipBits(8); // skip cw_index
    buf.getBits(tier, 12);

    // Splice command length and type. Note that the command length can be the legacy value 0x0FFF, meaning unspecified.
    size_t command_length = buf.getBits<size_t>(12);
    splice_command_type = buf.getUInt8();

    // Encrypted sections cannot be deserialized.
    if (encrypted) {
        return;
    }

    // Decode splice command. Remember that the command length can be unspecified (0x0FFF).
    const size_t max_length = command_length == 0x0FFF ? buf.remainingReadBytes() : command_length;
    int actual_length = -1;
    switch (splice_command_type) {
        case SPLICE_NULL:
        case SPLICE_BANDWIDTH_RESERVATION:
            // These commands are empty.
            actual_length = 0;
            break;
        case SPLICE_SCHEDULE:
            actual_length = splice_schedule.deserialize(buf.currentReadAddress(), max_length);
            break;
        case SPLICE_INSERT:
            actual_length = splice_insert.deserialize(buf.currentReadAddress(), max_length);
            break;
        case SPLICE_TIME_SIGNAL:
            actual_length = time_signal.deserialize(buf.currentReadAddress(), max_length);
            break;
        case SPLICE_PRIVATE_COMMAND:
            // A splice private command has no implicit size. It cannot be used with legacy command_length == 0x0FFF.
            if (command_length != 0x0FFF && command_length >= 4) {
                private_command.identifier = buf.getUInt32();
                buf.getBytes(private_command.private_bytes, command_length - 4);
                actual_length = 0; // already skipped
                command_length = 0;
            }
            break;
        default:
            // Invalid command.
            break;
    }

    // Handle error in the splice command.
    if (actual_length < 0) {
        buf.setUserError();
        if (command_length == 0x0FFF) {
            // Unknown command length, cannot recover.
            return;
        }
    }

    // Point after the splice command.
    buf.skipBytes(command_length == 0x0FFF ? size_t(actual_length) : command_length);

    // Process descriptor list.
    buf.getDescriptorListWithLength(descs, 16);

    // Skip alignment_stuffing
    buf.skipBytes(buf.remainingReadBytes());
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SpliceInformationTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    buf.putUInt8(protocol_version);
    buf.putBit(0);      // encrypted_packet
    buf.putBits(0, 6);  // encryption_algorithm
    buf.putBits(pts_adjustment, 33);
    buf.putUInt8(0);    // cw_index
    buf.putBits(tier, 12);
    buf.pushState();    // before splice_command_length
    buf.putBits(0, 12); // placeholder for splice_command_length
    buf.putUInt8(splice_command_type);

    // Serialize the splice command.
    const size_t start = buf.currentWriteByteOffset();
    ByteBlock bb;
    switch (splice_command_type) {
        case SPLICE_NULL:
        case SPLICE_BANDWIDTH_RESERVATION:
            // These commands are empty.
            break;
        case SPLICE_SCHEDULE:
            splice_schedule.serialize(bb);
            break;
        case SPLICE_INSERT:
            splice_insert.serialize(bb);
            break;
        case SPLICE_TIME_SIGNAL:
            time_signal.serialize(bb);
            break;
        case SPLICE_PRIVATE_COMMAND:
            buf.putUInt32(private_command.identifier);
            buf.putBytes(private_command.private_bytes);
            break;
        default:
            // Invalid command.
            break;
    }
    buf.putBytes(bb);

    // Adjust the command length.
    const size_t splice_command_length = buf.currentWriteByteOffset() - start;
    buf.swapState();
    buf.putBits(splice_command_length, 12);
    buf.popState();

    // Descriptor loop.
    buf.putDescriptorListWithLength(descs, 0, NPOS, 16);

    // A splice_information_table section is a short section with a CRC32.
    // But it will be automatically added since useTrailingCRC32() returns true.
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
            if (time_signal.has_value()) {
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
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute(pts_adjustment, u"pts_adjustment", false, 0) &&
        element->getIntAttribute(tier, u"tier", false, 0x0FFF, 0, 0x0FFF) &&
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
            ok = cmd->getOptionalIntAttribute(time_signal, u"pts_time", 0, PTS_DTS_MASK);
        }
        else if (cmd->name() == u"bandwidth_reservation") {
            splice_command_type = SPLICE_BANDWIDTH_RESERVATION;
        }
        else if (cmd->name() == u"private_command") {
            splice_command_type = SPLICE_PRIVATE_COMMAND;
            ok = cmd->getIntAttribute(private_command.identifier, u"identifier", true) &&
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

void ts::SpliceInformationTable::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (buf.canReadBytes(15)) {
        disp << margin << UString::Format(u"Protocol version: %d", {buf.getUInt8()}) << std::endl;
        disp << margin << "Encryption: ";
        const uint8_t encrypted_packet = buf.getBit();
        if (encrypted_packet == 0) {
            disp << "none";
            buf.skipBits(6); // skip encryption_algorithm
        }
        else {
            const uint8_t encryption_algo = buf.getBits<uint8_t>(6);
            disp << UString::Format(u"0x%X (%<d)", {encryption_algo});
            switch (encryption_algo) {
                case 0: disp << ", none"; break;
                case 1: disp << ", DES-ECB"; break;
                case 2: disp << ", DES-CBC"; break;
                case 3: disp << ", TDES-ECB"; break;
                default: break;
            }
        }
        disp << std::endl;
        disp << margin << "PTS adjustment: " << PTSToString(buf.getBits<uint64_t>(33)) << std::endl;
        disp << margin << UString::Format(u"CW index: 0x%X (%<d)", {buf.getUInt8()});
        disp << UString::Format(u", tier: 0x%03X (%<d)", {buf.getBits<uint16_t>(12)}) << std::endl;
        if (encrypted_packet != 0) {
            // The encrypted part starts at the command type.
            disp << margin << "Encrypted command, cannot display" << std::endl;
        }
        else {
            // Unencrypted packet, can display everything.
            const size_t cmd_length = buf.getBits<size_t>(12);
            const uint8_t cmd_type = buf.getUInt8();
            disp << margin
                 << "Command type: " << NameFromDTV(u"SpliceCommandType", cmd_type, NamesFlags::HEXA_FIRST)
                 << ", size: " << (cmd_length == 0x0FFF ? u"unspecified" : UString::Format(u"%d bytes", {cmd_length}))
                 << std::endl;

            // If the command length is the legacy value 0x0FFF, it means unspecified. See deserializePayload().
            const size_t max_length = cmd_length == 0x0FFF ? buf.remainingReadBytes() : cmd_length;
            int actual_length = -1;

            switch (cmd_type) {
                case SPLICE_NULL:
                case SPLICE_BANDWIDTH_RESERVATION: {
                    // These commands are empty.
                    actual_length = 0;
                    break;
                }
                case SPLICE_SCHEDULE: {
                    SpliceSchedule cmd;
                    actual_length = cmd.deserialize(buf.currentReadAddress(), max_length);
                    if (actual_length >= 0) {
                        cmd.display(disp, margin);
                    }
                    break;
                }
                case SPLICE_INSERT: {
                    SpliceInsert cmd;
                    actual_length = cmd.deserialize(buf.currentReadAddress(), max_length);
                    if (actual_length >= 0) {
                        cmd.display(disp, margin);
                    }
                    break;
                }
                case SPLICE_TIME_SIGNAL: {
                    SpliceTime cmd;
                    actual_length = cmd.deserialize(buf.currentReadAddress(), max_length);
                    if (actual_length >= 0) {
                        disp << margin << "Time: " << cmd.toString() << std::endl;
                    }
                    break;
                }
                case SPLICE_PRIVATE_COMMAND: {
                    // A splice private command has no implicit size. It cannot be used with legacy command_length == 0x0FFF.
                    if (cmd_length != 0x0FFF && cmd_length >= 4) {
                        disp << margin << UString::Format(u"Command identifier: 0x%0X (%<'d)", {GetUInt32(buf.currentReadAddress())}) << std::endl;
                        actual_length = 4;
                    }
                    break;
                }
                default:
                    // Invalid command.
                    break;
            }
            if (cmd_length != 0x0FFF) {
                // Total splice command line is known, we can display the extra.
                if (actual_length > 0) {
                    // Skipped what was already displayed.
                    buf.skipBytes(size_t(actual_length));
                }
                const size_t extra = cmd_length - std::min(cmd_length, actual_length < 0 ? 0 : size_t(actual_length));
                disp.displayPrivateData(u"Remaining command content", buf, extra, margin);
            }
            else if (actual_length < 0) {
                // Unknown command length, cannot recover.
                return;
            }
            else {
                // Need to trust the implicit command length.
                buf.skipBytes(size_t(actual_length));
            }

            // Splice descriptors.
            disp.displayDescriptorListWithLength(section, buf, margin, UString(), UString(), 16);
        }
    }
    disp.displayCRC32(section, buf, margin);
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
