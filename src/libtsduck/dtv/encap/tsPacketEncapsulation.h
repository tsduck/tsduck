//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard (PES mode by lars18th)
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
#include "tsTSPacketMetadata.h"

namespace ts {
    //!
    //! An efficient TSDuck-specific TS packets encapsulation in a PID.
    //! @ingroup libtsduck mpeg
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
    //!  This is because the PCR value isn't read at start. But the PTS
    //!  is required to be in all PES packets of the encapsulation.
    //!  So, outer packets are delayed until valid PTS values can be computed.
    //!  If too many initial packets need to be delayed, the first ones are
    //!  discarded.
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
        TS_NOBUILD_NOCOPY(PacketEncapsulation);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to log error or debug messages.
        //! @param [in] output_pid The output PID. When PID_NULL, no encapsulation is done.
        //! @param [in] input_pids The initial set of PID's to encapsulate.
        //! @param [in] input_labels The initial set of packet labels to encapsulate.
        //! @param [in] pcr_reference_pid The PID with PCR's to use as reference to add PCR's in the encapsulating PID.
        //! When @a pcr_reference_pid is PID_NULL and @a pcr_reference_label is NPOS, do not add PCR.
        //! @param [in] pcr_reference_label The label for packets with PCR's to use as reference to add PCR's.
        //!
        PacketEncapsulation(Report& report,
                            PID output_pid = PID_NULL,
                            const PIDSet& input_pids = NoPID(),
                            const TSPacketLabelSet& input_labels = TSPacketLabelSet(),
                            PID pcr_reference_pid = PID_NULL,
                            size_t pcr_reference_label = NPOS);

        //!
        //! Reset the encapsulation.
        //! @param [in] output_pid The new output PID. When PID_NULL, no encapsulation is done.
        //! @param [in] input_pids The new set of PID's to encapsulate.
        //! @param [in] input_labels The initial set of packet labels to encapsulate.
        //! @param [in] pcr_reference_pid The PID with PCR's to use as reference to add PCR's in the encapsulating PID.
        //! When @a pcr_reference_pid is PID_NULL and @a pcr_reference_label is NPOS, do not add PCR.
        //! @param [in] pcr_reference_label The label for packets with PCR's to use as reference to add PCR's.
        //!
        void reset(PID output_pid, const PIDSet& input_pids, const TSPacketLabelSet& input_labels, PID pcr_reference_pid, size_t pcr_reference_label);

        //!
        //! Process a TS packet from the input stream.
        //! @param [in,out] pkt A TS packet. If the packet belongs to one of the input PID's,
        //! it is replaced by an encapsulating packet in the output PID. Some null packets are
        //! also replaced to absorb the encapsulation overhead.
        //! @param [in,out] mdata Corresponding packet metadata.
        //! @return True on success, false on error (PID conflict, output overflow).
        //! In case of error, use lastError().
        //!
        bool processPacket(TSPacket& pkt, TSPacketMetadata& mdata);

        //!
        //! Get the last error message.
        //! @return The last error message.
        //!
        const UString& lastError() const { return _last_error; }

        //!
        //! Check if a previous error is pending.
        //! @return True if a previous error is pending.
        //! @see resetError()
        //!
        bool hasError() const { return !_last_error.empty(); }

        //!
        //! Reset the last error.
        //!
        void resetError() { _last_error.clear(); }

        //!
        //! Get the output PID.
        //! @return The output PID.
        //!
        PID outputPID() const { return _output_pid; }

        //!
        //! Change the output PID.
        //! @param [in] pid The new output PID.
        //!
        void setOutputPID(PID pid);

        //!
        //! Get the current set of input PID's.
        //! @return A constant reference to the set of input PID's.
        //!
        const PIDSet& inputPIDs() const { return _input_pids; }

        //!
        //! Get the current number of input PID's being encapsulated.
        //! @return The crrent number of PID's being encapsulated.
        //!
        size_t pidCount() const { return _input_pids.count(); }

        //!
        //! Replace the set of input PID's.
        //! The set of input packet labels is unchanged.
        //! @param [in] input_pids The new set of PID's to encapsulate.
        //!
        void setInputPIDs(const PIDSet& input_pids);

        //!
        //! Get the current set of input packet labels.
        //! @return A constant reference to the set of input packet labels.
        //!
        const TSPacketLabelSet& inputLabels() const { return _input_labels; }

