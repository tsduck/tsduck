//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Copy PCR values from a PID into another (with packet distance adjustment)
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsByteBlock.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCRCopyPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PCRCopyPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        PID           _ref_pid_arg = PID_NULL;     // Reference PCR source.
        PID           _target_pid_arg = PID_NULL;  // Target PID to alter.
        size_t        _ref_label = NPOS;           // Label which indicates the reference PID.
        size_t        _target_label = NPOS;        // Label which indicates the target PID.
        PacketCounter _every = 0;                  // Insert a PCR every N packets (if not zero).
        size_t        _max_shift = 0;              // Maximum number of bytes to shift.
        bool          _pusi = false;               // Insert a PCR in PUSI packets.

        // Working data.
        PID           _ref_pid = PID_NULL;         // Current reference PCR source.
        PID           _target_pid = PID_NULL;      // Current target PID to alter.
        PacketCounter _target_packets = 0;         // Number of packets in target PID.
        PacketCounter _ref_packet = 0;             // Packet index of last PCR in reference PID.
        uint64_t      _ref_pcr = INVALID_PCR;      // Last PCR value in reference PID.
        uint8_t       _target_cc_in = 0;           // Last read continuity counter in target PID.
        uint8_t       _target_cc_out = 0;          // Last written continuity counter in target PID.
        bool          _shift_overflow = false;     // Overflow in target shift buffer, resync at next PUSI.
        size_t        _shift_pusi = NPOS;          // Position of a PUSI in shift buffer (NPOS if there is none).
        ByteBlock     _shift_buffer {};            // Buffer for shifted payload.

        // Process a packet from the target PID, insert PCR when needed, shift payload.
        // Can also be used on the null PID to insert shifted payload.
        void processTargetPacket(TSPacket& pkt);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pcrcopy", ts::PCRCopyPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCRCopyPlugin::PCRCopyPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Copy and synchronize PCR's from one PID to another", u"[options]")
{
    option(u"reference-pid", 'r', PIDVAL);
    help(u"reference-pid",
         u"PID containing the reference PCR to copy. "
         u"Exactly one of --reference-pid and --reference-label shall be specified.");

    option(u"reference-label", 0, INTEGER, 0, 0, 0, TSPacketLabelSet::MAX);
    help(u"reference-label",
         u"Packet label indicating the PID containing the reference PCR to copy. "
         u"Each time a packet with that label is encountered, the reference PID switches "
         u"to the PID of this packet, if different from the previous reference PID. "
         u"Exactly one of --reference-pid and --reference-label shall be specified.");

    option(u"target-pid", 't', PIDVAL);
    help(u"target-pid",
         u"PID into which PCR shall be created and copied. "
         u"Exactly one of --target-pid and --target-label shall be specified.");

    option(u"target-label", 0, INTEGER, 0, 0, 0, TSPacketLabelSet::MAX);
    help(u"target-label",
         u"Packet label indicating the PID containing the target PID into which PCR shall be created and copied. "
         u"Each time a packet with that label is encountered, the target PID switches "
         u"to the PID of this packet, if different from the previous target PID. "
         u"Exactly one of --target-pid and --target-label shall be specified.");

    option(u"every", 'e', POSITIVE);
    help(u"every", u"packet-count",
         u"Insert a PCR every N packets in the target PID. "
         u"By default, insert a PCR in packets with a payload unit start only.");

    option(u"no-pusi", 'n');
    help(u"no-pusi",
         u"Do not insert a PCR in packets with a payload unit start indicator (PUSI). "
         u"By default, a PCR is inserted in all PUSI packets, even if --every is also specified.");

    option(u"max-shift", 0, INTEGER, 0, 1, PKT_MAX_PAYLOAD_SIZE, UNLIMITED_VALUE);
    help(u"max-shift", u"bytes",
         u"Maximum number of target packet payload bytes which can be shifted, due to PCR insertion. "
         u"When this value is reached, usually because of a lack of null packets, the current PES packet is truncated. "
         u"By default, allow the buffering of up to 16 packet payloads.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::PCRCopyPlugin::getOptions()
{
    _pusi = !present(u"no-pusi");
    getIntValue(_ref_pid_arg, u"reference-pid", PID_NULL);
    getIntValue(_target_pid_arg, u"target-pid", PID_NULL);
    getIntValue(_ref_label, u"reference-label", TSPacketLabelSet::MAX + 1);
    getIntValue(_target_label, u"target-label", TSPacketLabelSet::MAX + 1);
    getIntValue(_every, u"every");
    getIntValue(_max_shift, u"max-shift", 16 * PKT_MAX_PAYLOAD_SIZE);

    if (count(u"reference-pid") + count(u"reference-label") != 1) {
        error(u"Exactly one of --reference-pid and --reference-label shall be specified.");
        return false;
    }
    if (count(u"target-pid") + count(u"target-label") != 1) {
        error(u"Exactly one of --target-pid and --target-label shall be specified.");
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRCopyPlugin::start()
{
    _ref_pid = _ref_pid_arg;
    _ref_packet = 0;
    _ref_pcr = INVALID_PCR;

    _target_pid = _target_pid_arg;
    _target_packets = 0;
    _target_cc_in = _target_cc_out = CC_MAX; // invalid CC value

    _shift_buffer.clear();
    _shift_buffer.reserve(_max_shift);
    _shift_pusi = NPOS;
    _shift_overflow = false;
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCRCopyPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Process PID switching according to labels.
    if (_ref_label <= TSPacketLabelSet::MAX && pkt_data.hasLabel(_ref_label) && pid != _ref_pid && pid != PID_NULL) {
        // Switch to a new reference PID.
        verbose(u"using PID %n as PCR reference", pid);
        _ref_pid = pid;
        _ref_pcr = INVALID_PCR;
    }
    if (_target_label <= TSPacketLabelSet::MAX && pkt_data.hasLabel(_target_label) && pid != _target_pid && pid != PID_NULL) {
        // Switch to a new target PID.
        verbose(u"using PID %n to insert copied PCR", pid);
        _target_pid = pid;
        _target_packets = 0;
        _target_cc_in = _target_cc_out = CC_MAX; // invalid CC value
        _shift_buffer.clear();
        _shift_pusi = NPOS;
        _shift_overflow = false;
    }

    // Process packet content.
    if (pid == _ref_pid && pkt.hasPCR() && pid != PID_NULL) {
        // Collect PCR's in reference PID.
        _ref_pcr = pkt.getPCR();
        _ref_packet = tsp->pluginPackets();
    }
    else if (pid == _target_pid && pid != PID_NULL && pid != _ref_pid) {
        // Process a packet from the target PID.
        processTargetPacket(pkt);
    }
    else if (pid == PID_NULL && (_shift_buffer.size() >= PKT_MAX_PAYLOAD_SIZE || _shift_pusi != NPOS)) {
        // Steal null packet to copy a full shifted payload or end of shifted PES packet.
        processTargetPacket(pkt);
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Process a packet from the target PID, insert PCR when needed, shift payload.
//----------------------------------------------------------------------------

void ts::PCRCopyPlugin::processTargetPacket(TSPacket& pkt)
{
    // At the start of a PES packet, check the overflow status of the shift buffer.
    if (pkt.getPUSI()) {
        if (_shift_overflow) {
            // We had a shift overflow and this TS packet starts a new PES packet,
            // forget the overflown shift and restart on the current PES packet.
            _shift_buffer.clear();
            _shift_pusi = NPOS;
            _shift_overflow = false;
        }
        else if (_shift_pusi != NPOS) {
            // There is a full PES packet in the shift buffer, we cannot accumulate them.
            warning(u"dropping complete PES packet, not enought null packets to absorb the shift");
            assert(_shift_pusi <= _shift_buffer.size());
            _shift_buffer.resize(_shift_pusi);
            _shift_pusi = NPOS;
        }
    }

    // Check if we need to insert a PCR here.
    const bool set_pcr =
        // We can compute a PCR only if we have a reference.
        (_ref_pcr != INVALID_PCR) &&
        // And if we are at a PCR insertion point. If _shift_pusi is zero, this means that
        // we will replace the content of this packet with the start of a PES packet.
        ((_pusi && (pkt.getPUSI() || _shift_pusi == 0)) || (_every != 0 && _target_packets % _every == 0));

    // Count packet in the target PID.
    _target_packets++;

    // Prepare the packet for the target PID when coming from another PID (typically a null packet).
    const bool new_packet = pkt.getPID() != _target_pid;
    if (new_packet) {
        pkt.init(_target_pid);
    }

    // Check if there is a discontinuity.
    const uint8_t cc = pkt.getCC();
    const bool discontinuity = !new_packet && _target_cc_in < CC_MAX && cc != ((_target_cc_in + 1) & CC_MASK);

    // Keep track of input continuity counters.
    if (!new_packet) {
        _target_cc_in = cc;
    }

    // Compute next continuity counters.
    if (_target_cc_out >= CC_MAX) {
        // First output packet on target PID, use first input CC.
        _target_cc_out = _target_cc_in;
    }
    else if (discontinuity) {
        // Recreate a discontinuity by adding 2 to CC instead of 1.
        _target_cc_out = (_target_cc_out + 2) & CC_MASK;
    }
    else {
        // Normal packet, preserve output continuity.
        _target_cc_out = (_target_cc_out + 1) & CC_MASK;
    }
    pkt.setCC(_target_cc_out);

    // Check if the packet paylaad is significant.
    bool unused_payload = new_packet;

    // If the shift buffer is not empty, add the packet payload at end of shift buffer.
    // Also do it if we need to insert a PCR and there is currently none (we will shrink the payload).
    if (!unused_payload && (!_shift_buffer.empty() || (set_pcr && !pkt.hasPCR()))) {
        // Shift the payload only if there was no previous overflow.
        if (!_shift_overflow) {
            if (pkt.getPUSI()) {
                // Mark the start of a PES packet in the shift buffer.
                // Note that we previously checked that there was none (or we removed it).
                _shift_pusi = _shift_buffer.size();
                pkt.clearPUSI();
            }
            // Append the packet payload in the shift buffer.
            _shift_buffer.append(pkt.getPayload(), pkt.getPayloadSize());
        }
        // Mark the packet payload as unused since it was moved into the shift buffer.
        unused_payload = true;
    }

    // Compute and insert a PCR when needed.
    if (set_pcr) {
        // Compute PCR value from the previous reference PCR value and the bitrate.
        // If the bitrate is unknown, keep the reference PCR, even though we know it is incorrect.
        const BitRate bitrate = tsp->bitrate();
        const uint64_t pcr = _ref_pcr + (bitrate == 0 ? 0 : (((tsp->pluginPackets() - _ref_packet) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / bitrate).toInt());

        // Replace the PCR value. We know that we can safely overwrite the payload if the
        // adaptation field must be extended since we saved the payload in the shift buffer.
        pkt.setPCR(pcr, true);
    }

    // Fill the packet payload. There is no need to do that if the payload is not unused
    // because is means that the shift buffer was empty and the payload was not resized,
    // meaning there is nothing to do.
    if (unused_payload) {
        // How much space can we get in the packet for an updated payload?
        const size_t available = pkt.getAFStuffingSize() + pkt.getPayloadSize();
        // Maximum space we can get from the shift buffer (not crossing a PUSI).
        const size_t max_from_shift = (_shift_pusi == 0 || _shift_pusi == NPOS) ? _shift_buffer.size() : _shift_pusi;
        // Resize the packet payload from what we can get.
        const size_t size = std::min(available, max_from_shift);
        pkt.setPayloadSize(size);
        // Copy the new paylaod from the start of the shift buffer.
        if (size > 0) {
            MemCopy(pkt.getPayload(), _shift_buffer.data(), size);
            _shift_buffer.erase(0, size);
            if (_shift_pusi == 0) {
                // The PUSI has moved from the shift buffer to the packet.
                pkt.setPUSI();
                _shift_pusi = NPOS;
            }
            else if (_shift_pusi != NPOS) {
                assert(_shift_pusi >= size);
                _shift_pusi -= size;
            }
        }
    }

    // Check if there is an overflow in the shift buffer after all adjustments.
    if (!_shift_overflow && _shift_buffer.size() > _max_shift) {
        warning(u"dropping partial PES packet, not enought null packets to absorb the shift");
        _shift_overflow = true;
    }
}
