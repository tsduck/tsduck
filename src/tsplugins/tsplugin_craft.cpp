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
        virtual size_t receive(TSPacket*, size_t) override;
        virtual bool abortInput() override { return true; }

    private:
        uint8_t       _initCC;      // continuity_counter
        bool          _constantCC;  // Do not increment continuity counter
        PacketCounter _maxCount;    // Number of packets to generate
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
        size_t    _payloadSize;
        ByteBlock _payloadPattern;
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
    };
}


//----------------------------------------------------------------------------
// Plugin shared library interface
//----------------------------------------------------------------------------

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(craft, ts::CraftInput)
TSPLUGIN_DECLARE_PROCESSOR(craft, ts::CraftPlugin)


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
    help(u"no-payload", u"Do not use a payload, equivalent to --payload-size 0.");

    option(u"payload-pattern", 0, STRING);
    help(u"payload-pattern",
         u"Specify the binary pattern to apply on packets payload. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"The pattern is repeated to fill the payload. The default is FF.");

    option(u"payload-size", 0, INTEGER, 0, 1, 0, 184);
    help(u"payload-size", u"size",
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
    const bool fullPayload = !present(u"no-payload") && !present(u"payload-size");
    size_t payloadSize = intValue<size_t>(u"payload-size");

    // Check consistency of options.
    if (payloadSize > 0 && present(u"no-payload")) {
        tsp->error(u"options --no-payload and --payload-size are mutually exclusive");
        return false;
    }

    // The binary patterns.
    ByteBlock payloadPattern;
    if (!value(u"payload-pattern", u"FF").hexaDecode(payloadPattern) || payloadPattern.size() == 0) {
        tsp->error(u"invalid hexadecimal payload pattern");
        return false;
    }

    ByteBlock privateData;
    if (!value(u"private-data").hexaDecode(privateData)) {
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
        (payloadSize > 0 ? 0x10 : 0x00) |
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
// Input method
//----------------------------------------------------------------------------

size_t ts::CraftInput::receive(TSPacket* buffer, size_t maxPackets)
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
    _payloadSize(0),
    _payloadPattern(),
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
    help(u"no-payload", u"Remove the payload, equivalent to --payload-size 0.");

    option(u"payload-pattern", 0, STRING);
    help(u"payload-pattern",
         u"Overwrite the payload and specify the binary pattern to apply. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"The pattern is repeated to fill the payload.");

    option(u"payload-size", 0, INTEGER, 0, 1, 0, 184);
    help(u"payload-size", u"size",
         u"Resize the packet payload to the specified value in bytes. "
         u"When necessary, an adaptation field is created or enlarged. "
         u"Without --payload-pattern, the existing payload is either shrunk or enlarged. "
         u"When an existing payload is shrunk, the end of the payload is truncated. "
         u"When an existing payload is enlarged, its end is padded with 0xFF bytes. ");

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

    option(u"private-data", 0, STRING);
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
    _resizePayload = present(u"payload-size") || present(u"no-payload");
    _payloadSize = intValue<size_t>(u"payload-size", 0);
    _clearPCR = present(u"no-pcr");
    _newPCR = intValue<uint64_t>(u"pcr", INVALID_PCR);
    _clearOPCR = present(u"no-opcr");
    _newOPCR = intValue<uint64_t>(u"opcr", INVALID_PCR);
    _setPID = present(u"pid");
    _newPID = intValue<PID>(u"pid");
    _setPUSI = present(u"pusi");
    _clearPUSI = present(u"clear-pusi");
    _setRandomAccess = present(u"random-access");
    _clearRandomAccess = present(u"clear-random-access");
    _packPESHeader = present(u"pack-pes-header");
    _setScrambling = present(u"scrambling");
    _newScrambling = intValue<uint8_t>(u"scrambling");
    _setCC = present(u"continuity-counter");
    _newCC = intValue<uint8_t>(u"continuity-counter");
    _setSpliceCountdown = present(u"splice-countdown");
    _clearSpliceCountdown = present(u"no-splice-countdown");
    _newSpliceCountdown = intValue<uint8_t>(u"splice-countdown");
    _clearPrivateData = present(u"no-private-data");

    if (!value(u"payload-pattern").hexaDecode(_payloadPattern)) {
        tsp->error(u"invalid hexadecimal payload pattern");
        return false;
    }

    if (!value(u"private-data").hexaDecode(_privateData)) {
        tsp->error(u"invalid hexadecimal private data");
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

    // If the payload must be resized to a specific size, do it now.
    if (_resizePayload && !pkt.setPayloadSize(_payloadSize, true, 0xFF)) {
        tsp->warning(u"packet %'d: cannot resize payload to %'d bytes", {tsp->pluginPackets(), _payloadSize});
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
    if (!_payloadPattern.empty()) {
        uint8_t* data = pkt.getPayload();
        while (data < pkt.b + PKT_SIZE) {
            const size_t size = std::min<size_t>(_payloadPattern.size(), pkt.b + PKT_SIZE - data);
            ::memcpy(data, _payloadPattern.data(), size);
            data += size;
        }
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Perform --pack-pes-header on a packet.
//----------------------------------------------------------------------------

void ts::CraftPlugin::packPESHeader(TSPacket& pkt)
{
    // If there is no clear PES header inside the packet, nothing to do.
    if (!pkt.startPES()) {
        return;
    }

    // Analyze the start of the payload, supposed to be a PES header.
    uint8_t* const pl = pkt.getPayload();
    const size_t plSize = pkt.getPayloadSize();
    if (plSize < 9 || !IsLongHeaderSID(pl[3])) {
        // Can't get the start of a long header or this stream id does not have a long header.
        return;
    }

    // Size of the PES header, may include stuffing.
    const size_t headerSize = 9 + size_t(pl[8]);

    // Look for the offset of the stuffing in the PES packet.
    size_t offset = 9;
    const uint8_t PTS_DTS_flags = (pl[7] >> 6) & 0x03;
    if (offset < headerSize && PTS_DTS_flags == 2) {
        offset += 5;  // skip PTS
    }
    if (offset < headerSize && PTS_DTS_flags == 3) {
        offset += 10;  // skip PTS and DTS
    }
    if (offset < headerSize && (pl[7] & 0x20) != 0) {
        offset += 6;  // ESCR_flag set, skip ESCR
    }
    if (offset < headerSize && (pl[7] & 0x10) != 0) {
        offset += 3;  // ES_rate_flag set, skip ES_rate
    }
    if (offset < headerSize && (pl[7] & 0x08) != 0) {
        offset += 1;  // DSM_trick_mode_flag set, skip trick mode
    }
    if (offset < headerSize && (pl[7] & 0x04) != 0) {
        offset += 1;  // additional_copy_info_flag set, skip additional_copy_info
    }
    if (offset < headerSize && (pl[7] & 0x02) != 0) {
        offset += 2;  // PES_CRC_flag set, skip previous_PES_packet_CRC
    }
    if (offset < headerSize && offset < plSize && (pl[7] & 0x01) != 0) {
        // PES_extension_flag set, analyze and skip PES extensions
        // First, get the flags indicating which extensions are present.
        const uint8_t flags = pl[offset++];
        if (offset < headerSize && (flags & 0x80) != 0) {
            offset += 16; // PES_private_data_flag set
        }
        if (offset < headerSize && offset < plSize && (flags & 0x40) != 0) {
            offset += 1 + pl[offset]; // pack_header_field_flag set
        }
        if (offset < headerSize && (flags & 0x20) != 0) {
            offset += 2; // program_packet_sequence_counter_flag set
        }
        if (offset < headerSize && (flags & 0x10) != 0) {
            offset += 2; // P-STD_buffer_flag set
        }
        if (offset < headerSize && offset < plSize && (flags & 0x01) != 0) {
            offset += 1 + (pl[offset] & 0x7F); //  PES_extension_flag_2 set
        }
    }

    // Now, offset points to the beginning of the stuffing area in the PES header.
    if (offset < headerSize && offset < plSize) {
        // The stuffing area is not empty and starts inside the TS payload.
        // Compute how many stuffing bytes we have inside the TS payload.
        // This is the size we may remove from the PES header.
        const size_t stuffSize = std::min(headerSize - offset, plSize - offset);
        // Adjust the PES header size.
        assert(size_t(pl[8]) >= stuffSize);
        pl[8] -= uint8_t(stuffSize);
        // Adjust the PES packet size if not unbounded (ie. not zero).
        const uint16_t pesSize = GetUInt16(pl + 4);
        if (pesSize > stuffSize) {
            // Normally, should test != 0. But make sure that invalid small PES size does not cause an integer overflow.
            PutUInt16(pl + 4, pesSize - uint16_t(stuffSize));
        }
        // Shift the start of the TS payload to compress the PES header.
        ::memmove(pl + stuffSize, pl, std::min(headerSize, plSize) - stuffSize);
        // Now resize the TS payload
        pkt.setPayloadSize(plSize - stuffSize, false);
    }
}