        //!
        //! Replace the set of input packet labels.
        //! The set of input PID's is unchanged.
        //! @param [in] input_labels The new set of packet labels to encapsulate.
        //!
        void setInputLabels(const TSPacketLabelSet& input_labels);

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
        PID referencePCR() const { return _pcr_ref_pid; }

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
        void setPacking(bool on, size_t limit) { _packing = on; _pack_distance = limit; }

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
        void setPES(PESMode mode) { _pes_mode = mode; }

        //!
        //! Set PES Offset.
        //! When using the PES mode it enables the PES Synchronous encapsulation when != 0.
        //! The offset value is used to compute the PTS of the encap stream based on the PCR.
        //! @param [in] offset value. Use 0 for Asynchronous PES mode (default).
        //!
        void setPESOffset(int32_t offset);

        //!
        //! In synchronous PES mode, drop or delay initial packets before the first PCR.
        //! In synchronous PES mode, all outer packets must contain a PTS. However, a PTS
        //! cannot be computed before getting the first PCR. If initial input packets arrive
        //! before the first PCR, they cannot be immediately encapsulated. By default, they
        //! are delayed until the first PCR is found, when PTS can be computed. Using this
        //! method, it is possible to drop these initial packets instead of delaying them.
        //! @param [in] drop If true, initial input packets before a PCR are dropped. If false
        //! (the default), the input packets are delayed and inserted in the outer PID after
        //! the first PCR is found.
        //!
        void setInitialPacketDrop(bool drop) { _drop_before_pts = drop; }

    private:
        using PIDCCMap = std::map<PID,uint8_t>;  // map of continuity counters, indexed by PID
        using TSPacketPtrQueue = std::deque<TSPacketPtr>;

        [[maybe_unused]] Report& _report;
        bool             _packing = false;          // Packing mode.
        bool             _drop_before_pts = false;  // In synchronous PES mode, drop packets before getting a PCR.
        size_t           _pack_distance = NPOS;     // Maximum distance between inner packets.
        PESMode          _pes_mode = DISABLED;      // PES mode selected.
        uint64_t         _pes_offset = 0;           // PES Offset used in the Synchronous mode.
        PID              _output_pid = PID_NULL;    // Output PID.
        PIDSet           _input_pids {};            // Input PID's to encapsulate.
        TSPacketLabelSet _input_labels {};          // Labels of input packets to encapsulate.
        PID              _pcr_ref_pid = PID_NULL;   // Insert PCR's based on this reference PID.
        size_t           _pcr_ref_label = NPOS;     // Insert PCR's based on packets with that labels.
        UString          _last_error {};            // Last error message.
        PacketCounter    _current_packet = 0;       // Total TS packets since last reset.
        PacketCounter    _pcr_last_packet = INVALID_PACKET_COUNTER;  // Packet index of last PCR in reference PID.
        uint64_t         _pcr_last_value = INVALID_PCR;              // Last PCR value in reference PID.
        uint64_t         _pts_previous = INVALID_PTS;                // Previous PTS value in PES ASYNC mode.
        BitRate          _bitrate = 0;              // Bitrate computed from last PCR.
        bool             _insert_pcr = false;       // Insert a PCR in next output packet.
        uint8_t          _cc_output = 0;            // Continuity counter in output PID.
        uint8_t          _cc_pes {1};               // Continuity counter in PES ASYNC mode.
        PIDCCMap         _last_cc {};               // Continuity counter by PID.
        size_t           _late_distance = 0;        // Distance from the last packet in output PID.
        size_t           _late_max_packets = DEFAULT_MAX_BUFFERED_PACKETS;  // Maximum number of packets in _latePackets.
        size_t           _late_index = 0;           // Next index to read in first late packet.
        TSPacketPtrQueue _late_packets {};          // Packets to insert later.
        size_t           _delayed_initial = 0;      // Number of initial delayed packets before computing PTS (synchronous PES mode).

        // Reset PCR information, lost synchronization.
        void resetPCR();

        // Fill packet payload with data from the first queued packet.
        void fillPacket(TSPacket& pkt, size_t& pkt_index);

        // Compute the PCR distance from this packet to last PCR.
        uint64_t getPCRDistance() { return PacketInterval<PCR>(_bitrate, _current_packet - _pcr_last_packet).count(); }
    };
}
