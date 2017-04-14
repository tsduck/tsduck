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
//
//  This class rebuilds MPEG tables and sections from TS packets
//
//----------------------------------------------------------------------------

#pragma once
#include "tsETID.h"
#include "tsTSPacket.h"
#include "tsTableHandlerInterface.h"
#include "tsSectionHandlerInterface.h"

namespace ts {

    // General notes:
    // - Long sections are validated with CRC.
    //   Corrupted sections are not reported.
    // - Sections with the 'next' indicator are ignored.
    //   Only sections with the 'current' indicator are used.

    class TSDUCKDLL SectionDemux
    {
    public:
        // Constructor.
        // Specify optional handler for tables and/or sections.
        // Specify list of PID's to filter.
        SectionDemux (TableHandlerInterface* table_handler = 0,
                      SectionHandlerInterface* section_handler = 0,
                      const PIDSet& pid_filter = NoPID);

        // Destructor.
        ~SectionDemux();

        // The following method feeds the demux with a TS packet.
        // If the PID of the packet is not in the PID filter, it is ignored.
        void feedPacket (const TSPacket& pkt)
        {
            if (_pid_filter [pkt.getPID()]) {
                processPacket (pkt);
            }
            _packet_count++;
        }

        // Set the list of PID's to filter. Add/remove single PID's.
        void setPIDFilter (const PIDSet& pid_filter);
        void addPID (PID pid) {_pid_filter.set (pid);}
        void removePID (PID pid);

        // Modify the table and section handler.
        void setTableHandler (TableHandlerInterface* h) {_table_handler = h;}
        void setSectionHandler (SectionHandlerInterface* h) {_section_handler = h;}

        // Reset the analysis context (partially built sections and tables).
        // Useful when the transport stream changes.
        // The PID filter and the handlers are not modified.
        void reset();

        // Reset the analysis context for one single PID.
        // Forget all previously analyzed sections on this PID.
        void resetPID (PID pid);

        // Demux status information
        struct TSDUCKDLL Status
        {
            // Members:
            uint64_t invalid_ts;       // # invalid TS packets
            uint64_t discontinuities;  // # TS packets discontinuities
            uint64_t scrambled;        // # scrambled TS packets (undecoded)
            uint64_t inv_sect_length;  // # invalid section length
            uint64_t inv_sect_index;   // # invalid section index
            uint64_t wrong_crc;        // # sections with wrong CRC32

            // Default constructor
            Status ();

            // Constructor from the current status of SectionDemux
            Status (const SectionDemux&);

            // Reset content
            void reset();

            // Check if any counter is non zero
            bool hasErrors () const;

            // Display content of a status block.
            // Indent indicates the base indentation of lines.
            // If errors_only is true, don't report zero counters.
            std::ostream& display (std::ostream& strm, int indent = 0, bool errors_only = false) const;
        };

        // Get current status of the demux
        void getStatus (Status& status) const {status = _status;}

        // Check if the demux has errors
        bool hasErrors () const {return _status.hasErrors();}

    private:
        // Feed the depacketizer with a TS packet (PID already filtered).
        void processPacket (const TSPacket&);

        // This internal structure contains the analysis context for one TID/TIDext into one PID.
        struct ETIDContext
        {
            uint8_t  version;          // version of this table
            size_t sect_expected;    // # expected sections in table
            size_t sect_received;    // # received sections in table
            SectionPtrVector sects;  // Array of sections

            // Default constructor:
            ETIDContext () :
                version (0), 
                sect_expected (0),
                sect_received (0),
                sects ()
            {
            }
        };

        // This internal structure contains the analysis context for one PID.
        struct PIDContext
        {
            uint8_t continuity;                  // Last continuity counter
            bool sync;                         // We are synchronous in this PID
            ByteBlock ts;                      // TS payload buffer
            std::map <ETID, ETIDContext> tids; // TID analysis contexts
            bool reset_pending;                // Delayed reset on this PID
            PacketCounter pusi_pkt_index;      // Index of last PUSI packet in this PID

            // Default constructor:
            PIDContext () :
                continuity (0),
                sync (false),
                ts (),
                tids (),
                reset_pending (false),
                pusi_pkt_index (0)
            {
            }

            // Called when packet synchronization is lost on the pid
            void syncLost ()
            {
                sync = false;
                ts.clear ();
            }
        };

        // Private members:
        TableHandlerInterface* _table_handler;
        SectionHandlerInterface* _section_handler;
        PIDSet _pid_filter;
        std::map <PID, PIDContext> _pids;
        Status _status;
        PacketCounter _packet_count;  // number of TS packets in demultiplexed stream
        bool _in_handler;             // true when in the context of a table/section handler
        PID _pid_in_handler;          // PID which is currently processed by handler
        bool _reset_pending;          // delayed Reset()

        // Inacessible operations
        SectionDemux (const SectionDemux&);
        SectionDemux& operator= (const SectionDemux&);
    };
}

// Output operator for status
TSDUCKDLL inline std::ostream& operator<< (std::ostream& strm, const ts::SectionDemux::Status& status)
{
    return status.display (strm);
}
