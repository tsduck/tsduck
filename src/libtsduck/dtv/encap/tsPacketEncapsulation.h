//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard (PES mode by lars18th)
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  An efficient TSDuck-specific TS packets encapsulation in a PID.
//!
//----------------------------------------------------------------------------

#pragma once
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
    //! overhead is around 14-20% of additional data.
    //!
    //! The PES envelope uses a KLVA SMPTE-336M encapsulation to insert the
    //! inner payload into one private (testing) key. Each TS packet contains
    //! only one key, with a size no larger than the payload of one TS packet.
    //! So each PES packet fits into a single TS packet.
    //!
    //! The SMPTE-336M encapsulation implemented can be either the
    //! asynchronous (without timestamps) or the synchronous (with PTS).
    //! The latter consumes more space (+10 bytes), and it's only useful when
    //! it's needed to remux the encapsulated stream with an external tool
    //! that requires to use PTS marks. No other advantages are provided.
    //!
    //! Two variant strategies are implemented. The FIXED mode uses the
    //! short (7-bit) BER encoding. This limits the PES payload to a maximum
    //! of 127 bytes. And the adaptation field of the outer packet is
    //! enlarged with some stuff. However, the advantage is that the PES
    //! is sufficient small to include more data in the outer TS packet.
    //! This reduces the possibility than some external processing will
    //! split the outer packet in two to accommodate the entire PES data.
    //!
    //! The VARIABLE mode does not impose this restriction and outer
    //! packets are filled to the maximum. The drawback is that sometimes
    //! the long form of BER encoding is used with two bytes and others
    //! the short form with one byte. Furthermore, this increases the chances
    //! that some external processing occupies two outer packets for the
    //! same inner PES packet. Still, support for those split PES packets
    //! is included. The only requirement is that the 26|27 PES+KLVA header
    //! is inserted in the first packet (with PUSI on). The remaining
    //! payload can be distributed in the following TS packets.
    //!
    //! The PES envelope has an overhead of 26|27|36|37 bytes based on:
    //! - 9 bytes for the PES header.
    //! - 0|5 bytes for the PTS (only in synchronous mode)
    //! - 0|5 bytes for the Metadata AU Header (only in synchronous mode)
    //! - 16 bytes for the UL key
    //! - 1|2 bytes for the payload size (BER short or long format)
    //!
    //! To enable the use of the Synchronous encapsulation is required
    //! to use PCRs and provide one offset. This value (positive or negative)
    //! will be added to the PCR to compute the PTS. Recommended values are
    //! between -90000 and +90000 (-1,+1 second). If you use negative values
    //! then you can restore in advance the encapsulated stream after
    //! remuxing. However, this will be valid only if you use an external
    //! tool to remux. If you're unsure, then don't enable it.
    //!
    //! A warning about the Synchronous mode:
    //!  At start the PTS marks can't be in synch with the target pcr-pid.
    //!  This is because the PCR value isn't readed at start. But the PTS
    //!  is required to be in all PES packets of the encapsulation.
    //!  So, it's recommended to discard the outcoming stream until valid
    //!  PTS values appear in the encapsulated stream.
    //!
    //! In order to correctly identify the encapsulated PES stream, it is
    //! recommended to include in the PMT table a format identifier
    //! descriptor for "KLVA" (0x4B4C5641); and use the associated metadata
    //! for the stream type based on the selected Sync/Async mode:
    //!  - Asynchronous mode: Private Type (0x06)
    //!  - Synchronous mode: Metadata Type (0x15)
    //!
    //! Example (Asynchronous):
    //! @code
    //! tsp ...
    //!     -P encap -o 7777 --pes-mode ...
    //!     -P pmt -s 100 -a 7777/0x06 --add-programinfo-id 0x4B4C5641
    //!     ...
    //! @endcode
    //! where the outer PID is 7777 and the attached service is 100.
    //!
    //! Example (Synchronous):
    //! @code
    //! tsp ...
    //!     -P encap -o 7777 --pes-mode ... --pcr-pid 101 --pes-offset=-50000
    //!     -P pmt -s 100 -a 7777/0x15 --add-programinfo-id 0x4B4C5641
    //!     ...
    //! @endcode
    //! where the outer PID is 7777, the pid 101 carries the PCR of the service,
    //! the attached service is 100 and the PTS is advanced around half second.
    //!
    //! @see https://impleotv.com/2017/02/17/klv-encoded-metadata-in-stanag-4609-streams/
    //!
    class TSDUCKDLL PacketEncapsulation
    {
        TS_NOCOPY(PacketEncapsulation);
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
        static constexpr size_t DEFAULT_MAX_BUFFERED_PACKETS = 1024;

        //!
        //! Set the maximum number of buffered packets.
        //! The buffered packets are produced by the encapsulation overhead.
        //! An overflow is usually caused by insufficient null packets in the input stream.
        //! @param [in] count The maximum number of buffered packets.
        //!
        void setMaxBufferedPackets(size_t count);

        //!
        //! Set packing mode.
        //! When packing mode is off (the default), encapsulated packets are issued
        //! as soon as null packet are available for replacement, potentially leaving
        //! unused part in some outer packet. When packing mode if on, outer packets
        //! are emitted only when they are full.
        //! @param [in] on Packing mode.
        //! @param [in] limit Number of packets after which encapsulation is forced, even when packing is off.
        //!
        void setPacking(bool on, size_t limit) { _packing = on; _packDistance = limit; }

        //!
        //! Type of PES encapsulation mode.
        //!
        enum PESMode {
            DISABLED = 0,  //!< PES mode is disabled.
            FIXED    = 1,  //!< Fixed PES mode.
            VARIABLE = 2,  //!< Variable PES mode.
        };

        //!
        //! Set PES mode.
        //! Enables the PES mode encapsulation (disabled by default).
        //! @param [in] mode PES mode.
        //!
        void setPES(PESMode mode) { _pesMode = mode; }

        //!
        //! Set PES Offset.
        //! When using the PES mode it enables the PES Synchronous encapsulation when != 0.
        //! The offset value is used to compute the PTS of the encap stream based on the PCR.
        //! @param [in] offset value. Use 0 for Asynchronous PES mode (default).
        //!
        void setPESOffset(size_t offset) { _pesOffset = offset; }

    private:
        typedef std::map<PID,uint8_t> PIDCCMap;  // map of continuity counters, indexed by PID
        typedef SafePtr<TSPacket> TSPacketPtr;
        typedef std::deque<TSPacketPtr> TSPacketPtrQueue;

        bool             _packing = false;         // Packing mode.
        size_t           _packDistance = NPOS;     // Maximum distance between inner packets.
        PESMode          _pesMode = DISABLED;      // PES mode selected.
        size_t           _pesOffset = 0;           // PES Offset used in the Synchronous mode.
        PID              _pidOutput = PID_NULL;    // Output PID.
        PIDSet           _pidInput {};             // Input PID's to encapsulate.
        PID              _pcrReference = PID_NULL; // Insert PCR's based on this reference PID.
        UString          _lastError {};            // Last error message.
        PacketCounter    _currentPacket = 0;       // Total TS packets since last reset.
        PacketCounter    _pcrLastPacket = INVALID_PACKET_COUNTER;   // Packet index of last PCR in reference PID.
        uint64_t         _pcrLastValue = INVALID_PCR;               // Last PCR value in reference PID.
        uint64_t         _ptsPrevious = INVALID_PTS;                // Previous PTS value in PES ASYNC mode.
        BitRate          _bitrate = 0;             // Bitrate computed from last PCR.
        bool             _insertPCR = false;       // Insert a PCR in next output packet.
        uint8_t          _ccOutput = 0;            // Continuity counter in output PID.
        uint8_t          _ccPES {1};               // Continuity counter in PES ASYNC mode.
        PIDCCMap         _lastCC {};               // Continuity counter by PID.
        size_t           _lateDistance = 0;        // Distance from the last packet.
        size_t           _lateMaxPackets = DEFAULT_MAX_BUFFERED_PACKETS;  // Maximum number of packets in _latePackets.
        size_t           _lateIndex = 0;           // Index in first late packet.
        TSPacketPtrQueue _latePackets {};          // Packets to insert later.

        // Reset PCR information, lost synchronization.
        void resetPCR();

        // Fill packet payload with data from the first queued packet.
        void fillPacket(TSPacket& pkt, size_t& pktIndex);

        // Compute the PCR distance from this packe to last PCR.
        uint64_t getPCRDistance() { return (PacketInterval(_bitrate, _currentPacket - _pcrLastPacket) * SYSTEM_CLOCK_FREQ) / MilliSecPerSec; }
    };
}
