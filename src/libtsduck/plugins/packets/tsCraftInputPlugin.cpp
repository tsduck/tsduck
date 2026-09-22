//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCraftInputPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_INPUT_PLUGIN(u"craft", ts::CraftInputPlugin);


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::CraftInputPlugin::CraftInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Build specifically crafted input packets", u"[options]")
{
    option(u"constant-cc");
    help(u"constant-cc",
         u"Do not increment the continuity counter. "
         u"By default, the continuity counter in incremented when the packet has a payload.");

    option(u"cc", 0, INTEGER, 0, 1, 0, 15);
    help(u"cc", u"Specify the initial value of the continuity_counter field (0 by default).");

    option(u"count", 'c', UNSIGNED);
    help(u"count",
         u"Specify the number of crafted packets to generate. After the last packet, "
         u"an end-of-file condition is generated. By default, if --count is not "
         u"specified, crafted packets are generated endlessly.");

    option(u"discontinuity");
    help(u"discontinuity", u"Set the discontinuity_indicator in the packets. An adaptation field is created.");

    option(u"error");
    help(u"error", u"Set the transport_error_indicator in the packets.");

    option(u"es-priority");
    help(u"es-priority", u"Set the elementary_stream_priority_indicator in the packets. An adaptation field is created.");

    option(u"joint-termination", 'j');
    help(u"joint-termination",
         u"When the number of crafted packets is specified, perform a \"joint "
         u"termination\" when completed instead of unconditional termination. "
         u"See \"tsp --help\" for more details on \"joint termination\".");

    option(u"no-payload");
    help(u"no-payload", u"Do not use a payload.");

    option(u"payload-pattern", 0, HEXADATA, 0, UNLIMITED_COUNT, 1, PKT_MAX_PAYLOAD_SIZE);
    help(u"payload-pattern",
         u"Specify the binary pattern to apply on packets payload. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"The pattern is repeated to fill the payload. The default is FF.");

    option(u"payload-size", 0, INTEGER, 0, 1, 0, PKT_MAX_PAYLOAD_SIZE);
    help(u"payload-size", u"size",
         u"Specify the size of the packet payload in bytes. "
         u"When necessary, an adaptation field is created. "
         u"Note that --payload-size 0 specifies that a payload exists with a zero size. "
         u"This is different from --no-payload which also specifies that the payload does not exist. "
         u"By default, the payload uses all free space in the packet.");

    option(u"pcr", 0, UNSIGNED);
    help(u"pcr", u"Set this PCR value in the packets. An adaptation field is created.");

    option(u"opcr", 0, UNSIGNED);
    help(u"opcr", u"Set this OPCR value in the packets. An adaptation field is created.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid", u"Specify the PID for the packets (0 by default).");

    option(u"priority");
    help(u"priority", u"Set the transport_priority flag in the packets.");

    option(u"private-data", 0, HEXADATA);
    help(u"private-data",
         u"Specify the binary content of the transport_private_data in the adaptation field. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes.");

    option(u"pusi");
    help(u"pusi", u"Set the payload_unit_start_indicator in the packets.");

    option(u"random-access");
    help(u"random-access", u"Set the random_access_indicator in the packets. An adaptation field is created.");

    option(u"rs204", 0, HEXADATA);
    help(u"rs204",
         u"Generate a 204-byte packet and specify the binary content to store in the 16-byte trailer. "
         u"The value must be a string of hexadecimal digits specifying up to 16 bytes. "
         u"If the data are shorter than 16 bytes, they are padded with 0xFF.");

    option(u"scrambling", 0, INTEGER, 0, 1, 0, 3);
    help(u"scrambling", u"Specify the value of the transport_scrambling_control field (0 by default).");

    option(u"splice-countdown", 0, UINT8);
    help(u"splice-countdown", u"Create a splicing point and set this splice countdown value in the packets. An adaptation field is created.");
}


//----------------------------------------------------------------------------
// Input command line options method
//----------------------------------------------------------------------------

bool ts::CraftInputPlugin::getOptions()
{
    // Processing options.
    _initCC = intValue<uint8_t>(u"cc");
    _constantCC = present(u"constant-cc");
    _maxCount = intValue<PacketCounter>(u"count", std::numeric_limits<PacketCounter>::max());
    tsp->useJointTermination(present(u"joint-termination"));

    // Options for packet content.
    const PID pid = intValue<PID>(u"pid");
    const bool pusi = present(u"pusi");
    const bool transportError = present(u"error");
    const bool transportPriority = present(u"priority");
    const uint8_t scrambling = intValue<uint8_t>(u"scrambling");
    const bool discontinuity = present(u"discontinuity");
    const bool randomAccess = present(u"random-access");
    const bool esPriority = present(u"es-priority");
    const uint64_t pcr = intValue<uint64_t>(u"pcr", INVALID_PCR);
    const uint64_t opcr = intValue<uint64_t>(u"opcr", INVALID_PCR);
    const uint8_t spliceCountdown = intValue<uint8_t>(u"splice-countdown");
    const bool hasSplicing = present(u"splice-countdown");
    const bool noPayload = present(u"no-payload");
    const bool fullPayload = !noPayload && !present(u"payload-size"); // payload uses all available size
    size_t payloadSize = intValue<size_t>(u"payload-size");

    // Check consistency of options.
    if (payloadSize > 0 && noPayload) {
        error(u"options --no-payload and --payload-size are mutually exclusive");
        return false;
    }

    // The binary patterns.
    const ByteBlock payloadPattern(hexaValue(u"payload-pattern", ByteBlock(1, 0xFF)));
    const ByteBlock privateData(hexaValue(u"private-data"));
    ByteBlock rs204(hexaValue(u"rs204"));

    // Check if we need to set some data in adaptation field.
    const bool needAF =
        discontinuity ||
        randomAccess ||
        esPriority ||
        pcr != INVALID_PCR ||
        opcr != INVALID_PCR ||
        hasSplicing ||
        !privateData.empty();

    // Compute adaptation field size.
    // If an AF is needed, it needs at least 2 bytes: length and flags.
    size_t afSize = needAF ? 2 : 0;
    if (pcr != INVALID_PCR) {
        afSize += 6;
    }
    if (opcr != INVALID_PCR) {
        afSize += 6;
    }
    if (hasSplicing) {
        afSize += 1;
    }
    if (!privateData.empty()) {
        afSize += 1 + privateData.size();
    }

    // Check if adaptation field and payload fit in the packet.
    if (afSize > 184) {
        error(u"private data too large, cannot fit in a TS packet");
        return false;
    }
    if (fullPayload) {
        // Payload size unspecified, use the rest of the packet as payload.
        payloadSize = 184 - afSize;
    }
    else if (afSize + payloadSize > 184) {
        error(u"payload and adaptation field too large, cannot fit in a TS packet");
        return false;
    }
    else {
        // Payload size was specified and is smaller than the rest of the packet.
        // Enlarge the adaptation field with stuffing.
        afSize = 184 - payloadSize;
    }
    assert(afSize + payloadSize == 184);

    // Build packet header.
    _packet = NullPacket;
    _packet.b[0] = 0x47;
    _packet.b[1] =
        (transportError ? 0x80 : 0x00) |
        (pusi ? 0x40 : 0x00) |
        (transportPriority ? 0x20 : 0x00) |
        (uint8_t(pid >> 8) & 0x1F);
    _packet.b[2] = uint8_t(pid);
    _packet.b[3] =
        uint8_t((scrambling & 0x03) << 6) |
        (afSize > 0 ? 0x20 : 0x00) |
        (payloadSize > 0 || !noPayload ? 0x10 : 0x00) |
        (_initCC & 0x0F);

    // Build adaptation field.
    if (afSize > 0) {
        uint8_t* data = _packet.b + 4;
        *data++ = uint8_t(afSize - 1); // length field.
        if (afSize > 1) {
            // Flags byte.
            *data++ =
                (discontinuity ? 0x80 : 0x00) |
                (randomAccess ? 0x40 : 0x00) |
                (esPriority ? 0x20 : 0x00) |
                (pcr != INVALID_PCR ? 0x10 : 0x00) |
                (opcr != INVALID_PCR ? 0x08 : 0x00) |
                (hasSplicing ? 0x04 : 0x00) |
                (privateData.empty() ? 0x00 : 0x02);
            // Optional fields in the adaptation field
            if (pcr != INVALID_PCR) {
                TSPacket::PutPCR(data, pcr);
                data += TSPacket::PCR_BYTES;
            }
            if (opcr != INVALID_PCR) {
                TSPacket::PutPCR(data, opcr);
                data += TSPacket::PCR_BYTES;
            }
            if (hasSplicing) {
                *data++ = spliceCountdown;
            }
            if (!privateData.empty()) {
                *data++ = uint8_t(privateData.size());
                MemCopy(data, privateData.data(), privateData.size());
                data += privateData.size();
            }
            // Potential stuffing if a small payload size was specified.
            MemSet(data, 0xFF, _packet.b + 4 + afSize - data);
        }
    }

    // Build payload.
    if (payloadSize > 0) {
        assert(!payloadPattern.empty());
        uint8_t* data = _packet.b + 4 + afSize;
        while (data < _packet.b + PKT_SIZE) {
            const size_t size = std::min<size_t>(payloadPattern.size(), _packet.b + PKT_SIZE - data);
            MemCopy(data, payloadPattern.data(), size);
            data += size;
        }
    }

    // Build metadata.
    _mdata.reset();
    if (!rs204.empty()) {
        rs204.resize(RS_SIZE, 0xFF);
        _mdata.setAuxData(rs204.data(), rs204.size());
    }
    return true;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::CraftInputPlugin::start()
{
    _packet.setCC(_initCC);
    _limit = _maxCount;
    return true;
}


//----------------------------------------------------------------------------
// Input is never blocking.
//----------------------------------------------------------------------------

bool ts::CraftInputPlugin::setReceiveTimeout(cn::milliseconds timeout)
{
    return true;
}

bool ts::CraftInputPlugin::abortInput()
{
    return true;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::CraftInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t maxPackets)
{
    // Previous number of generated packets.
    const PacketCounter previousCount = tsp->pluginPackets();

    // If "joint termination" reached for this plugin
    if (previousCount >= _limit && tsp->useJointTermination()) {
        // Declare terminated
        tsp->jointTerminate();
        // Continue generating packets until completion of tsp (suppress max packet count)
        _limit = std::numeric_limits<PacketCounter>::max();
    }

    // Fill buffer
    size_t n;
    for (n = 0; n < maxPackets && previousCount + n < _limit; n++) {
        buffer[n] = _packet;
        pkt_data[n] = _mdata;
        // Increment the continuity counter for the next packet when necessary.
        if (!_constantCC && _packet.hasPayload()) {
            _packet.setCC((_packet.getCC() + 1) & CC_MASK);
        }
    }
    return n;
}
