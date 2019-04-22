//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//
//  Transport stream processor shared library:
//  Build specifically crafted input packets
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsPCR.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class CraftInput: public InputPlugin
    {
    public:
        // Implementation of plugin API
        CraftInput(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual size_t receive(TSPacket*, size_t) override;
        virtual bool abortInput() override { return true; }

    private:
        uint8_t       _initCC;            // continuity_counter
        bool          _constantCC;        // Do not increment continuity counter
        PacketCounter _maxCount;          // Number of packets to generate
        PacketCounter _limit;             // Current max number of packets
        TSPacket      _packet;            // Template of packet to generate

        // Inaccessible operations
        CraftInput() = delete;
        CraftInput(const CraftInput&) = delete;
        CraftInput& operator=(const CraftInput&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(craft, ts::CraftInput)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::CraftInput::CraftInput(TSP* tsp_) :
    InputPlugin(tsp_, u"Build specifically crafted input packets", u"[options]"),
    _initCC(0),
    _constantCC(false),
    _maxCount(0),
    _limit(0),
    _packet(NullPacket)
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
    help(u"no-payload", u"Do not use a payload, equivalent to --payload-size 0.");

    option(u"payload-pattern", 0, STRING);
    help(u"payload-pattern",
         u"Specify the binary pattern to apply on packets payload. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"The pattern is repeated to fill the payload. The default is FF.");

    option(u"payload-size", 0, INTEGER, 0, 1, 0, 184);
    help(u"payload-size",
         u"Specify the size of the packet payload in bytes. "
         u"When necessary, an adaptation field is created. "
         u"By default, the payload uses all free space in the packet.");

    option(u"pcr", 0, UNSIGNED);
    help(u"pcr", u"Set this PCR value in the packets. An adaptation field is created.");

    option(u"opcr", 0, UNSIGNED);
    help(u"opcr", u"Set this OPCR value in the packets. An adaptation field is created.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid", u"Specify the PID for the packets (0 by default).");

    option(u"priority");
    help(u"priority", u"Set the transport_priority flag in the packets.");

    option(u"private-data", 0, STRING);
    help(u"private-data",
         u"Specify the binary content of the transport_private_data in the adaptation field. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes.");

    option(u"pusi");
    help(u"pusi", u"Set the payload_unit_start_indicator in the packets.");

    option(u"random-access");
    help(u"random-access", u"Set the random_access_indicator in the packets. An adaptation field is created.");

    option(u"scrambling", 0, INTEGER, 0, 1, 0, 3);
    help(u"scrambling", u"Specify the value of the transport_scrambling_control field (0 by default).");

    option(u"splice-countdown", 0, UINT8);
    help(u"splice-countdown", u"Create a splicing point and set this splice countdown value in the packets. An adaptation field is created.");
}


//----------------------------------------------------------------------------
// Command line options method
//----------------------------------------------------------------------------

bool ts::CraftInput::getOptions()
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
    const bool fullPayload = !present(u"no-payload") && !present(u"payload-size");
    size_t payloadSize = intValue<size_t>(u"payload-size");

    // Check consistency of options.
    if (payloadSize > 0 && present(u"no-payload")) {
        tsp->error(u"options --no-payload and --payload-size are mutually exclusive");
        return false;
    }

    // The binary patterns.
    ByteBlock _payloadPattern;
    if (!value(u"payload-pattern", u"FF").hexaDecode(_payloadPattern) || _payloadPattern.size() == 0) {
        tsp->error(u"invalid hexadecimal payload pattern");
        return false;
    }

    ByteBlock _privateData;
    if (!value(u"private-data").hexaDecode(_privateData)) {
        tsp->error(u"invalid hexadecimal private data");
        return false;
    }

    // Check if we need to set some data in adaptation field.
    const bool needAF =
        discontinuity ||
        randomAccess ||
        esPriority ||
        pcr != INVALID_PCR ||
        opcr != INVALID_PCR ||
        hasSplicing ||
        !_privateData.empty();

    // Compute adaptation field size.
    // If an AF is needed, it needs at least 2 bytes: length and flags.
    size_t _afSize = needAF ? 2 : 0;
    if (pcr != INVALID_PCR) {
        _afSize += 6;
    }
    if (opcr != INVALID_PCR) {
        _afSize += 6;
    }
    if (hasSplicing) {
        _afSize += 1;
    }
    if (!_privateData.empty()) {
        _afSize += 1 + _privateData.size();
    }

    // Check if adaptation field and payload fit in the packet.
    if (_afSize > 184) {
        tsp->error(u"private data too large, cannot fit in a TS packet");
        return false;
    }
    if (fullPayload) {
        // Payload size unspecified, use the rest of the packet as payload.
        payloadSize = 184 - _afSize;
    }
    else if (_afSize + payloadSize > 184) {
        tsp->error(u"payload and adaptation field too large, cannot fit in a TS packet");
        return false;
    }
    else {
        // Payload size was specified and is smaller than the rest of the packet.
        // Enlarge the adaptation field with stuffing.
        _afSize = 184 - payloadSize;
    }
    assert(_afSize + payloadSize == 184);

    // Build packet header.
    _packet.b[0] = 0x47;
    _packet.b[1] =
        (transportError ? 0x80 : 0x00) |
        (pusi ? 0x40 : 0x00) |
        (transportPriority ? 0x20 : 0x00) |
        (uint8_t(pid >> 8) & 0x1F);
    _packet.b[2] = uint8_t(pid);
    _packet.b[3] =
        ((scrambling & 0x03) << 6) |
        (_afSize > 0 ? 0x20 : 0x00) |
        (payloadSize > 0 ? 0x10 : 0x00) |
        (_initCC & 0x0F);

    // Build adaptation field.
    if (_afSize > 0) {
        uint8_t* data = _packet.b + 4;
        *data++ = uint8_t(_afSize - 1); // length field.
        if (_afSize > 1) {
            // Flags byte.
            *data++ =
                (discontinuity ? 0x80 : 0x00) |
                (randomAccess ? 0x40 : 0x00) |
                (esPriority ? 0x20 : 0x00) |
                (pcr != INVALID_PCR ? 0x10 : 0x00) |
                (opcr != INVALID_PCR ? 0x08 : 0x00) |
                (hasSplicing ? 0x04 : 0x00) |
                (_privateData.empty() ? 0x00 : 0x02);
            // Optional fields in the adaptation field
            if (pcr != INVALID_PCR) {
                PutPCR(data, pcr);
                data += 6;
            }
            if (opcr != INVALID_PCR) {
                PutPCR(data, opcr);
                data += 6;
            }
            if (hasSplicing) {
                *data++ = spliceCountdown;
            }
            if (!_privateData.empty()) {
                *data++ = uint8_t(_privateData.size());
                ::memcpy(data, _privateData.data(), _privateData.size());
                data += _privateData.size();
            }
            // Potential stuffing if a small payload size was specified.
            ::memset(data, 0xFF, _packet.b + 4 + _afSize - data);
        }
    }

    // Build payload.
    if (payloadSize > 0) {
        assert(!_payloadPattern.empty());
        uint8_t* data = _packet.b + 4 + _afSize;
        while (data < _packet.b + PKT_SIZE) {
            const size_t size = std::min<size_t>(_payloadPattern.size(), _packet.b + PKT_SIZE - data);
            ::memcpy(data, _payloadPattern.data(), size);
            data += size;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::CraftInput::start()
{
    _packet.setCC(_initCC);
    _limit = _maxCount;
    return true;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::CraftInput::receive (TSPacket* buffer, size_t maxPackets)
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
        // Increment the continuity counter for the next packet when necessary.
        if (!_constantCC && _packet.hasPayload()) {
            _packet.setCC((_packet.getCC() + 1) & CC_MASK);
        }
    }
    return n;
}
