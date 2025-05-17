//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard (PES mode by lars18th)
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPacketEncapsulation.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::PacketEncapsulation::PacketEncapsulation(Report& report,
                                             PID output_pid,
                                             const PIDSet& input_pids,
                                             const TSPacketLabelSet& input_labels,
                                             PID pcr_reference_pid,
                                             size_t pcr_reference_label) :
    _report(report),
    _output_pid(output_pid),
    _input_pids(input_pids),
    _input_labels(input_labels),
    _pcr_ref_pid(pcr_reference_pid),
    _pcr_ref_label(pcr_reference_label)
{
}


//----------------------------------------------------------------------------
// Reset the encapsulation.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::reset(PID output_pid, const PIDSet& input_pids, const TSPacketLabelSet& input_labels, PID pcr_reference_pid, size_t pcr_reference_label)
{
    _packing = false;
    _pack_distance = NPOS;
    _pes_mode = DISABLED;
    _pes_offset = 0;
    _output_pid = output_pid;
    _input_pids = input_pids;
    _input_labels = input_labels;
    _pcr_ref_pid = pcr_reference_pid;
    _pcr_ref_label = pcr_reference_label;
    _last_error.clear();
    _current_packet = 0;
    _cc_output = 0;
    _cc_pes = 1;
    _last_cc.clear();
    _late_distance = 0;
    _late_index = 0;
    _late_packets.clear();
    _delayed_initial = 0;
    resetPCR();
}


//----------------------------------------------------------------------------
// Change the output PID.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::setOutputPID(PID pid)
{
    if (pid != _output_pid) {
        _output_pid = pid;
        // Reset encapsulation.
        _cc_output = 0;
        _cc_pes = 1;
        _last_cc.clear();
        _late_distance = 0;
        _late_index = 0;
        _late_packets.clear();
        _delayed_initial = 0;
    }
}


//----------------------------------------------------------------------------
// Set PES Offset.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::setPESOffset(int32_t offset)
{
    _pes_offset = offset >= 0 ? uint64_t(offset) : PTS_DTS_SCALE - std::abs(offset);
}


//----------------------------------------------------------------------------
// Set the maximum number of buffered packets.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::setMaxBufferedPackets(size_t count)
{
    // Always keep some margin.
    _late_max_packets = std::max<size_t>(8, count);
}


//----------------------------------------------------------------------------
// Change the reference PID for PCR's.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::setReferencePCR(PID pid)
{
    if (pid != _pcr_ref_pid) {
        // Reference PID modified, reset synchro.
        _pcr_ref_pid = pid;
        resetPCR();
    }
}


//----------------------------------------------------------------------------
// Reset PCR information, lost synchronization.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::resetPCR()
{
    _pcr_last_packet = INVALID_PACKET_COUNTER;
    _pcr_last_value = INVALID_PCR;
    _bitrate = 0;
    _insert_pcr = false;
}


//----------------------------------------------------------------------------
// Replace the set of input. The null PID can never be encapsulated.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::setInputPIDs(const PIDSet& input_pids)
{
    _input_pids = input_pids;
    _input_pids.reset(PID_NULL);
}

void ts::PacketEncapsulation::setInputLabels(const TSPacketLabelSet& input_labels)
{
    _input_labels = input_labels;
}

void ts::PacketEncapsulation::addInputPID(PID pid)
{
    if (pid < PID_NULL) {
        _input_pids.set(pid);
    }
}

void ts::PacketEncapsulation::removeInputPID(PID pid)
{
    if (pid < PID_NULL) {
        _input_pids.reset(pid);
    }
}


//----------------------------------------------------------------------------
// Process a TS packet from the input stream.
//----------------------------------------------------------------------------

