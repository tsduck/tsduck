//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsSpliceInfoTable.h"
#include "tsBinaryTable.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"splice_info_table"
#define MY_TID ts::TID_SCTE35_SIT

TS_ID_SECTION_DISPLAY(ts::SpliceInfoTable::DisplaySection, MY_TID);


//----------------------------------------------------------------------------
// A static method to display a SpliceInfoTable section.
//----------------------------------------------------------------------------

void ts::SpliceInfoTable::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    // Payload layout: fixed part (11 bytes), variable part, CRC2 (4 bytes).
    // There is a CRC32 at the end of a SpliceInfoTable, even though we are in a short section.

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
        strm << margin << UString::Format(u"Command type: %s, size: %d bytes", {DVBNameFromSection(u"SpliceCommandType", cmd_type, names::HEXA_FIRST), cmd_length}) << std::endl;

        // Display the command body. Format some commands, simply dump others.
        if (cmd_length > size) {
            cmd_length = size;
        }
        if (cmd_type == SPLICE_INSERT) {
            SpliceInsert cmd;
            const int done = cmd.deserialize(data, cmd_length);
            if (done >= 0) {
                cmd.display(display, indent);
                data += done;
                cmd_length -= done;
                size -= done;
            }
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
            while (dl_length >= 6) {
                const uint8_t tag = data[0];
                const size_t len = std::max<size_t>(4, std::min<size_t>(dl_length - 2, data[1]));
                const uint32_t id = GetUInt32(data + 2);
                strm << margin
                     << UString::Format(u"Splice descriptor tag: %s, size: %d bytes, id: 0x%X, content: %s",
                                        {DVBNameFromSection(u"SpliceDescriptorTag", tag, names::HEXA_FIRST), len, id, UString::Dump(data + 6, len - 4, UString::SINGLE_LINE)})
                     << std::endl;
                data += 2 + len;
                size -= 2 + len;
                dl_length -= 2 + len;
            }
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

bool ts::SpliceInfoTable::ExtractSpliceInsert(SpliceInsert& command, const Section& section)
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
