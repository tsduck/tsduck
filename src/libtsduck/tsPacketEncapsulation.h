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
//!
//!  @file
//!  An efficient TSDuck-specific TS packets encapsulation in a PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsTSPacket.h"
#include "tsSafePtr.h"

namespace ts {
    //!
    //! An efficient TSDuck-specific TS packets encapsulation in a PID.
    //! @ingroup mpeg
    //!
    //! An instance of this class encapsulates several PID's from the
    //! input transport stream into one single output PID. Functionally,
    //! this is a subset of the features of T2-MI but much more lightweight
    //! and significantly faster to process.
    //!
    //! Encapsulation format (plain)
    //! ----------------------------
    //! In the output elementary stream (ES), all input TS packets are
    //! contiguous, without encapsulation. The initial 0x47 synchronization
    //! byte is removed. Only the remaining 187 bytes are encapsulated.
    //!
    //! In the output PID, the packetization is similar to sections, with
    //! 187-bytes packets instead of sections. The Payload Unit Start
    //! Indicator (PUSI) bit is set in the header of TS packets containing
    //! the start of an encapsulated packet. When the PUSI bit is set, the
    //! first byte of the payload is a "pointer field" to the beginning of
    //! the first encapsulated packet.
    //!
    //! Due to the overhead of the TS header, the number of output packets
    //! is slightly larger than the input packets. The input streams must
    //! contain a few null packets to absorb the extra output packets. For
    //! this reason, null packets (PID 0x1FFF) are never encapsulated.
    //!
    //! Encapsulation format (PES)
    //! --------------------------
    //! When selecting the PES encapsulation the same plain elementary
    //! stream is used, but with a PES envelope. This reduces the payload
    //! size, but makes the outer encapsulation more transparent. The full
    //! overhead is around 14% of more data.
    //!
    //! The PES envelope uses a KLVA SMPTE-336M encapsulation to insert the
    //! inner payload into one private (testing) key. Each TS packet contains
    //! only one key, with a size no larger than the payload of one TS packet.
    //! So each PES packet fits into a single TS packet.
    //!
    //! The SMPTE-336M encapsulation is the asynchrouns one. So no PTS
    //! marks are used, and the payload size is larger.
    //!
    //! Two variant strategies are implemented. The FIXED mode uses the
    //! short (7bit) BER encoding. This limits the PES payload to a maximum
    //! of 127 bytes. And the adaptation field of the outer packet is
    //! enlarged with some stuff. However, the advantage is that the PES
    //! is sufficient small to include more data in the outer TS packet.
    //! This reduces the possibility than some external processing will
    //! split the outer packet in two to accomodate the entire PES data.
    //!
    //! The VARIABLE mode does not imposses this restriction, and outer
    //! packets are filled as maximum. The drawback is that some times
    //! the long form of BER encoding is used with two bytes and others
    //! the short form with one byte. Futhermore, this increases the chances
    //! that some external procesing ocuppies two outer packets for the
    //! same inner PES packet. Still, support for those split PES packets
    //! is included. The only requirement is that the 26|27 PES+KLVA header
    //! is inserted in the first packet (with PUSI on). The remaining
    //! payload can be distributed in thge following TS packets.
    //!
    //! The PES envelope has an overhead of 26 or 27 bytes based on:
    //!   9 bytes for the PES header.
    //!  16 bytes for the UL key
    //! 1|2 bytes for the payload size (BER short or long format)
    //!
    //! In order to correctly identify the encapsulated PES stream is
    //! recommended to include in the PMT table a format identifier
    //! descriptor for "KLVA" (0x4B4C5641); and use the Private Type (0x06)
    //! for the stream type.
    //!  Example:
    //!   "tsp ... \                                                     "
    //!   " -P encap -o 7777 --pes-mode ... \                            "
    //!   " -P pmt -s 100 -a 7777/0x06 --add-programinfo-id 0x4B4C5641 \ "
    //!   " ...                                                          "
    //!   where the target pid is 7777 and the attached service is 100.
    //!
    //! References:
    //! https://impleotv.com/2017/02/17/klv-encoded-metadata-in-stanag-4609-streams/
    //!
    class TSDUCKDLL PacketEncapsulation
    {
    public:
        //!
        //! Constructor.
        //! @param [in] pidOutput The output PID. When PID_NULL, no encapsulation is done.
        //! @param [in] pidInput The initial set of PID's to encapsulate.
        //! @param [in] pcrReference The PID with PCR's to use as reference to add PCR's in
        //! the encapsulating PID. When PID_NULL, do not add PCR.
        //!
        PacketEncapsulation(PID pidOutput = PID_NULL, const PIDSet& pidInput = NoPID, PID pcrReference = PID_NULL);

        //!
        //! Reset the encapsulation.
        //! @param [in] pidOutput The new output PID. When PID_NULL, no encapsulation is done.
        //! @param [in] pidInput The new set of PID's to encapsulate.
        //! @param [in] pcrReference The PID with PCR's to use as reference to add PCR's in
        //! the encapsulating PID. When PID_NULL, do not add PCR.
        //!
        void reset(PID pidOutput = PID_NULL, const PIDSet& pidInput = NoPID, PID pcrReference = PID_NULL);

