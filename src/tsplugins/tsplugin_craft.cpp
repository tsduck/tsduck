//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsPluginRepository.h"
#include "tsFunctors.h"


//----------------------------------------------------------------------------
// Input plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class CraftInput: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(CraftInput);
    public:
        // Implementation of plugin API
        CraftInput(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput() override;
        virtual bool setReceiveTimeout(MilliSecond timeout) override;

    private:
        // Command line options:
        uint8_t       _initCC;      // continuity_counter
        bool          _constantCC;  // Do not increment continuity counter
        PacketCounter _maxCount;    // Number of packets to generate

        // Working data:
        PacketCounter _limit;       // Current max number of packets
        TSPacket      _packet;      // Template of packet to generate
    };
}


//----------------------------------------------------------------------------
// Packet processing plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class CraftPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(CraftPlugin);
    public:
        // Implementation of plugin API
        CraftPlugin(TSP*);
        virtual bool getOptions() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool      _setDiscontinuity;
        bool      _clearDiscontinuity;
        bool      _setTransportError;
        bool      _clearTransportError;
        bool      _setTransportPriority;
        bool      _clearTransportPriority;
        bool      _setESPriority;
        bool      _clearESPriority;
        bool      _resizePayload;
        bool      _noRepeat;
        size_t    _payloadSize;
        bool      _noPayload;
        bool      _pesPayload;
        ByteBlock _payloadPattern;
        ByteBlock _payloadAnd;
        ByteBlock _payloadOr;
        ByteBlock _payloadXor;
        size_t    _offsetPattern;
        ByteBlock _privateData;
        bool      _clearPrivateData;
        bool      _clearPCR;
        uint64_t  _newPCR;
        bool      _clearOPCR;
        uint64_t  _newOPCR;
        bool      _setPID;
        PID       _newPID;
        bool      _setPUSI;
        bool      _clearPUSI;
        bool      _setRandomAccess;
        bool      _clearRandomAccess;
        bool      _packPESHeader;
        bool      _setScrambling;
        uint8_t   _newScrambling;
        bool      _setCC;
        uint8_t   _newCC;
        bool      _setSpliceCountdown;
        bool      _clearSpliceCountdown;
        uint8_t   _newSpliceCountdown;

        // Perform --pack-pes-header on a packet.
        void packPESHeader(TSPacket&);

        // Perform payload operations such as --payload-pattern, --payload-and, etc.
        template <typename Op>
        void updatePayload(TSPacket& pkt, size_t payloadBase, const ByteBlock& pattern, Op assign);
    };
}


//----------------------------------------------------------------------------
// Plugin shared library interface
//----------------------------------------------------------------------------

TS_REGISTER_INPUT_PLUGIN(u"craft", ts::CraftInput);
TS_REGISTER_PROCESSOR_PLUGIN(u"craft", ts::CraftPlugin);


//----------------------------------------------------------------------------
// Input constructor
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

    option(u"scrambling", 0, INTEGER, 0, 1, 0, 3);
    help(u"scrambling", u"Specify the value of the transport_scrambling_control field (0 by default).");

    option(u"splice-countdown", 0, UINT8);
    help(u"splice-countdown", u"Create a splicing point and set this splice countdown value in the packets. An adaptation field is created.");
}