bool ts::PacketEncapsulation::processPacket(TSPacket& pkt, TSPacketMetadata& mdata)
{
    PID pid = pkt.getPID();
    bool status = true;  // final return value.

    // Keep track of continuity counter per PID, detect discontinuity.
    // Do not check discontinuity on the stuffing PID, there is none.
    if (pid != PID_NULL) {
        const auto icc = _last_cc.find(pid);
        const uint8_t cc = pkt.getCC();
        const bool has_payload = pkt.hasPayload();
        if (icc == _last_cc.end()) {
            // No CC found so far on this PID. Just keep this one.
            _last_cc.insert(std::make_pair(pid, cc));
        }
        else {
            if ((has_payload && cc != ((icc->second + 1) & CC_MASK)) || (!has_payload && cc != icc->second)) {
                // Discontinuity detected, forget information about PCR, they will be incorrect.
                resetPCR();
            }
            icc->second = cc;
        }
    }

    // Collect PCR from the reference PID to compute bitrate.
    if (((_pcr_ref_pid != PID_NULL && pid == _pcr_ref_pid) || (_pcr_ref_label <= TSPacketLabelSet::MAX && mdata.hasLabel(_pcr_ref_label))) && pkt.hasPCR()) {
        const uint64_t pcr = pkt.getPCR();
        // If previous PCR is known, compute bitrate. Ignore PCR value wrap-up.
        if (_pcr_last_value != INVALID_PCR && _pcr_last_value < pcr) {
            assert(_pcr_last_packet < _current_packet);
            // Compute TS bitrate since last PCR.
            _bitrate = PacketBitRate(_current_packet - _pcr_last_packet, PCR(pcr - _pcr_last_value));
            // Insert PCR in output PID asap after a PCR on reference PID when the bitrate is known.
            _insert_pcr = true;
        }
        // Save current PCR.
        _pcr_last_packet = _current_packet;
        _pcr_last_value = pcr;
    }

    // Detect PID conflicts (when the output PID is present on input but not encapsulated).
    if (pid == _output_pid && !_input_pids.test(pid)) {
        _last_error.format(u"PID conflict, output PID %n is present but not encapsulated", pid);
        status = false;
    }

    // Increase the counter of the distance with each incoming packet.
    _late_distance++;

    // We need to guarantee the limits of all packages.
    // When the buffer is empty we alredy set the late pointer to the first byte after 0x47.
    if (_late_index < 1) {
        _late_index = 1;
    }

    // Can we compute a PCR for the current packet?
    const bool pcr_known = _bitrate != 0 && _pcr_last_packet != INVALID_PACKET_COUNTER && _pcr_last_value != INVALID_PCR;

    // Check if we need to generate a PTS in the packet.
    const bool need_pts = _pes_mode != DISABLED && _pes_offset != 0;

    // If this packet is part of the input set, place it in the "late" queue.
    // Note that a packet always need to go into the queue, even if the queue is empty because
    // no input packet can fit into an output packet. At least a few bytes need to be queued.
    if ((_input_pids.test(pid) || mdata.hasAnyLabel(_input_labels)) && _output_pid != PID_NULL) {
        if (need_pts && !pcr_known) {
            // Synchronous PES mode, before getting a PCR. We can't compute a PTS.
            // Delay packets until we can compute a PTS. Count and report delayed packets.
            if (_drop_before_pts) {
                // Drop initial packets.
                if (_delayed_initial == 0) {
                    _report.verbose(u"start dropping packets in PID %n, waiting for a PCR to compute PTS", _output_pid);
                }
            }
            else {
                // Delay initial packets.
                if (_delayed_initial == 0) {
                    _report.verbose(u"start delaying packets in PID %n, waiting for a PCR to compute PTS", _output_pid);
                }
                // In the initial phase of synchronous PES mode, delay packets before PTS can be computed.
                if (_late_packets.size() > _late_max_packets) {
                    // Delay queue is too long, drop initial packets.
                    _late_packets.pop_front();
                    _late_index = 1;
                }
                // Enqueue the packet.
                _late_packets.push_back(std::make_shared<TSPacket>(pkt));
                // If this is the first packet in the queue, point to the first byte after 0x47.
                if (_late_packets.size() == 1) {
                    _late_index = 1;
                }
            }
            _delayed_initial++;
        }
        else if (_late_packets.size() > _late_max_packets) {
            _last_error.assign(u"buffered packets overflow, insufficient null packets in input stream");
            status = false;
        }
        else {
            // Enqueue the packet.
            _late_packets.push_back(std::make_shared<TSPacket>(pkt));
            // If this is the first packet in the queue, point to the first byte after 0x47.
            if (_late_packets.size() == 1) {
                _late_index = 1;
            }
        }
        // Pretend that the input packet is a null one.
        pkt = NullPacket;
        pid = PID_NULL;
    }

    // Check PES mode support
    if (_pes_mode > VARIABLE) {
        _last_error.assign(u"PES mode %d is not implemented", _pes_mode);
        status = false;
    }

    // Replace input or null packets.
    if (pid == PID_NULL && !_late_packets.empty() && (pcr_known || !need_pts)) {

        // Report end of packet delaying in PES synchronous mode.
        if (_delayed_initial > 0) {
            _report.verbose(u"stop %s packets in PID %n, can compute PTS now, %d packets were %s",
                            _drop_before_pts ? u"dropping" : u"delaying", _output_pid, _delayed_initial,
                            _drop_before_pts ? u"dropped" : u"delayed");
            _delayed_initial = 0;
        }

        // Do we need to add a PCR in this packet?
        const bool add_pcr = _insert_pcr && pcr_known;

        // How many bytes do we have in the queue (at least).
        const size_t add_bytes = (PKT_SIZE - _late_index) + (_late_packets.size() > 1 ? PKT_SIZE : 0);

        // Available size in outer packet:
        //   PKT_SIZE
        //   -4 => TS header.
        //   -8 => Adaptation field in case of PCR: 1-byte AF size, 1-byte flags, 6-byte PCR.
        //   -1 => Pointer field, first byte of payload (not always, but very often).
        // -26|27 => PES size (when PES mode is enabled).
        //  -10 => PTS (5) + Metadata AU Header (5), in Synchronous PES mode (when PES mode is enabled).

        // Depending on packing option, we may decide to not insert an outer packet which is not full.
        // We insert a packet all the time if packing is off.
        // Otherwise, we insert a packet when there is enough data to fill it.
        bool send_packet = !_packing;
        if (_packing && _pack_distance > 0 && _late_distance > _pack_distance) {
            send_packet = true;
        }
        if (send_packet || add_bytes >= PKT_SIZE - (add_pcr ? 12 : 4) - 1) {

            // Build the new packet.
            pkt.init(_output_pid, _cc_output);

            // Continuity counter of next output packet.
            _cc_output = (_cc_output + 1) & CC_MASK;

            // Insert a PCR if requested.
            if (add_pcr) {

                // Set the PCR in the adaptation field.
                pkt.setPCR(_pcr_last_value + getPCRDistance(), true);

                // Don't insert another PCR in output PID until a PCR is found in reference PID.
                _insert_pcr = false;
            }

            const size_t pes_sync = _pes_offset == 0 ? 0 : 10;
            // Increase the header size to the limit of the selected PES mode:
            //  PES mode 1 (ASYNC): 127+9+16+1    = 153 max payload
            //  PES mode 1 (SYNC) : 127+9+16+1+10 = 163 max payload
            //  PES mode 2 (any)  : no limit
            if (_pes_mode == FIXED && pkt.getPayloadSize() > (153 + pes_sync)) {
                pkt.setPayloadSize(153 + pes_sync);
            }

            // How many bytes consumes de PES encapsulation, based on this calculus:
            //  ASYNC mode:
            //   26|27 bytes =
            //          9 bytes PES header;
            //       + 16 bytes KLVA UL key;
            //       +  1 byte with BER short mode | 2 bytes with BER long mode.
            //  SYNC mode:
            //   36|37 bytes =
            //         14 bytes PES header;
            //       +  5 bytes Metadata AU Header;
            //       + 16 bytes KLVA UL key;
            //       +  1 byte with BER short mode | 2 bytes with BER long mode.
            //  and 0 when PES mode is OFF.
            const uint8_t pes_header = _pes_mode != DISABLED ?
                uint8_t(add_bytes <= 127 || pkt.getPayloadSize() <= (153+pes_sync) ? (26+pes_sync) : (27+pes_sync)) :
                0;

            // If there are less "late" bytes than the output payload size, enlarge the adaptation field
            // with stuffing. Note that if there is so few bytes in the only "late" packet, this cannot
            // be the beginning of a packet and there will be no pointer field.
            if (_late_packets.size() == 1 && _late_index > (pes_header + pkt.getHeaderSize())) {
                pkt.setPayloadSize(PKT_SIZE - _late_index + pes_header);
            }

            // Index in pkt where we write data. Start at beginning of payload.
            size_t pkt_index = pkt.getHeaderSize();

            // When PES mode is ON add here the envelope before the data/payload
            uint8_t pes_pointer = 0; // Indirect reference for futher completion
            if (pes_header > 0) {

                // Fill the PES Header

                pkt.b[pkt_index++] = 0x00; // PES start code prefix
                pkt.b[pkt_index++] = 0x00;
                pkt.b[pkt_index++] = 0x01;

                if (pes_sync == 0) {
                    pkt.b[pkt_index++] = 0xBD; // Stream_id == Pivate_Stream_1
                }
                else {
                    pkt.b[pkt_index++] = 0xFC; // Stream_id == Metadata_Stream
                }

                pkt.b[pkt_index++] = 0x00; // PES packet length (2 bytes)
                pkt.b[pkt_index++] = 0x00; // Pending to complete at end (**)

                pes_pointer = uint8_t(pkt_index);   // Store for reference

                if (pes_sync == 0) {
                    pkt.b[pkt_index++] = 0x84; // Header flags (1)
                    pkt.b[pkt_index++] = 0x00; // Header flags (2)
                    pkt.b[pkt_index++] = 0x00; // Length of remaining optional fields
                }
                else {
                    pkt.b[pkt_index++] = 0x80; // Header flags (1)
                    pkt.b[pkt_index++] = 0x80; // Header flags (2)
                    pkt.b[pkt_index++] = 0x05; // Length of remaining optional fields (PTS:5 Bytes)

                    // Write PTS
                    pkt.b[pkt_index++] = 0x21;
                    pkt.b[pkt_index++] = 0x00;
                    pkt.b[pkt_index++] = 0x01;
                    pkt.b[pkt_index++] = 0x00;
                    pkt.b[pkt_index++] = 0x01;
                    // This is an empty PTS (00:00:00.0000), it will be rewritten at end (**)

                    // Write the Metadata AU Header (5 Bytes).
                    pkt.b[pkt_index++] = 0x00; // Service_id
                    pkt.b[pkt_index++] = _cc_pes++; // Sequence_number (0x00-0xFF)
                    pkt.b[pkt_index++] = 0xDF; // Flags
                    pkt.b[pkt_index++] = 0x00; // AU Cell data length (2 bytes)
                    pkt.b[pkt_index++] = 0x00; // Pending to complete at end (**)
                }

                // Fill the KLVA Data

                // >>> (K)ey
                // UL Used: 060E2B34.01010101.0F010800.0F0F0F0F
                // It's an Unique ID value in the testing range.
                pkt.b[pkt_index++] = 0x06;
                pkt.b[pkt_index++] = 0x0e;
                pkt.b[pkt_index++] = 0x2b;
                pkt.b[pkt_index++] = 0x34;

                pkt.b[pkt_index++] = 0x01;
                pkt.b[pkt_index++] = 0x01;
                pkt.b[pkt_index++] = 0x01;
                pkt.b[pkt_index++] = 0x01;

                pkt.b[pkt_index++] = 0x0f;
                pkt.b[pkt_index++] = 0x01;
                pkt.b[pkt_index++] = 0x08;
                pkt.b[pkt_index++] = 0x00;

                pkt.b[pkt_index++] = 0x0f;
                pkt.b[pkt_index++] = 0x0f;
                pkt.b[pkt_index++] = 0x0f;
                pkt.b[pkt_index++] = 0x0f; // 0x1f when equivalent PUSI flag is set

                // >>> (L)ength
                size_t payload_size = PKT_SIZE - pkt_index - 1;
                assert(payload_size > 0);
                if (payload_size > 127) {
                    pkt.b[pkt_index++] = 0x81; // Long flag with size length = 1
                    payload_size--;
                }
                pkt.b[pkt_index++] = uint8_t(payload_size); // Final size of data/payload

                // In PES mode any packet is an unique PES packet, so set the Payload Unit Start
                pkt.setPUSI();
                // When using the PES encapsulation the PUSI flag is mapped
                // at bit 0x10 in the last byte of the UL Key.

                // Update the remaining values (**)
                if (pes_sync == 0) {
                    pkt.b[pes_pointer-1] = uint8_t(PKT_SIZE - pes_pointer); // PES packet length
                }
                else {
                    pkt.b[pes_pointer-1] = uint8_t(PKT_SIZE - pes_pointer);         // PES packet length
                    pkt.b[pes_pointer-1+13] = uint8_t(PKT_SIZE - pes_pointer - 13); // AU cell data length

                    // Calculate the PTS based on PCR + Offset (positive/negative) without wrapping up.
                    uint64_t pts = (_pcr_last_value + getPCRDistance()) / SYSTEM_CLOCK_SUBFACTOR;
                    if (pts != 0 && _pcr_last_value != INVALID_PCR && _pcr_last_value != 0 && getPCRDistance() != 0 && (_pes_offset + pts) > 0) {
                        pts += _pes_offset;
                    }
                    else {
                        pts = _pts_previous;
                    }
                    // Ensure monotonic increments
                    if (pts <= _pts_previous) {
                        pts++;
                    }
                    pts = pts & PTS_DTS_MASK;
                    pkt.setPTS(pts);
                    _pts_previous = pts;
                }

                // >>> (V)alue
                // At this point the PES packet is fully filled and only remains the payload
                assert(pkt_index < PKT_SIZE);
            }

            // Insert PUSI and pointer field when necessary.
            if (_late_index == 1) {
                // We immediately start with the start of a packet.
                // Note: The flag is different in the PES mode!
                if (_pes_mode != DISABLED) {
                    pkt.b[pes_pointer+18+pes_sync] |= 0x10;
                }
                else {
                    pkt.setPUSI();
                }
                pkt.b[pkt_index++] = 0; // pointer field
            }
            else if (_late_index > pkt_index + 1 && _late_packets.size() > 1) {
                // The remaining bytes in the first packet are less than the output payload,
                // we will start a new packet in this payload.
                if (_pes_mode != DISABLED) {
                    pkt.b[pes_pointer+18+pes_sync] |= 0x10;
                }
                else {
                    pkt.setPUSI();
                }
                pkt.b[pkt_index++] = uint8_t(PKT_SIZE - _late_index); // pointer field
            }

            // Copy first part of output payload from the first queued packet.
            fillPacket(pkt, pkt_index);

            // Then copy remaining from next packet.
            if (pkt_index < PKT_SIZE) {
                fillPacket(pkt, pkt_index);
            }

            // The output packet shall be exactly full.
            assert(pkt_index == PKT_SIZE);
        }
    }

    // Count packets before returning.
    _current_packet++;
    return status;
}


//----------------------------------------------------------------------------
// Fill packet payload with data from the first queued packet.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::fillPacket(ts::TSPacket& pkt, size_t& pkt_index)
{
    assert(!_late_packets.empty());
    assert(_late_packets.front() != nullptr);
    assert(_late_index < PKT_SIZE);
    assert(pkt_index < PKT_SIZE);

    // Copy part of output payload from the first queued packet.
    const size_t size = std::min(PKT_SIZE - pkt_index, PKT_SIZE - _late_index);
    MemCopy(pkt.b + pkt_index, _late_packets.front()->b + _late_index, size);
    pkt_index += size;
    _late_index += size;

    // If the first queued packet if fully encapsulated, remove it.
    if (_late_index >= PKT_SIZE) {
        _late_packets.pop_front();
        _late_index = 1;  // skip 0x47 in next packet
    }
}
