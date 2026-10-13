//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCraftPacketPlugin.h"
#include "tsPluginRepository.h"
#include "tsFunctors.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"craft", ts::CraftPacketPlugin);


//----------------------------------------------------------------------------
// Packet processing plugin constructor
//----------------------------------------------------------------------------

ts::CraftPacketPlugin::CraftPacketPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Craft specific low-level transformations on packets", u"[options]")
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

    option(u"rs204", 0, HEXADATA);
    help(u"rs204",
         u"Specify the binary content to store in the 16-byte trailer of a 204-byte packet. "
         u"If the packets have no trailer, one is created. "
         u"The value must be a string of hexadecimal digits specifying up to 16 bytes. "
         u"If the data are shorter than 16 bytes, they are padded with 0xFF.");

    option(u"delete-rs204");
    help(u"delete-rs204", u"Delete the 16-byte trailer of a 204-byte packet, if there is one.");
}


//----------------------------------------------------------------------------
// Packet processing plugin get command line options.
//----------------------------------------------------------------------------

bool ts::CraftPacketPlugin::getOptions()
{
    _set_discontinuity = present(u"discontinuity");
    _clear_discontinuity = present(u"clear-discontinuity");
    _set_transport_error = present(u"error");
    _clear_transport_error = present(u"clear-error");
    _set_transport_priority = present(u"priority");
    _clear_transport_priority = present(u"clear-priority");
    _set_es_priority = present(u"es-priority");
    _clear_es_priority = present(u"clear-es-priority");
    _no_payload = present(u"no-payload");
    _no_repeat = present(u"no-repeat");
    _resize_payload = present(u"payload-size") || _no_payload;
    getIntValue(_payload_size, u"payload-size", 0);
    _pes_payload = present(u"pes-payload");
    getIntValue(_offset_pattern, u"offset-pattern", 0);
    _clear_pcr = present(u"no-pcr");
    getIntValue(_new_pcr, u"pcr", INVALID_PCR);
    _clear_opcr = present(u"no-opcr");
    getIntValue(_new_opcr, u"opcr", INVALID_PCR);
    _set_pid = present(u"pid");
    getIntValue(_new_pid, u"pid");
    _set_pusi = present(u"pusi");
    _clear_pusi = present(u"clear-pusi");
    _set_random_access = present(u"random-access");
    _clear_random_access = present(u"clear-random-access");
    _pack_pes_header = present(u"pack-pes-header");
    _set_scrambling = present(u"scrambling");
    getIntValue(_new_scrambling, u"scrambling");
    _set_cc = present(u"continuity-counter");
    getIntValue(_new_cc, u"continuity-counter");
    _set_splice_countdown = present(u"splice-countdown");
    _clear_splice_countdown = present(u"no-splice-countdown");
    getIntValue(_new_splice_countdown, u"splice-countdown");
    _clear_private_data = present(u"no-private-data");
    getHexaValue(_payload_pattern, u"payload-pattern");
    getHexaValue(_payload_and, u"payload-and");
    getHexaValue(_payload_or, u"payload-or");
    getHexaValue(_payload_xor, u"payload-xor");
    getHexaValue(_private_data, u"private-data");
    _delete_rs204 = present(u"delete-rs204");
    getHexaValue(_rs204, u"rs204");
    if (!_rs204.empty()) {
        _rs204.resize(RS_SIZE, 0xFF);
    }

    if (_payload_size > 0 && _no_payload) {
        error(u"options --no-payload and --payload-size are mutually exclusive");
        return false;
    }
    if (!_rs204.empty() && _delete_rs204) {
        error(u"options --rs204 and --delete-rs204 are mutually exclusive");
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::CraftPacketPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Hack the packet header. Just overwrite a few bits in place, nothing to move.
    if (_clear_transport_error) {
        pkt.clearTEI();
    }
    if (_set_transport_error) {
        pkt.setTEI();
    }
    if (_clear_transport_priority) {
        pkt.clearPriority();
    }
    if (_set_transport_priority) {
        pkt.setPriority();
    }
    if (_clear_pusi) {
        pkt.clearPUSI();
    }
    if (_set_pusi) {
        pkt.setPUSI();
    }
    if (_set_pid) {
        pkt.setPID(_new_pid);
    }
    if (_set_scrambling) {
        pkt.setScrambling(_new_scrambling);
    }
    if (_set_cc) {
        pkt.setCC(_new_cc);
    }

    // Remove fields or clear bits in the adaptation field.
    // These operations always succeed and do not change the size of the AF,
    // they only potentially increase the stuffing part of the AF.
    if (_clear_discontinuity) {
       pkt.clearDiscontinuityIndicator();
    }
    if (_clear_random_access) {
       pkt.clearRandomAccessIndicator();
    }
    if (_clear_es_priority) {
       pkt.clearESPI();
    }
    if (_clear_pcr) {
        pkt.removePCR();
    }
    if (_clear_opcr) {
        pkt.removeOPCR();
    }
    if (_clear_splice_countdown) {
        pkt.removeSpliceCountdown();
    }
    if (_clear_private_data) {
        pkt.removePrivateData();
    }
    if (_pack_pes_header) {
        packPESHeader(pkt);
    }

    // Now modify the payload.
    // With --pes-payload, we may do that only if the TS contains the start of a PES packet with some PES payload.
    const size_t pesHeaderSize = pkt.getPESHeaderSize();
    const bool pesPayloadPresent = pesHeaderSize > 0 && pkt.getPayloadSize() > pesHeaderSize;
    const bool mayUpdatePayload = !_pes_payload || pesPayloadPresent;
    const size_t payloadBase = _pes_payload ? pesHeaderSize : 0;

    // If the payload must be resized to a specific size, do it now.
    if (mayUpdatePayload && _resize_payload && !pkt.setPayloadSize(payloadBase + _payload_size, true, 0xFF)) {
        warning(u"packet %'d: cannot resize %s payload to %'d bytes", tsp->pluginPackets(), _pes_payload ? u"PES" : u"TS", _payload_size);
    }

    // Check if we are allowed to shrink the payload to any value.
    // We can shrink if the payload is replaced (--payload-pattern) and with no specified size.
    const bool canShrinkPayload = !_payload_pattern.empty() && !_resize_payload;

    // Set individual flags in AF. Try to create minimal AF with flags field.
    if (_set_discontinuity && !pkt.setDiscontinuityIndicator(canShrinkPayload)) {
        warning(u"packet %'d: no adaptation field to set discontinuity indicator", tsp->pluginPackets());
    }
    if (_set_es_priority && !pkt.setESPI(canShrinkPayload)) {
        warning(u"packet %'d: no adaptation field to set ES priority indicator", tsp->pluginPackets());
    }
    if (_set_random_access && !pkt.setRandomAccessIndicator(canShrinkPayload)) {
        warning(u"packet %'d: no adaptation field to set random access indicator", tsp->pluginPackets());
    }

    // Set fields which need more space in the adaptation field.
    if (_new_pcr != INVALID_PCR && !pkt.setPCR(_new_pcr, canShrinkPayload)) {
        warning(u"packet %'d: no adaptation field to set PCR", tsp->pluginPackets());
    }
    if (_new_opcr != INVALID_PCR && !pkt.setOPCR(_new_opcr, canShrinkPayload)) {
        warning(u"packet %'d: no adaptation field to set OPCR", tsp->pluginPackets());
    }
    if (_set_splice_countdown && !pkt.setSpliceCountdown(_new_splice_countdown, canShrinkPayload)) {
        warning(u"packet %'d: no adaptation field to set splicing point countdown", tsp->pluginPackets());
    }
    if (!_private_data.empty() && !pkt.setPrivateData(_private_data, canShrinkPayload)) {
        warning(u"packet %'d: adaptation field too short to set private data", tsp->pluginPackets());
    }

    // Fill payload with pattern.
    if (mayUpdatePayload) {
        updatePayload(pkt, payloadBase, _payload_pattern, Assign<uint8_t>{});
        updatePayload(pkt, payloadBase, _payload_and, AssignAnd<uint8_t>{});
        updatePayload(pkt, payloadBase, _payload_or, AssignOr<uint8_t>{});
        updatePayload(pkt, payloadBase, _payload_xor, AssignXor<uint8_t>{});
    }

    // If the payload was explicitly resized to zero, set or reset payload presence.
    if (_resize_payload && _payload_size == 0 && pkt.getPayloadSize() == 0) {
        if (_no_payload) {
            // Was resized with --no-payload, clear payload existence.
            pkt.b[3] &= ~0x10;
        }
        else {
            // Was resized with --payload-size 0, set payload existence (even if empty).
            pkt.b[3] |= 0x10;
        }
    }

    // Process metadata.
    if (_delete_rs204) {
        pkt_data.setAuxData(nullptr, 0);
    }
    else if (!_rs204.empty()) {
        pkt_data.setAuxData(_rs204.data(), _rs204.size());
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Perform payload operations such as --payload-pattern, --payload-and, etc.
//----------------------------------------------------------------------------

template <typename Op>
void ts::CraftPacketPlugin::updatePayload(TSPacket& pkt, size_t payload_base, const ByteBlock& pattern, Op assign)
{
    if (!pattern.empty()) {
        const uint8_t* pat = pattern.data();
        const uint8_t* const endpat = pattern.data() + pattern.size();
        uint8_t* data = pkt.getPayload() + payload_base + _offset_pattern;
        while (data < pkt.b + PKT_SIZE) {
            assign(*data++, *pat++);
            if (pat >= endpat) {
                if (_no_repeat) {
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

void ts::CraftPacketPlugin::packPESHeader(TSPacket& pkt)
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
        MemCopy(pl + stuffSize, pl, std::min(headerSize, plSize) - stuffSize);

        // Now resize the TS payload
        pkt.setPayloadSize(plSize - stuffSize, false);
    }
}
