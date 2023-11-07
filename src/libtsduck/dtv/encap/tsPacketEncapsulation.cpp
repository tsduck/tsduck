//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard (PES mode by lars18th)
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPacketEncapsulation.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::PacketEncapsulation::PacketEncapsulation(PID pidOutput, const PIDSet& pidInput, PID pcrReference) :
    _pidOutput(pidOutput),
    _pidInput(pidInput),
    _pcrReference(pcrReference)
{
}


//----------------------------------------------------------------------------
// Reset the encapsulation.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::reset(PID pidOutput, const PIDSet& pidInput, PID pcrReference)
{
    _packing = false;
    _packDistance = NPOS;
    _pesMode = DISABLED;
    _pesOffset = 0;
    _pidOutput = pidOutput;
    _pidInput = pidInput;
    _pcrReference = pcrReference;
    _lastError.clear();
    _currentPacket = 0;
    _ccOutput = 0;
    _ccPES = 1;
    _lastCC.clear();
    _lateDistance = 0;
    _lateIndex = 0;
    _latePackets.clear();
    resetPCR();
}


//----------------------------------------------------------------------------
// Change the output PID.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::setOutputPID(PID pid)
{
    if (pid != _pidOutput) {
        _pidOutput = pid;
        // Reset encapsulation.
        _ccOutput = 0;
        _ccPES = 1;
        _lastCC.clear();
        _lateDistance = 0;
        _lateIndex = 0;
        _latePackets.clear();
    }
}


//----------------------------------------------------------------------------
// Set the maximum number of buffered packets.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::setMaxBufferedPackets(size_t count)
{
    // Always keep some margin.
    _lateMaxPackets = std::max<size_t>(8, count);
}


//----------------------------------------------------------------------------
// Change the reference PID for PCR's.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::setReferencePCR(PID pid)
{
    if (pid != _pcrReference) {
        // Reference PID modified, reset synchro.
        _pcrReference = pid;
        resetPCR();
    }
}


//----------------------------------------------------------------------------
// Reset PCR information, lost synchronization.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::resetPCR()
{
    _pcrLastPacket = INVALID_PACKET_COUNTER;
    _pcrLastValue = INVALID_PCR;
    _bitrate = 0;
    _insertPCR = false;
}


//----------------------------------------------------------------------------
// Replace the set of input PID's. The null PID can never be encapsulated.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::setInputPIDs(const PIDSet& pidInput)
{
    _pidInput = pidInput;
    _pidInput.reset(PID_NULL);
}

void ts::PacketEncapsulation::addInputPID(PID pid)
{
    if (pid < PID_NULL) {
        _pidInput.set(pid);
    }
}

void ts::PacketEncapsulation::removeInputPID(PID pid)
{
    if (pid < PID_NULL) {
        _pidInput.reset(pid);
    }
}


//----------------------------------------------------------------------------
// Process a TS packet from the input stream.
//----------------------------------------------------------------------------

