//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsTSPacket.h"
#include "tsPESPacket.h"
#include "tsPESHandlerInterface.h"
#include "tsAudioAttributes.h"
#include "tsVideoAttributes.h"
#include "tsAVCAttributes.h"
#include "tsAC3Attributes.h"

namespace ts {
    //!
    //! This class extracts PES packets from TS packets.
    //!
    class TSDUCKDLL PESDemux
    {
    public:
        //!
        //! Constructor.
        //! @param [in] handler The object to invoke when PES packets are analyzed.
        //! @param [in] pids The set of PID's to demux.
        //!
        PESDemux(PESHandlerInterface* handler = 0, const PIDSet& pids = AllPIDs);

        //!
        //! Destructor.
        //!
        ~PESDemux();

        //!
        //! The following method feeds the demux with a TS packet.
        //! @param [in] pkt A TS packet. If the PID of the packet is not in the
        //! PID filter, it is ignored. Thus, a TS packet reader can simply pass
        //! all TS packets to the demux.
        //!
        void feedPacket(const TSPacket& pkt)
        {
            if (_pid_filter[pkt.getPID()]) {
                processPacket(pkt);
            }
            _packet_count++;
        }

        //!
        //! Replace the list of PID's to filter.
        //! @param [in] pid_filter The list of PID's to filter.
        //!
        void setPIDFilter(const PIDSet& pid_filter);

        //!
        //! Add one PID to filter.
        //! @param [in] pid The new PID to filter.
        //!
        void addPID(PID pid)
        {
            _pid_filter.set(pid);
        }

        //!
        //! Remove one PID to filter.
        //! @param [in] pid The PID to no longer filter.
        //!
        void removePID(PID pid);

        //!
        //! Get the current number of PID's being filtered..
        //! @return The current number of PID's being filtered..
        //!
        size_t pidCount() const
        {
            return _pid_filter.count();
        }

        //!
        //! Replace the PES packet handler.
        //! @param [in] h The object to invoke when PES packets are analyzed.
        //!
        void setPESHandler(PESHandlerInterface* h)
        {
            _pes_handler = h;
        }

        //!
        //! Reset the analysis context.
        //! Partially built PES packets are dropped.
        //! Useful when the transport stream changes.
        //! The PID filter and the handlers are not modified.
        //!
        void reset();

        //!
        //! Reset the analysis context for one single PID.
        //! Partially built PES packets are dropped.
        //! @param [in] pid The PID to reset.
        //!
        void resetPID(PID pid);

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

    private:
        // Inacessible operations
        PESDemux(const PESDemux&) = delete;
        PESDemux& operator=(const PESDemux&) = delete;

        // This internal structure contains the analysis context for one PID.
        struct PIDContext
        {
            PacketCounter pes_count; // Number of detected valid PES packets on this PID
            uint8_t continuity;        // Last continuity counter
            bool sync;               // We are synchronous in this PID
            PacketCounter first_pkt; // Index of first TS packet for current PES packet
            PacketCounter last_pkt;  // Index of last TS packet for current PES packet
            ByteBlockPtr ts;         // TS payload buffer
            bool reset_pending;      // Delayed reset on this PID
            AudioAttributes audio;   // Current audio attributes
            VideoAttributes video;   // Current video attributes (MPEG-1, MPEG-2)
            AVCAttributes avc;       // Current AVC attributes
            AC3Attributes ac3;       // Current AC-3 attributes
            PacketCounter ac3_count; // Number of PES packets with contents which looks like AC-3

            // Default constructor:
            PIDContext();

            // Called when packet synchronization is lost on the pid
            void syncLost() {sync = false; ts->clear();}
        };

        typedef std::map <PID, PIDContext> PIDContextMap;

        // Feed the demux with a TS packet (PID already filtered).
        void processPacket(const TSPacket&);

        // Process a complete PES packet
        void processPESPacket(PID, PIDContext&);

        // Private members:
        PESHandlerInterface* _pes_handler;
        PIDSet _pid_filter;
        PIDContextMap _pids;
        PacketCounter _packet_count;  // number of TS packets in demultiplexed stream
        bool _in_handler;             // true when in the context of a handler
        PID _pid_in_handler;          // PID which is currently processed by handler
        bool _reset_pending;          // delayed reset()
    };
}
