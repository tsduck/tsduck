//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  This class extracts PES packets from TS packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTimeTrackerDemux.h"
#include "tsPESPacket.h"
#include "tsPESHandlerInterface.h"
#include "tsMPEG2AudioAttributes.h"
#include "tsMPEG2VideoAttributes.h"
#include "tsAVCAttributes.h"
#include "tsHEVCAttributes.h"
#include "tsAC3Attributes.h"
#include "tsSectionDemux.h"

namespace ts {
    //!
    //! This class extracts PES packets from TS packets.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PESDemux: public TimeTrackerDemux, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(PESDemux);
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef TimeTrackerDemux SuperClass;

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] handler The object to invoke when PES packets are analyzed.
        //! @param [in] pids The set of PID's to demux.
        //!
        explicit PESDemux(DuckContext& duck, PESHandlerInterface* handler = nullptr, const PIDSet& pids = AllPIDs);

        //!
        //! Destructor.
        //!
        virtual ~PESDemux() override;

        // Inherited methods
        virtual void feedPacket(const TSPacket& pkt) override;

        //!
        //! Flush any unterminated unbounded PES packet on the specified PID.
        //! Unbounded PES packets have no predetermined size. They implicitely end when the next PES
        //! packet starts on the same PID. However, at end of stream, there is no next PES packet and
        //! the previous buffered data are not considered as a full unbounded packet. These data are lost.
        //! This method shall be called at end of stream when the caller is certain that the buffered data
        //! from the PID form a complete PES packet. This PES packet is then processed.
        //! @param [in] pid PID containing an unbounded PES packet to complete.
        //!
        void flushUnboundedPES(PID pid);

        //!
        //! Flush any unterminated unbounded PES packet on all PID's.
        //! @see flushUnboundedPES(PID)
        //!
        void flushUnboundedPES();

        //!
        //! Replace the PES packet handler.
        //! @param [in] h The object to invoke when PES packets are analyzed.
        //!
        void setPESHandler(PESHandlerInterface* h) { _pes_handler = h; }

        //!
        //! Set the default audio or video codec for all analyzed PES PID's.
        //! The analysis of the content of a PES packet sometimes depends on the PES data format.
        //! The PES demux uses several ways to determine the data format inside a PES packet.
        //! First, when the packet is identified in a PMT, the stream type may uniquely identify the format.
        //! Second, the content itself can be identified as a specific format. Finally, in the absence of
        //! other indications, the specified @a codec is used.
        //! @param [in] codec The default codec to use.
        //!
        void setDefaultCodec(CodecType codec) { _default_codec = codec; }

        //!
        //! Set the default audio or video codec for one specific PES PID's.
        //! This is the same as setDefaultCodec(CodecType) for one specific PID.
        //! The codec of a PID is automatically determined from the characteristics
        //! of this PID in the PMT, if the PMT packets are passed to this demux.
        //! @param [in] pid The PID to identify.
        //! @param [in] codec The default codec to use.
        //! @see setDefaultCodec(CodecType)
        //!
        void setDefaultCodec(PID pid, CodecType codec);

        //!
        //! Get the default codec type on a given PID.
        //! @param [in] pid The PID to check.
        //! @return The default codec to use on @a pid.
        //! @see setDefaultCodec()
        //!
        CodecType getDefaultCodec(PID pid) const;

        //!
        //! Get the current audio attributes on the specified PID.
        //! @param [in] pid The PID to check.
        //! @param [out] attr The returned attributes.
        //! Invoke its isValid() method to verify its validity.
        //!
        void getAudioAttributes(PID pid, MPEG2AudioAttributes& attr) const;

        //!
        //! Get the current MPEG-2 video attributes on the specified PID.
        //! @param [in] pid The PID to check.
        //! @param [out] attr The returned attributes.
        //! Invoke its isValid() method to verify its validity.
        //!
        void getVideoAttributes(PID pid, MPEG2VideoAttributes& attr) const;

        //!
        //! Get the current AVC video attributes on the specified PID.
        //! @param [in] pid The PID to check.
        //! @param [out] attr The returned attributes.
        //! Invoke its isValid() method to verify its validity.
        //!
        void getAVCAttributes(PID pid, AVCAttributes& attr) const;

        //!
        //! Get the current HEVC video attributes on the specified PID.
        //! @param [in] pid The PID to check.
        //! @param [out] attr The returned attributes.
        //! Invoke its isValid() method to verify its validity.
        //!
        void getHEVCAttributes(PID pid, HEVCAttributes& attr) const;

        //!
        //! Get the current AC-3 audio attributes on the specified PID.
        //! @param [in] pid The PID to check.
        //! @param [out] attr The returned attributes.
        //! Invoke its isValid() method to verify its validity.
        //!
        void getAC3Attributes(PID pid, AC3Attributes& attr) const;

        //!
        //! Check if all PES packets on the specified PID contain AC-3 audio.
        //! @param [in] pid The PID to check.
        //! @return True if all PES packets on the specified PID contain AC-3 audio.
        //! Due to the way AC-3 is detected, it is possible that some PES packets
        //! are erroneously detected as AC-3. Thus, getAC3Attributes() returns
        //! a valid value since some AC-3 was detected. But, on homogeneous
        //! streams, it is safe to assume that the PID really contains AC-3 only if
        //! all PES packets contain AC-3.
        //!
        bool allAC3(PID pid) const;

    protected:
        //!
        //! This hook is invoked when a complete PES packet is available.
        //! Can be overloaded by subclasses to add intermediate processing.
        //! @param [in] packet The PES packet.
        //!
        virtual void handlePESPacket(const PESPacket& packet);

        // Inherited methods
        virtual void immediateReset() override;
        virtual void immediateResetPID(PID pid) override;

    private:
        // This internal structure contains the analysis context for one PID.
        struct PIDContext
        {
            PacketCounter        pes_count = 0;   // Number of detected valid PES packets on this PID
            uint8_t              continuity = 0;  // Last continuity counter
            bool                 sync = false;        // We are synchronous in this PID
            PacketCounter        first_pkt = 0;   // Index of first TS packet for current PES packet
            PacketCounter        last_pkt = 0;    // Index of last TS packet for current PES packet
            uint64_t             pcr {INVALID_PCR};         // First PCR for current PES packet
            ByteBlockPtr         ts {};          // TS payload buffer
            MPEG2AudioAttributes audio {};       // Current audio attributes
            MPEG2VideoAttributes video {};       // Current video attributes (MPEG-1, MPEG-2)
            AVCAttributes        avc {};         // Current AVC attributes
            HEVCAttributes       hevc {};        // Current HEVC attributes
            AC3Attributes        ac3 {};         // Current AC-3 attributes
            PacketCounter        ac3_count = 0;   // Number of PES packets with contents which looks like AC-3

            // Default constructor:
            PIDContext() : ts(new ByteBlock()) {}

            // Called when packet synchronization is lost on the PID.
            void syncLost() { sync = false; ts->clear(); }
        };

        // Map of PID contexts, indexed by PID.
        // One context is created per demuxed PES PID.
        typedef std::map<PID,PIDContext> PIDContextMap;

        // This internal structure describes the content of one PID.
        struct PIDType
        {
            uint8_t   stream_type {ST_NULL};                 // Stream type from PMT.
            CodecType default_codec {CodecType::UNDEFINED};  // Default codec if not otherwise sepcified.

            // Default constructor:
            PIDType() = default;
        };

        // Map of PID types, indexed by PID.
        // All known PID's are referenced here, not only demuxed PES PID's.
        typedef std::map<PID,PIDType> PIDTypeMap;

        // Feed the demux with a TS packet (PID already filtered).
        void processPacket(const TSPacket&);

        // If a PID context contains a complete PES packet with specified length, process it.
        void processPESPacketIfComplete(PID, PIDContext&);

        // Process a complete PES packet
        void processPESPacket(PID, PIDContext&);

        // Process all video/audio analysis on the PES packet.
        void handlePESContent(PIDContext&, const PESPacket&);

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;

        // Private members:
        PESHandlerInterface* _pes_handler = nullptr;
        CodecType            _default_codec {CodecType::UNDEFINED};
        PIDContextMap        _pids {};
        PIDTypeMap           _pid_types {};
        SectionDemux         _section_demux;
    };
}
