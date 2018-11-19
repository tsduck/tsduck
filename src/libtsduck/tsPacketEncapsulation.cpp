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

#include "tsPacketEncapsulation.h"
#include "tsMemoryUtils.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::PacketEncapsulation::DEFAULT_MAX_BUFFERED_PACKETS;
#endif


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::PacketEncapsulation::PacketEncapsulation(PID pidOutput, const PIDSet& pidInput, PID pcrReference) :
    _packing(false),
    _pesmode(0),
    _pidOutput(pidOutput),
    _pidInput(pidInput),
    _pcrReference(pcrReference),
    _lastError(),
    _currentPacket(0),
    _pcrLastPacket(INVALID_PACKET_COUNTER),
    _pcrLastValue(INVALID_PCR),
    _bitrate(0),
    _insertPCR(false),
    _ccOutput(0),
    _lastCC(),
    _lateMaxPackets(DEFAULT_MAX_BUFFERED_PACKETS),
    _lateIndex(0),
    _latePackets()
{
}


//----------------------------------------------------------------------------
// Reset the encapsulation.
//----------------------------------------------------------------------------

void ts::PacketEncapsulation::reset(PID pidOutput, const PIDSet& pidInput, PID pcrReference)
{
    _packing = false;
    _pesmode = 0;
    _pidOutput = pidOutput;
    _pidInput = pidInput;
    _pcrReference = pcrReference;
    _lastError.clear();
    _currentPacket = 0;
    _ccOutput = 0;
    _lastCC.clear();
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
        _lastCC.clear();
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

    // We need to guarantee the limits of all packages.
    // When the buffer is empty we alredy set the late pointer to the first byte after 0x47.
    if (_lateIndex < 1) _lateIndex = 1;

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
    if (_pesmode > 2) {
        _lastError.assign(u"PES mode %u not implemented!", _pesmode);
        status = false;
    }

    // Replace input or null packets.
    if (pid == PID_NULL && !_latePackets.empty()) {

        // Do we need to add a PCR in this packet?
        const bool addPCR = _insertPCR && _bitrate != 0 && _pcrLastPacket != INVALID_PACKET_COUNTER && _pcrLastValue != INVALID_PCR;

        // How many bytes do we have in the queue (at least).
        const size_t addBytes = (PKT_SIZE - _lateIndex) + (_latePackets.size() > 1 ? PKT_SIZE : 0);

        // How many bytes consumes de PES encapsulation, based on this calculus:
        //   26|27 bytes =
        //               9 bytes PES header;
        //            + 16 bytes KLVA UL key;
        //            + 1 byte with BER short mode | 2 bytes with BER long mode.
        const uint8_t pes_header = _pesmode > 0 ? (addBytes <= 127 || _pesmode == 1 ? 26 : 27) : 0;

        // Depending on packing option, we may decide to not insert an outer packet which is not full.
        // Available size in outer packet:
        //   PKT_SIZE
        //   -4 => TS header.
        //   -8 => Adaptation field in case of PCR: 1-byte AF size, 1-byte flags, 6-byte PCR.
        //   -1 => Pointer field, first byte of payload (not always, but very often).
        // -26|27 => PES size (when PES mode is enabled)
        // We insert a packet all the time if packing is off.
        // Otherwise, we insert a packet when there is enough data to fill it.
        if (!_packing || addBytes >= PKT_SIZE - (addPCR ? 12 : 4) - 1) {

            // Build the new packet header.
            pkt.b[0] = SYNC_BYTE;
            pkt.b[1] = 0;
            pkt.b[2] = 0;
            pkt.b[3] = 0x10; // no adaptation_field, payload only
            pkt.setPID(_pidOutput);
            pkt.setCC(_ccOutput);
            ::memset(pkt.b + 4, 0xFF, PKT_SIZE - 4);
            pkt.b[4] = 0; // We temporally set Adaptation Field Length to 0
                          // This position will be later overwritten:
                          // or with data, or with a real Length value.
                          // But setting it to zero simplifies the code.

            // Index in pkt where we write data. Start at beginning of payload.
            size_t pktIndex = 4;

            // Continuity counter of next output packet.
            _ccOutput = (_ccOutput + 1) & CC_MASK;

            // Insert a PCR if requested.
            if (addPCR) {

                // Compute the PCR of this packet.
                const uint64_t pcrDistance = (PacketInterval(_bitrate, _currentPacket - _pcrLastPacket) * SYSTEM_CLOCK_FREQ) / MilliSecPerSec;
                const uint64_t pcr = (_pcrLastValue + pcrDistance) & PCR_MASK;

                // We need to add an adaptation field in the TS packet.
                pkt.b[3] |= 0x20; // adaptation field present
                pkt.b[4] = 7;     // adaptation field size (1 byte for flags, 6 bytes for PCR)
                pkt.b[5] = 0x10;  // PCR_flag
                pktIndex += 8;    // including 1-byte AF size and 7-byte AF

                // Set the PCR in the adaptation field.
                pkt.setPCR(pcr);
                assert(pkt.getPCR() == pcr); // make sure the AF was properly built

                // Don't insert another PCR in output PID until a PCR is found in reference PID.
                _insertPCR = false;
            }

            // Calculate PES maximum envelope size
            // (0 when disabled)
            const uint8_t pes_maxsize = _pesmode > 0 ? (_pesmode == 1 ? 127 : (PKT_SIZE - (uint8_t(pkt.b[4]) + 4) - pes_header)) : 0;
            // The upper limit of 127 is an artificial value; and it's not
            // related with the payload max size of 127 bytes of the KLV packet.
            // The aim is to provide enough free adaptation space in the header.

            // Calculate PES minimum padding stuff required in the Adaptation field
            // (0 when disabled) (equals to 0 when it's not needed)
            const uint8_t pes_stuff   = _pesmode > 0 ? ((PKT_SIZE - (uint8_t(pkt.b[4]) + 4) - pes_header) - pes_maxsize) : 0;

            // If there are less "late" bytes than the output payload size, enlarge the adaptation field
            // with stuffing. Note that if there is so few bytes in the only "late" packet, this cannot
            // be the beginning of a packet and there will be no pointer field.
            if (_latePackets.size() == 1 && _lateIndex > pktIndex + pes_header + pes_stuff) {
                // This code works identically whether there was an adaptation field or not.
                pkt.b[3] |= 0x20;  // adaptation field present
                pkt.b[4] = uint8_t(_lateIndex - 5 - pes_header);  // adaptation field size
                if (!addPCR) {
                    pkt.b[5] = 0x00; // AF flags
                }
                pktIndex = _lateIndex - pes_header;
            }
            else if (pes_stuff > 0) { // Add the alone padding only for PES.
                pkt.b[3] |= 0x20;
                pkt.b[4] = uint8_t(uint8_t(pkt.b[4]) + pes_stuff - 1);
                if (!addPCR) {
                    pkt.b[5] = 0x00;
                }
                pktIndex += pes_stuff;
            }

            // At this point, pktIndex points at the beginning of the TS payload.
            assert(pktIndex == pkt.getHeaderSize());

            // When PES mode is ON add here the envelope before the data/payload
            uint8_t pes_pointer = 0; // Indirect reference for futher completion
            if (pes_header > 0) {

                // Fill the PES Header

                pkt.b[pktIndex++] = 0x00; // PES start code prefix
                pkt.b[pktIndex++] = 0x00;
                pkt.b[pktIndex++] = 0x01;

                pkt.b[pktIndex++] = 0xBD; // Stream_id == Pivate_Stream_1

                pkt.b[pktIndex++] = 0x00; // PES packet length (2 bytes)
                pkt.b[pktIndex++] = 0x00; // Pending to complete at end (**)

                pes_pointer = pktIndex;   // Store for reference

                pkt.b[pktIndex++] = 0x84; // Header flags
                pkt.b[pktIndex++] = 0x00; // Header flags
                pkt.b[pktIndex++] = 0x00; // Length of remaining optional fields

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
                uint8_t payload_size = PKT_SIZE - pktIndex - 1;
                assert(payload_size > 0);
                if (payload_size > 127) {
                    pkt.b[pktIndex++] = 0x81; // Long flag with size length = 1
                    payload_size--;
                }
                pkt.b[pktIndex++] = payload_size; // Final size of data/payload

                // Update the remaining value of the PES packet length (**)
                pkt.b[pes_pointer-1] = uint8_t(PKT_SIZE - pes_pointer);

                // >>> (V)alue
                // At this point the PES packet is fully filled and only remains the payload
                assert(pktIndex < PKT_SIZE);

                pkt.setPUSI(); // In PES mode any packet is an unique PES mode, so set the Payload Unit Start
                // The PES encapsulation maps the PUSI flag at bit 0x10 in the last byte of the UL Key.
            }

            // Insert PUSI and pointer field when necessary.
            if (_lateIndex == 1) {
                // We immediately start with the start of a packet.
                // Note: The flag is different in the PES mode!
                if (_pesmode > 0)
                    pkt.b[pes_pointer+18] |= 0x10;
                else
                    pkt.setPUSI();
                pkt.b[pktIndex++] = 0; // pointer field
            }
            else if (_lateIndex > pktIndex + 1 && _latePackets.size() > 1) {
                // The remaining bytes in the first packet are less than the output payload,
                // we will start a new packet in this payload.
                if (_pesmode > 0)
                    pkt.b[pes_pointer+18] |= 0x10;
                else
                    pkt.setPUSI();
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
    ::memcpy(pkt.b + pktIndex, _latePackets.front()->b + _lateIndex, size);
    pktIndex += size;
    _lateIndex += size;

    // If the first queued packet if fully encapsulated, remove it.
    if (_lateIndex >= PKT_SIZE) {
        _latePackets.pop_front();
        _lateIndex = 1;  // skip 0x47 in next packet
    }
}