//----------------------------------------------------------------------------
// Input command line options method
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
    const bool noPayload = present(u"no-payload");
    const bool fullPayload = !noPayload && !present(u"payload-size"); // payload uses all available size
    size_t payloadSize = intValue<size_t>(u"payload-size");

    // Check consistency of options.
    if (payloadSize > 0 && noPayload) {
        tsp->error(u"options --no-payload and --payload-size are mutually exclusive");
        return false;
    }

    // The binary patterns.
    const ByteBlock payloadPattern(hexaValue(u"payload-pattern", ByteBlock(1, 0xFF)));
    const ByteBlock privateData(hexaValue(u"private-data"));

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
        tsp->error(u"private data too large, cannot fit in a TS packet");
        return false;
    }
    if (fullPayload) {
        // Payload size unspecified, use the rest of the packet as payload.
        payloadSize = 184 - afSize;
    }
    else if (afSize + payloadSize > 184) {
        tsp->error(u"payload and adaptation field too large, cannot fit in a TS packet");
        return false;
    }
    else {
        // Payload size was specified and is smaller than the rest of the packet.
        // Enlarge the adaptation field with stuffing.
        afSize = 184 - payloadSize;
    }
    assert(afSize + payloadSize == 184);

    // Build packet header.
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
                ::memcpy(data, privateData.data(), privateData.size());
                data += privateData.size();
            }
            // Potential stuffing if a small payload size was specified.
            ::memset(data, 0xFF, _packet.b + 4 + afSize - data);
        }
    }

    // Build payload.
    if (payloadSize > 0) {
        assert(!payloadPattern.empty());
        uint8_t* data = _packet.b + 4 + afSize;
        while (data < _packet.b + PKT_SIZE) {
            const size_t size = std::min<size_t>(payloadPattern.size(), _packet.b + PKT_SIZE - data);
            ::memcpy(data, payloadPattern.data(), size);
            data += size;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Input start method
//----------------------------------------------------------------------------

bool ts::CraftInput::start()
{
    _packet.setCC(_initCC);
    _limit = _maxCount;
    return true;
}


//----------------------------------------------------------------------------
// Input is never blocking.
//----------------------------------------------------------------------------

bool ts::CraftInput::setReceiveTimeout(MilliSecond timeout)
{
    return true;
}

bool ts::CraftInput::abortInput()
{
    return true;
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

size_t ts::CraftInput::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t maxPackets)
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


//----------------------------------------------------------------------------
// Packet processing plugin constructor
//----------------------------------------------------------------------------

ts::CraftPlugin::CraftPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Craft specific low-level transformations on packets", u"[options]"),
    _setDiscontinuity(false),
    _clearDiscontinuity(false),
    _setTransportError(false),
    _clearTransportError(false),
    _setTransportPriority(false),
    _clearTransportPriority(false),
    _setESPriority(false),
    _clearESPriority(false),
    _resizePayload(false),
    _noRepeat(false),
    _payloadSize(0),
    _noPayload(false),
    _pesPayload(false),
    _payloadPattern(),
    _payloadAnd(),
    _payloadOr(),
    _payloadXor(),
    _offsetPattern(0),
    _privateData(),
    _clearPrivateData(false),
    _clearPCR(false),
    _newPCR(0),
    _clearOPCR(false),
    _newOPCR(0),
    _setPID(false),
    _newPID(PID_NULL),
    _setPUSI(false),
    _clearPUSI(false),
    _setRandomAccess(false),
    _clearRandomAccess(false),
    _packPESHeader(false),
    _setScrambling(false),
    _newScrambling(0),
    _setCC(false),
    _newCC(0),
    _setSpliceCountdown(false),
    _clearSpliceCountdown(false),
    _newSpliceCountdown(0)
{
    setIntro(u"This plugin modifies precise fields in all TS packets. "
             u"Some operations may need space in the adaptation field. "
             u"By default, the payload is left unmodified and a transformation is "
             u"rejected if it needs to enlarge the adaptation field since this "
             u"would destroy part of the existing payload. "
             u"Enlarging the adaptation field is possible only when --payload-pattern "
             u"is specified, in which case the payload is overwritten anyway.");

    option(u"continuity-counter", 0, INTEGER, 0, 1, 0, 15);
    help(u"continuity-counter", u"Specify the value of the continuity_counter field.");

    option(u"discontinuity");
    help(u"discontinuity", u"Set the discontinuity_indicator in the packets. Space is required in the adaptation field.");

    option(u"clear-discontinuity");
    help(u"clear-discontinuity", u"Clear the discontinuity_indicator in the packets.");

    option(u"error");
    help(u"error", u"Set the transport_error_indicator in the packets.");

    option(u"clear-error");
    help(u"clear-error", u"Clear the transport_error_indicator in the packets.");

    option(u"es-priority");
    help(u"es-priority", u"Set the elementary_stream_priority_indicator in the packets. Space is required in the adaptation field.");

    option(u"clear-es-priority");
    help(u"clear-es-priority", u"Clear the elementary_stream_priority_indicator in the packets.");

    option(u"no-payload");
    help(u"no-payload", u"Remove the payload.");

    option(u"no-repeat");
    help(u"no-repeat",
         u"Do not repeat payload pattern operations as specified by options "
         u"--payload-pattern, --payload-and, --payload-or, --payload-xor. "
         u"The operation is performed once only.");

    option(u"payload-pattern", 0, HEXADATA, 0, UNLIMITED_COUNT, 0, PKT_MAX_PAYLOAD_SIZE);
    help(u"payload-pattern",
         u"Overwrite the payload with the specified binary pattern. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"The pattern is repeated to fill the payload (unless --no-repeat is specified).");

    option(u"payload-and", 0, HEXADATA, 0, UNLIMITED_COUNT, 0, PKT_MAX_PAYLOAD_SIZE);
    help(u"payload-and",
         u"Apply a binary \"and\" operation on the payload using the specified binary pattern. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"The \"and\" operation is repeated up to the end of the payload (unless --no-repeat is specified).");

    option(u"payload-or", 0, HEXADATA, 0, UNLIMITED_COUNT, 0, PKT_MAX_PAYLOAD_SIZE);
    help(u"payload-or",
         u"Apply a binary \"or\" operation on the payload using the specified binary pattern. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"The \"or\" operation is repeated up to the end of the payload (unless --no-repeat is specified).");

    option(u"payload-xor", 0, HEXADATA, 0, UNLIMITED_COUNT, 0, PKT_MAX_PAYLOAD_SIZE);
    help(u"payload-xor",
         u"Apply a binary \"exclusive or\" operation on the payload using the specified binary pattern. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"The \"exclusive or\" operation is repeated up to the end of the payload (unless --no-repeat is specified).");

    option(u"payload-size", 0, INTEGER, 0, 1, 0, PKT_SIZE - 4);
    help(u"payload-size", u"size",
         u"Resize the packet payload to the specified value in bytes. "
         u"When necessary, an adaptation field is created or enlarged. "
         u"Without --payload-pattern, the existing payload is either shrunk or enlarged. "
         u"When an existing payload is shrunk, the end of the payload is truncated. "
         u"When an existing payload is enlarged, its end is padded with 0xFF bytes. "
         u"Note that --payload-size 0 specifies that a payload exists with a zero size. "
         u"This is different from --no-payload which also specifies that the payload does not exist.");

    option(u"offset-pattern", 0, INTEGER, 0, 1, 0, PKT_SIZE - 4);
    help(u"offset-pattern",
         u"Specify starting offset in payload when using --payload-pattern. By default, "
         u"the pattern replacement starts at the beginning of the packet payload.");

    option(u"pes-payload");
    help(u"pes-payload",
         u"With this option, the modified payload is the PES payload, not the TS payload. "
         u"When the TS packet does not contain the start of a PES packet, the TS payload is not modified. "
         u"With --payload-size, the TS payload is resized so that the part of the PES payload which is in "
         u"the TS packet gets the specified size. "
         u"With --payload-pattern and --offset-pattern, the pattern is applied inside the PES payload.");

    option(u"pcr", 0, UNSIGNED);
    help(u"pcr", u"Set this PCR value in the packets. Space is required in the adaptation field.");

    option(u"no-pcr");
    help(u"no-pcr", u"Remove the PCR from the packets.");

    option(u"opcr", 0, UNSIGNED);
    help(u"opcr", u"Set this OPCR value in the packets. Space is required in the adaptation field.");

    option(u"no-opcr");
    help(u"no-opcr", u"Remove the OPCR from the packets.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid", u"Modify the PID to the specified value.");

    option(u"priority");
    help(u"priority", u"Set the transport_priority flag in the packets.");

    option(u"clear-priority");
    help(u"clear-priority", u"Clear the transport_priority flag in the packets.");

    option(u"private-data", 0, HEXADATA);
    help(u"private-data",
         u"Specify the binary content of the transport_private_data in the adaptation field. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"Space is required in the adaptation field.");

    option(u"no-private-data");
    help(u"no-private-data", u"Remove the private data from adaptation field.");

    option(u"pusi");
    help(u"pusi", u"Set the payload_unit_start_indicator in the packets.");

    option(u"clear-pusi");
    help(u"clear-pusi", u"Clear the payload_unit_start_indicator in the packets.");

    option(u"random-access");
    help(u"random-access", u"Set the random_access_indicator in the packets. Space is required in the adaptation field.");

    option(u"clear-random-access");
    help(u"clear-random-access", u"Clear the random_access_indicator in the packets.");

    option(u"scrambling", 0, INTEGER, 0, 1, 0, 3);
    help(u"scrambling", u"Specify the value of the transport_scrambling_control field.");

    option(u"splice-countdown", 0, UINT8);
    help(u"splice-countdown", u"Create a splicing point and set this splice countdown value in the packets. Space is required in the adaptation field.");

    option(u"no-splice-countdown");
    help(u"no-splice-countdown", u"Remove the splicing point from the packets.");

    option(u"pack-pes-header");
    help(u"pack-pes-header",
         u"When a TS packet contains the start of a PES packet and the header of this PES packet "
         u"contains stuffing, shift the TS payload to remove all possible stuffing from the PES "
         u"header. Create TS stuffing in the adaptation field to compensate.");
}


//----------------------------------------------------------------------------
// Packet processing plugin get command line options.
//----------------------------------------------------------------------------

bool ts::CraftPlugin::getOptions()
{
    _setDiscontinuity = present(u"discontinuity");
    _clearDiscontinuity = present(u"clear-discontinuity");
    _setTransportError = present(u"error");
    _clearTransportError = present(u"clear-error");
    _setTransportPriority = present(u"priority");
    _clearTransportPriority = present(u"clear-priority");
    _setESPriority = present(u"es-priority");
    _clearESPriority = present(u"clear-es-priority");
    _noPayload = present(u"no-payload");
    _noRepeat = present(u"no-repeat");
    _resizePayload = present(u"payload-size") || _noPayload;
    getIntValue(_payloadSize, u"payload-size", 0);
    _pesPayload = present(u"pes-payload");
    getIntValue(_offsetPattern, u"offset-pattern", 0);
    _clearPCR = present(u"no-pcr");
    getIntValue(_newPCR, u"pcr", INVALID_PCR);
    _clearOPCR = present(u"no-opcr");
    getIntValue(_newOPCR, u"opcr", INVALID_PCR);
    _setPID = present(u"pid");
    getIntValue(_newPID, u"pid");
    _setPUSI = present(u"pusi");
    _clearPUSI = present(u"clear-pusi");
    _setRandomAccess = present(u"random-access");
    _clearRandomAccess = present(u"clear-random-access");
    _packPESHeader = present(u"pack-pes-header");
    _setScrambling = present(u"scrambling");
    getIntValue(_newScrambling, u"scrambling");
    _setCC = present(u"continuity-counter");
    getIntValue(_newCC, u"continuity-counter");
    _setSpliceCountdown = present(u"splice-countdown");
    _clearSpliceCountdown = present(u"no-splice-countdown");
    getIntValue(_newSpliceCountdown, u"splice-countdown");
    _clearPrivateData = present(u"no-private-data");
    getHexaValue(_payloadPattern, u"payload-pattern");
    getHexaValue(_payloadAnd, u"payload-and");
    getHexaValue(_payloadOr, u"payload-or");
    getHexaValue(_payloadXor, u"payload-xor");
    getHexaValue(_privateData, u"private-data");

    if (_payloadSize > 0 && _noPayload) {
        tsp->error(u"options --no-payload and --payload-size are mutually exclusive");
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::CraftPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Hack the packet header. Just overwrite a few bits in place, nothing to move.
    if (_clearTransportError) {
        pkt.clearTEI();
    }
    if (_setTransportError) {
        pkt.setTEI();
    }
    if (_clearTransportPriority) {
        pkt.clearPriority();
    }
    if (_setTransportPriority) {
        pkt.setPriority();
    }
    if (_clearPUSI) {
        pkt.clearPUSI();
    }
    if (_setPUSI) {
        pkt.setPUSI();
    }
    if (_setPID) {
        pkt.setPID(_newPID);
    }
    if (_setScrambling) {
        pkt.setScrambling(_newScrambling);
    }
    if (_setCC) {
        pkt.setCC(_newCC);
    }

    // Remove fields or clear bits in the adaptation field.
    // These operations always succeed and do not change the size of the AF,
    // they only potentially increase the stuffing part of the AF.
    if (_clearDiscontinuity) {
       pkt.clearDiscontinuityIndicator();
    }
    if (_clearRandomAccess) {
       pkt.clearRandomAccessIndicator();
    }
    if (_clearESPriority) {
       pkt.clearESPI();
    }
    if (_clearPCR) {
        pkt.removePCR();
    }
    if (_clearOPCR) {
        pkt.removeOPCR();
    }
    if (_clearSpliceCountdown) {
        pkt.removeSpliceCountdown();
    }
    if (_clearPrivateData) {
        pkt.removePrivateData();
    }
    if (_packPESHeader) {
        packPESHeader(pkt);
    }

    // Now modify the payload.
    // With --pes-payload, we may do that only if the TS contains the start of a PES packet with some PES payload.
    const size_t pesHeaderSize = pkt.getPESHeaderSize();
    const bool pesPayloadPresent = pesHeaderSize > 0 && pkt.getPayloadSize() > pesHeaderSize;
    const bool mayUpdatePayload = !_pesPayload || pesPayloadPresent;
    const size_t payloadBase = _pesPayload ? pesHeaderSize : 0;

    // If the payload must be resized to a specific size, do it now.
    if (mayUpdatePayload && _resizePayload && !pkt.setPayloadSize(payloadBase + _payloadSize, true, 0xFF)) {
        tsp->warning(u"packet %'d: cannot resize %s payload to %'d bytes", {tsp->pluginPackets(), _pesPayload ? u"PES" : u"TS", _payloadSize});
    }

    // Check if we are allowed to shrink the payload to any value.
    // We can shrink if the payload is replaced (--payload-pattern) and with no specified size.
    const bool canShrinkPayload = !_payloadPattern.empty() && !_resizePayload;

    // Set individual flags in AF. Try to create minimal AF with flags field.
    if (_setDiscontinuity && !pkt.setDiscontinuityIndicator(canShrinkPayload)) {
        tsp->warning(u"packet %'d: no adaptation field to set discontinuity indicator", {tsp->pluginPackets()});
    }
    if (_setESPriority && !pkt.setESPI(canShrinkPayload)) {
        tsp->warning(u"packet %'d: no adaptation field to set ES priority indicator", {tsp->pluginPackets()});
    }
    if (_setRandomAccess && !pkt.setRandomAccessIndicator(canShrinkPayload)) {
        tsp->warning(u"packet %'d: no adaptation field to set random access indicator", {tsp->pluginPackets()});
    }

    // Set fields which need more space in the adaptation field.
    if (_newPCR != INVALID_PCR && !pkt.setPCR(_newPCR, canShrinkPayload)) {
        tsp->warning(u"packet %'d: no adaptation field to set PCR", {tsp->pluginPackets()});
    }
    if (_newOPCR != INVALID_PCR && !pkt.setOPCR(_newOPCR, canShrinkPayload)) {
        tsp->warning(u"packet %'d: no adaptation field to set OPCR", {tsp->pluginPackets()});
    }
    if (_setSpliceCountdown && !pkt.setSpliceCountdown(_newSpliceCountdown, canShrinkPayload)) {
        tsp->warning(u"packet %'d: no adaptation field to set splicing point countdown", {tsp->pluginPackets()});
    }
    if (!_privateData.empty() && !pkt.setPrivateData(_privateData, canShrinkPayload)) {
        tsp->warning(u"packet %'d: adaptation field too short to set private data", {tsp->pluginPackets()});
    }

    // Fill payload with pattern.
    if (mayUpdatePayload) {
        updatePayload(pkt, payloadBase, _payloadPattern, Assign<uint8_t>{});
        updatePayload(pkt, payloadBase, _payloadAnd, AssignAnd<uint8_t>{});
        updatePayload(pkt, payloadBase, _payloadOr, AssignOr<uint8_t>{});
        updatePayload(pkt, payloadBase, _payloadXor, AssignXor<uint8_t>{});
    }

    // If the payload was explicitly resized to zero, set or reset payload presence.
    if (_resizePayload && _payloadSize == 0 && pkt.getPayloadSize() == 0) {
        if (_noPayload) {
            // Was resized with --no-payload, clear payload existence.
            pkt.b[3] &= ~0x10;
        }
        else {
            // Was resized with --payload-size 0, set payload existence (even if empty).
            pkt.b[3] |= 0x10;
        }
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Perform payload operations such as --payload-pattern, --payload-and, etc.
//----------------------------------------------------------------------------

template <typename Op>
void ts::CraftPlugin::updatePayload(TSPacket& pkt, size_t payloadBase, const ByteBlock& pattern, Op assign)
{
    if (!pattern.empty()) {
        const uint8_t* pat = pattern.data();
        const uint8_t* const endpat = pattern.data() + pattern.size();
        uint8_t* data = pkt.getPayload() + payloadBase + _offsetPattern;
        while (data < pkt.b + PKT_SIZE) {
            assign(*data++, *pat++);
            if (pat >= endpat) {
                if (_noRepeat) {
                    break;
                }
                else {
                    pat = pattern.data();
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Perform --pack-pes-header on a packet.
//----------------------------------------------------------------------------

void ts::CraftPlugin::packPESHeader(TSPacket& pkt)
{
    uint8_t* stuff = nullptr;
    size_t stuffSize = 0;
    size_t unused = 0;

    // Locate the stuffing area inside the PES header, if there is one.
    if (pkt.getPESHeaderStuffingArea(stuff, unused, stuffSize) && stuffSize > 0) {
        // The stuffing area is not empty and starts inside the TS payload. The value stuffSize is what we can pack.

        // TS packet payload:
        uint8_t* const pl = pkt.getPayload();
        const size_t plSize = pkt.getPayloadSize();
        assert(plSize >= 9 + stuffSize);

        // Adjust the PES header size:
        const size_t headerSize = 9 + size_t(pl[8]);
        assert(size_t(pl[8]) >= stuffSize);
        pl[8] -= uint8_t(stuffSize);

        // Adjust the PES packet size if not unbounded (ie. not zero).
        const size_t pesSize = GetUInt16(pl + 4);
        if (pesSize > stuffSize) {
            // Normally, should test != 0. But make sure that invalid small PES size does not cause an integer overflow.
            PutUInt16(pl + 4, uint16_t(pesSize - stuffSize));
        }

        // Shift the start of the TS payload to compress the PES header.
        ::memmove(pl + stuffSize, pl, std::min(headerSize, plSize) - stuffSize);

        // Now resize the TS payload
        pkt.setPayloadSize(plSize - stuffSize, false);
    }
}