        //!
        //! Process a TS packet from the input stream.
        //! @param [in,out] pkt A TS packet. If the packet belongs to one of the input
        //! PID's, it is replaced by an encapsulating packet in the output PID. Some
        //! null packets are also replaced to absorb the encapsulation overhead.
        //! @return True on success, false on error (PID conflict, output overflow).
        //! In case of error, use lastError().
        //!
        bool processPacket(TSPacket& pkt);

        //!
        //! Get the last error message.
        //! @return The last error message.
        //!
        const UString& lastError() const { return _lastError; }

        //!
        //! Check if a previous error is pending.
        //! @return True if a previous error is pending.
        //! @see resetError()
        //!
        bool hasError() const { return !_lastError.empty(); }

        //!
        //! Reset the last error.
        //!
        void resetError() { _lastError.clear(); }

        //!
        //! Get the output PID.
        //! @return The output PID.
        //!
        PID outputPID() const { return _pidOutput; }

        //!
        //! Change the output PID.
        //! @param [in] pid The new output PID.
        //!
        void setOutputPID(PID pid);

        //!
        //! Get the current set of input PID's.
        //! @return A constant reference to the set of input PID's.
        //!
        const PIDSet& inputPIDs() const { return _pidInput; }

        //!
        //! Get the current number of input PID's being encapsulated.
        //! @return The crrent number of PID's being encapsulated.
        //!
        size_t pidCount() const { return _pidInput.count(); }

        //!
        //! Replace the set of input PID's.
        //! @param [in] pidInput The new set of PID's to encapsulate.
        //!
        void setInputPIDs(const PIDSet& pidInput);

        //!
        //! Add one PID to encapsulate.
        //! @param [in] pid The new PID to encapsulate.
        //!
        void addInputPID(PID pid);

        //!
        //! Remove one PID to encapsulate.
        //! @param [in] pid The PID to no longer encapsulate.
        //!
        void removeInputPID(PID pid);

        //!
        //! Get the reference PID for PCR's.
        //! @return The reference PID for PCR's. PID_NULL if there is none.
        //!
        PID referencePCR() const { return _pcrReference; }

        //!
        //! Change the reference PID for PCR's.
        //! @param [in] pid The new reference PID for PCR's. PID_NULL if there is none.
        //!
        void setReferencePCR(PID pid);

        //!
        //! Default maximum number of buffered packets.
        //!
        static const size_t DEFAULT_MAX_BUFFERED_PACKETS = 1024;

        //!
        //! Set the maximum number of buffered packets.
        //! The buffered packets are produced by the encapsulation overhead.
        //! An overflow is usually caused by insufficient null packets in the input stream.
        //! @param [in] count The maximum number of buffered packets.
        //!
        void setMaxBufferedPackets(size_t count);

        //!
        //! Set packing mode.
        //! When packing mode is of (the default), encapsulated packets are issued
        //! as soon as null packet are available for replacement, potentioally leaving
        //! unused part in some outer packet. When packing mode if off, outer packets
        //! are emitted only when they are full.
        //! @param [in] on Packing mode.
        //!
        void setPacking(bool on, size_t limit) { _packing = on; _packDistance = limit; }
         //!
        //! Set PES mode.
        //! Enbles the PES mode encapsulation (disabled by default).
        //! Accepted values are:
        //! 0=disabled; 1=fixed; 2=variable.
        //! @param [in] on PES mode.
        //!
        void setPES(size_t mode) { _pesmode = mode; }

    private:
        typedef std::map<PID,uint8_t> PIDCCMap;  // map of continuity counters, indexed by PID
        typedef SafePtr<TSPacket> TSPacketPtr;
        typedef std::deque<TSPacketPtr> TSPacketPtrQueue;

        bool             _packing;         // Packing mode.
        size_t           _packDistance;    // Maximum distance between inner packets.
        size_t           _pesmode;         // PES mode selected.
        PID              _pidOutput;       // Output PID.
        PIDSet           _pidInput;        // Input PID's to encapsulate.
        PID              _pcrReference;    // Insert PCR's based on this reference PID.
        UString          _lastError;       // Last error message.
        PacketCounter    _currentPacket;   // Total TS packets since last reset.
        PacketCounter    _pcrLastPacket;   // Packet index of last PCR in reference PID.
        uint64_t         _pcrLastValue;    // Last PCR value in reference PID.
        BitRate          _bitrate;         // Bitrate computed from last PCR.
        bool             _insertPCR;       // Insert a PCR in next output packet.
        uint8_t          _ccOutput;        // Continuity counter in output PID.
        PIDCCMap         _lastCC;          // Continuity counter by PID.
        size_t           _lateDistance;    // Distance from the last packet.
        size_t           _lateMaxPackets;  // Maximum number of packets in _latePackets.
        size_t           _lateIndex;       // Index in first late packet.
        TSPacketPtrQueue _latePackets;     // Packets to insert later.

        // Reset PCR information, lost synchronization.
        void resetPCR();

        // Fill packet payload with data from the first queued packet.
        void fillPacket(TSPacket& pkt, size_t& pktIndex);

        // Inaccessible operations.
        PacketEncapsulation(const PacketEncapsulation&) = delete;
        PacketEncapsulation& operator=(const PacketEncapsulation&) = delete;
    };
}
