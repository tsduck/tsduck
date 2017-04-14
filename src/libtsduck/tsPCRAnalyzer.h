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
//  PCR statistics analysis
//
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsTSPacket.h"

namespace ts {

    class TSDUCKDLL PCRAnalyzer
    {
    public:
        // Constructor
        // Specify the criteria for valid bitrate analysis:
        // Minimum number of PID's, each with a minimum number of PCR"s.
        PCRAnalyzer (size_t min_pid = 1, size_t min_pcr = 64);

        // Destructor
        ~PCRAnalyzer ();

        // Reset all collected information
        void reset ();
        void reset (size_t min_pid, size_t min_pcr);

        // Reset all collected information and use DTS instead of PCR from now on.
        // Using DTS (Decoding Time Stamps, typically in video PIDs) gives less
        // accurate results than PCR (Program Clock Reference) but can save you
        // in the absence of PCR.
        void resetAndUseDTS ();
        void resetAndUseDTS (size_t min_pid, size_t min_dts);

        // Feed the PCR analyzer with a new transport packet.
        // Return true if we have collected enough packet to evaluate TS bitrate.
        bool feedPacket (const TSPacket& pkt);

        // Check if we have collected enough packet to evaluate TS bitrate.
        bool bitrateIsValid () const {return _bitrate_valid;}

        // Return the evaluated TS bitrate in bits/second
        // (based on 188-byte or 204-byte packets).
        BitRate bitrate188 () const;
        BitRate bitrate204 () const;

        // Return the evaluated PID bitrate in bits/second
        // (based on 188-byte or 204-byte packets).
        BitRate bitrate188 (PID pid) const;
        BitRate bitrate204 (PID pid) const;

        // Return the number of TS packets on a PID
        PacketCounter packetCount (PID pid) const;

        // Return all global results at once.
        struct TSDUCKDLL Status
        {
            // Members:
            bool          bitrate_valid;
            BitRate       bitrate_188;
            BitRate       bitrate_204;
            PacketCounter packet_count;
            PacketCounter pcr_count;
            size_t        pcr_pids;

            // Default constructor
            Status ();

            // Constructor from the current status of PCRAnalyzer
            Status (const PCRAnalyzer&);
        };

        void getStatus (Status&) const;

    private:
        // Unreachable constructors and operators.
        PCRAnalyzer (const PCRAnalyzer&);
        PCRAnalyzer& operator= (const PCRAnalyzer&);

        // Process a discontinuity in the transport stream
        void processDiscountinuity();

        // Analysis of one PID
        struct PIDAnalysis 
        {
            // Constructor:
            PIDAnalysis ();
            // Members:
            uint64_t ts_pkt_cnt;       // Count of TS packets
            uint8_t  cur_continuity;   // Current continuity counter
            uint64_t last_pcr_value;   // Last PCR value in this PID
            uint64_t last_pcr_packet;  // Packet index containing first PCR
            uint64_t ts_bitrate_188;   // Sum of all computed TS bitrates (188-byte)
            uint64_t ts_bitrate_204;   // Sum of all computed TS bitrates (204-byte)
            uint64_t ts_bitrate_cnt;   // Count of computed TS bitrates
        };

        // Private members:
        bool   _use_dts;            // Use DTS instead of PCR
        size_t _min_pid;            // Min # of PID
        size_t _min_pcr;            // Min # of PCR per PID
        bool   _bitrate_valid;      // Bitrate evaluation is valid
        uint64_t _ts_pkt_cnt;         // Total TS packets count
        uint64_t _ts_bitrate_188;     // Sum of all computed TS bitrates (188-byte)
        uint64_t _ts_bitrate_204;     // Sum of all computed TS bitrates (204-byte)
        uint64_t _ts_bitrate_cnt;     // Count of computed bitrates
        size_t _completed_pids;     // Number of PIDs with enough PCRs
        size_t _pcr_pids;           // Number of PIDs with PCRs
        PIDAnalysis* _pid[PID_MAX]; // Per-PID stats
    };
}