bool ts::PacketEncapsulation::processPacket(TSPacket& pkt)
{
    PID pid = pkt.getPID();
    bool status = true;  // final return value.

    // Keep track of continuity counter per PID, detect discontinuity.
    // Do not check discontinuity on the stuffing PID, there is none.
    if (pid != PID_NULL) {
        const auto icc = _lastCC.find(pid);
        const uint8_t cc = pkt.getCC();
        if (icc == _lastCC.end()) {
            // No CC found so far on this PID. Just keep this one.
            _lastCC.insert(std::make_pair(pid, cc));
        }
        else {
            if (cc != ((icc->second + 1) & CC_MASK)) {
                // Discontinuity detected, forget information about PCR, they will be incorrect.
                resetPCR();
            }
            icc->second = cc;
        }
    }

    // Collect PCR from the reference PID to compute bitrate.
    if (_pcrReference != PID_NULL && pid == _pcrReference && pkt.hasPCR()) {
        const uint64_t pcr = pkt.getPCR();
        // If previous PCR is known, compute bitrate. Ignore PCR value wrap-up.
        if (_pcrLastValue != INVALID_PCR && _pcrLastValue < pcr) {
            assert(_pcrLastPacket < _currentPacket);
            // Duration in milliseconds since last PCR.
            const MilliSecond ms = ((pcr - _pcrLastValue) * MilliSecPerSec) / SYSTEM_CLOCK_FREQ;
            // Compute TS bitrate since last PCR.
            _bitrate = PacketBitRate(_currentPacket - _pcrLastPacket, ms);
            // Insert PCR in output PID asap after a PCR on reference PID when the bitrate is known.
            _insertPCR = true;
        }
        // Save current PCR.
        _pcrLastPacket = _currentPacket;
        _pcrLastValue = pcr;
    }

    // Detect PID conflicts (when the output PID is present on input but not encapsulated).
    if (pid == _pidOutput && !_pidInput.test(pid)) {
        _lastError.format(u"PID conflict, output PID 0x%X (%d) is present but not encapsulated", {pid, pid});
        status = false;
    }

    // Increase the counter of the distance with each incoming packet.
    _lateDistance++;

    // We need to guarantee the limits of all packages.
    // When the buffer is empty we alredy set the late pointer to the first byte after 0x47.
    if (_lateIndex < 1) {
        _lateIndex = 1;
    }

    // If this packet is part of the input set, place it in the "late" queue.
    // Note that a packet always need to go into the queue, even if the queue
    // is empty because no input packet can fit into an output packet. At least
    // a few bytes need to be queued.
    if (_pidInput.test(pid) && _pidOutput != PID_NULL) {
        if (_latePackets.size() > _lateMaxPackets) {
            _lastError.assign(u"buffered packets overflow, insufficient null packets in input stream");
            status = false;
        }
        else {
            // Enqueue the packet.
            _latePackets.push_back(new TSPacket(pkt));
            // If this is the first packet in the queue, point to the first byte after 0x47.
            if (_latePackets.size() == 1) {
                _lateIndex = 1;
            }
        }
        // Pretend that the input packet is a null one.
        pid = PID_NULL;
    }

    // Check PES mode support
    if (_pesMode > VARIABLE) {
        _lastError.assign(u"PES mode %d not implemented!", _pesMode);
        status = false;
    }

    // Replace input or null packets.
    if (pid == PID_NULL && !_latePackets.empty()) {

        // Do we need to add a PCR in this packet?
        const bool addPCR = _insertPCR && _bitrate != 0 && _pcrLastPacket != INVALID_PACKET_COUNTER && _pcrLastValue != INVALID_PCR;

        // How many bytes do we have in the queue (at least).
        const size_t addBytes = (PKT_SIZE - _lateIndex) + (_latePackets.size() > 1 ? PKT_SIZE : 0);

        // Depending on packing option, we may decide to not insert an outer packet which is not full.
        // Available size in outer packet:
        //   PKT_SIZE
        //   -4 => TS header.
        //   -8 => Adaptation field in case of PCR: 1-byte AF size, 1-byte flags, 6-byte PCR.
        //   -1 => Pointer field, first byte of payload (not always, but very often).
        // -26|27 => PES size (when PES mode is enabled).
        //  -10 => PTS (5) + Metadata AU Header (5), in Synchronous PES mode (when PES mode is enabled).
        // We insert a packet all the time if packing is off.
        // Otherwise, we insert a packet when there is enough data to fill it.
        bool packout = !_packing;
        if (_packing && _packDistance > 0 && _lateDistance > _packDistance) {
            packout = true;
        }
        if (packout || addBytes >= PKT_SIZE - (addPCR ? 12 : 4) - 1) {

            // Build the new packet.
            pkt.init(_pidOutput, _ccOutput);

            // Continuity counter of next output packet.
            _ccOutput = (_ccOutput + 1) & CC_MASK;

            // Insert a PCR if requested.
            if (addPCR) {

                // Set the PCR in the adaptation field.
                pkt.setPCR(_pcrLastValue + getPCRDistance(), true);

                // Don't insert another PCR in output PID until a PCR is found in reference PID.
                _insertPCR = false;
            }

            const size_t pes_sync = _pesOffset == 0 ? 0 : 10;
            // Increase the header size to the limit of the selected PES mode:
            //  PES mode 1 (ASYNC): 127+9+16+1    = 153 max payload
            //  PES mode 1 (SYNC) : 127+9+16+1+10 = 163 max payload
            //  PES mode 2 (any)  : no limit
            if (_pesMode == FIXED && pkt.getPayloadSize() > (153 + pes_sync)) {
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
            const uint8_t pes_header = _pesMode != DISABLED ?
                uint8_t(addBytes <= 127 || pkt.getPayloadSize() <= (153+pes_sync) ? (26+pes_sync) : (27+pes_sync)) :
                0;

            // If there are less "late" bytes than the output payload size, enlarge the adaptation field
            // with stuffing. Note that if there is so few bytes in the only "late" packet, this cannot
            // be the beginning of a packet and there will be no pointer field.
            if (_latePackets.size() == 1 && _lateIndex > (pes_header + pkt.getHeaderSize())) {
                pkt.setPayloadSize(PKT_SIZE - _lateIndex + pes_header);
            }

            // Index in pkt where we write data. Start at beginning of payload.
            size_t pktIndex = pkt.getHeaderSize();

            // When PES mode is ON add here the envelope before the data/payload
            uint8_t pes_pointer = 0; // Indirect reference for futher completion
            if (pes_header > 0) {

                // Fill the PES Header

                pkt.b[pktIndex++] = 0x00; // PES start code prefix
                pkt.b[pktIndex++] = 0x00;
                pkt.b[pktIndex++] = 0x01;

                if (pes_sync == 0) {
                    pkt.b[pktIndex++] = 0xBD; // Stream_id == Pivate_Stream_1
                }
                else {
                    pkt.b[pktIndex++] = 0xFC; // Stream_id == Metadata_Stream
                }

                pkt.b[pktIndex++] = 0x00; // PES packet length (2 bytes)
                pkt.b[pktIndex++] = 0x00; // Pending to complete at end (**)

                pes_pointer = uint8_t(pktIndex);   // Store for reference

                if (pes_sync == 0) {
                    pkt.b[pktIndex++] = 0x84; // Header flags (1)
                    pkt.b[pktIndex++] = 0x00; // Header flags (2)
                    pkt.b[pktIndex++] = 0x00; // Length of remaining optional fields
                }
                else {
                    pkt.b[pktIndex++] = 0x80; // Header flags (1)
                    pkt.b[pktIndex++] = 0x80; // Header flags (2)
                    pkt.b[pktIndex++] = 0x05; // Length of remaining optional fields (PTS:5 Bytes)

                    // Write PTS
                    pkt.b[pktIndex++] = 0x21;
                    pkt.b[pktIndex++] = 0x00;
                    pkt.b[pktIndex++] = 0x01;
                    pkt.b[pktIndex++] = 0x00;
                    pkt.b[pktIndex++] = 0x01;
                    // This is an empty PTS (00:00:00.0000), it will be rewrited at end (**)

                    // Write the Metadata AU Header (5 Bytes).
                    pkt.b[pktIndex++] = 0x00; // Service_id
                    pkt.b[pktIndex++] = _ccPES++; // Sequence_number (0x00-0xFF)
                    pkt.b[pktIndex++] = 0xDF; // Flags
                    pkt.b[pktIndex++] = 0x00; // AU Cell data length (2 bytes)
                    pkt.b[pktIndex++] = 0x00; // Pending to complete at end (**)
                }

                // Fill the KLVA Data

                // >>> (K)ey
                // UL Used: 060E2B34.01010101.0F010800.0F0F0F0F
                // It's an Unique ID value in the testing range.
                pkt.b[pktIndex++] = 0x06;
                pkt.b[pktIndex++] = 0x0e;
                pkt.b[pktIndex++] = 0x2b;
                pkt.b[pktIndex++] = 0x34;

                pkt.b[pktIndex++] = 0x01;
                pkt.b[pktIndex++] = 0x01;
                pkt.b[pktIndex++] = 0x01;
                pkt.b[pktIndex++] = 0x01;

                pkt.b[pktIndex++] = 0x0f;
                pkt.b[pktIndex++] = 0x01;
                pkt.b[pktIndex++] = 0x08;
                pkt.b[pktIndex++] = 0x00;

                pkt.b[pktIndex++] = 0x0f;
                pkt.b[pktIndex++] = 0x0f;
                pkt.b[pktIndex++] = 0x0f;
                pkt.b[pktIndex++] = 0x0f; // 0x1f when equivalent PUSI flag is set

                // >>> (L)ength
                size_t payload_size = PKT_SIZE - pktIndex - 1;
                assert(payload_size > 0);
                if (payload_size > 127) {
                    pkt.b[pktIndex++] = 0x81; // Long flag with size length = 1
                    payload_size--;
                }
                pkt.b[pktIndex++] = uint8_t(payload_size); // Final size of data/payload

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
                    uint64_t pts = (_pcrLastValue + getPCRDistance()) / 300;
                    if (pts != 0 && _pcrLastValue != INVALID_PCR && _pcrLastValue != 0 &&
                         getPCRDistance() != 0 && (_pesOffset + pts) > 0) {
                        pts += _pesOffset;
                    }
                    else {
                        pts = _ptsPrevious;
                    }
                    // Ensure monotonic increments
                    if (pts <= _ptsPrevious) {
                        pts++;
                    }
                    pts = pts & PTS_DTS_MASK;
                    pkt.setPTS(pts);
                    _ptsPrevious = pts;
                }

                // >>> (V)alue
                // At this point the PES packet is fully filled and only remains the payload
                assert(pktIndex < PKT_SIZE);
            }

            // Insert PUSI and pointer field when necessary.
            if (_lateIndex == 1) {
                // We immediately start with the start of a packet.
                // Note: The flag is different in the PES mode!
                if (_pesMode != DISABLED) {
                    pkt.b[pes_pointer+18+pes_sync] |= 0x10;
                }
                else {
                    pkt.setPUSI();
                }
                pkt.b[pktIndex++] = 0; // pointer field
            }
            else if (_lateIndex > pktIndex + 1 && _latePackets.size() > 1) {
                // The remaining bytes in the first packet are less than the output payload,
                // we will start a new packet in this payload.
                if (_pesMode != DISABLED) {
                    pkt.b[pes_pointer+18+pes_sync] |= 0x10;
                }
                else {
                    pkt.setPUSI();
                }
                pkt.b[pktIndex++] = uint8_t(PKT_SIZE - _lateIndex); // pointer field
            }

            // Copy first part of output payload from the first queued packet.
            fillPacket(pkt, pktIndex);

            // Then copy remaining from next packet.
            if (pktIndex < PKT_SIZE) {
                fillPacket(pkt, pktIndex);
            }

            // The output packet shall be exactly full.
            assert(pktIndex == PKT_SIZE);
        }
    }

    // Count packets before returning.
    _currentPacket++;
    return status;
}


//----------------------------------------------------------------------------
// Fill packet payload with data from the first queued packet.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::fillPacket(ts::TSPacket& pkt, size_t& pktIndex)
{
    assert(!_latePackets.empty());
    assert(!_latePackets.front().isNull());
    assert(_lateIndex < PKT_SIZE);
    assert(pktIndex < PKT_SIZE);

    // Copy part of output payload from the first queued packet.
    const size_t size = std::min(PKT_SIZE - pktIndex, PKT_SIZE - _lateIndex);
    std::memcpy(pkt.b + pktIndex, _latePackets.front()->b + _lateIndex, size);
    pktIndex += size;
    _lateIndex += size;

    // If the first queued packet if fully encapsulated, remove it.
    if (_lateIndex >= PKT_SIZE) {
        _latePackets.pop_front();
        _lateIndex = 1;  // skip 0x47 in next packet
    }
}
