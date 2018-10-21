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
//!  This class extracts PES packets from TS packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTimeTrackerDemux.h"
#include "tsPESPacket.h"
#include "tsPESHandlerInterface.h"
#include "tsAudioAttributes.h"
#include "tsVideoAttributes.h"
#include "tsAVCAttributes.h"
#include "tsAC3Attributes.h"
#include "tsSectionDemux.h"

namespace ts {
    //!
    //! This class extracts PES packets from TS packets.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PESDemux: public TimeTrackerDemux, private TableHandlerInterface
    {
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef TimeTrackerDemux SuperClass;

        //!
        //! Constructor.
        //! @param [in] handler The object to invoke when PES packets are analyzed.
        //! @param [in] pids The set of PID's to demux.
        //!
        PESDemux(PESHandlerInterface* handler = nullptr, const PIDSet& pids = AllPIDs);

        //!
        //! Destructor.
        //!
        virtual ~PESDemux();

        // Inherited methods
        virtual void feedPacket(const TSPacket& pkt) override;

        //!
        //! Replace the PES packet handler.
        //! @param [in] h The object to invoke when PES packets are analyzed.
        //!
        void setPESHandler(PESHandlerInterface* h)
        {
            _pes_handler = h;
        }

        //!
        //! Get the current audio attributes on the specified PID.
        //! @param [in] pid The PID to check.
        //! @param [out] attr The returned attributes.
        //! Invoke its isValid() method to verify its validity.
        //!
        void getAudioAttributes(PID pid, AudioAttributes& attr) const;

        //!
        //! Get the current MPEG-2 video attributes on the specified PID.
        //! @param [in] pid The PID to check.
        //! @param [out] attr The returned attributes.
        //! Invoke its isValid() method to verify its validity.
        //!
        void getVideoAttributes(PID pid, VideoAttributes& attr) const;

        //!
        //! Get the current AVC video attributes on the specified PID.
        //! @param [in] pid The PID to check.
        //! @param [out] attr The returned attributes.
        //! Invoke its isValid() method to verify its validity.
        //!
        void getAVCAttributes(PID pid, AVCAttributes& attr) const;

        //!
        //! Get the current AC-3 audio attributes on the specified PID.
        //! @param [in] pid The PID to check.
        //! @param [out] attr The returned attributes.
        //! Invoke its isValid() method to verify its validity.
        //!
        void getAC3Attributes(PID pid, AC3Attributes& attr) const;

        //!
        //! Check if all PES packets on the specified PID contain AC-3 audio.
        //! @return True if all PES packets on the specified PID contain AC-3 audio.
        //! Due to the way AC-3 is detected, it is possible that some PES packets
        //! are erroneously detected as AC-3. Thus, getAC3Attributes() returns
        //! a valid value since some AC-3 was detected. But, on homogeneous
        //! streams, it is safe to assume that the PID really contains AC-3 only if
        //! all PES packets contain AC-3.
        //!
        bool allAC3(PID) const;

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
            PacketCounter   pes_count;   // Number of detected valid PES packets on this PID
            uint8_t         continuity;  // Last continuity counter
            bool            sync;        // We are synchronous in this PID
            PacketCounter   first_pkt;   // Index of first TS packet for current PES packet
            PacketCounter   last_pkt;    // Index of last TS packet for current PES packet
            ByteBlockPtr    ts;          // TS payload buffer
            AudioAttributes audio;       // Current audio attributes
            VideoAttributes video;       // Current video attributes (MPEG-1, MPEG-2)
            AVCAttributes   avc;         // Current AVC attributes
            AC3Attributes   ac3;         // Current AC-3 attributes
            PacketCounter   ac3_count;   // Number of PES packets with contents which looks like AC-3

            // Default constructor:
            PIDContext();

            // Called when packet synchronization is lost on the pid
            void syncLost() {sync = false; ts->clear();}
        };

        // Map of PID contexts, indexed by PID.
        // One context is created per demuxed PES PID.
        typedef std::map<PID,PIDContext> PIDContextMap;

        // Map of stream types (from PMT), indexed by PID.
        // All known PID's are referenced here, not only demuxed PES PID's.
        typedef std::map<PID,uint8_t> StreamTypeMap;

        // Feed the demux with a TS packet (PID already filtered).
        void processPacket(const TSPacket&);

        // Process a complete PES packet
        void processPESPacket(PID, PIDContext&);

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;

        // Private members:
        PESHandlerInterface* _pes_handler;
        PIDContextMap        _pids;
        StreamTypeMap        _stream_types;
        SectionDemux         _section_demux;

        // Inaccessible operations
        PESDemux(const PESDemux&) = delete;
        PESDemux& operator=(const PESDemux&) = delete;
    };
}
