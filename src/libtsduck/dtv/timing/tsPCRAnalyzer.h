//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  PCR statistics analysis
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsStringifyInterface.h"

namespace ts {
    //!
    //! PCR statistics analysis.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PCRAnalyzer
    {
        TS_NOCOPY(PCRAnalyzer);
    public:
        //!
        //! Constructor.
        //! The parameters specify the criteria for valid bitrate analysis.
        //! @param [in] min_pid Minimum number of PID's with PCR's.
        //! @param [in] min_pcr Minimum number of PCR's per PID.
        //!
        PCRAnalyzer(size_t min_pid = 1, size_t min_pcr = 64);

        //!
        //! Destructor.
        //!
        ~PCRAnalyzer();

        //!
        //! Reset all collected information.
        //!
        void reset();

        //!
        //! Reset all collected information and change criteria for valid bitrate analysis.
        //! @param [in] min_pid Minimum number of PID's with PCR's.
        //! @param [in] min_pcr Minimum number of PCR's per PID.
        //!
        void reset(size_t min_pid, size_t min_pcr);

        //!
        //! Reset all collected information and use DTS instead of PCR from now on.
        //! Using DTS (Decoding Time Stamps, typically in video PIDs) gives less
        //! accurate results than PCR (Program Clock Reference) but can save you
        //! in the absence of PCR.
        //!
        void resetAndUseDTS();

        //!
        //! Reset all collected information and use DTS instead of PCR from now on.
        //! Using DTS (Decoding Time Stamps, typically in video PIDs) gives less
        //! accurate results than PCR (Program Clock Reference) but can save you
        //! in the absence of PCR. Also change criteria for valid bitrate analysis.
        //! @param [in] min_pid Minimum number of PID's with DTS's.
        //! @param [in] min_dts Minimum number of DTS's per PID.
        //!
        void resetAndUseDTS(size_t min_pid, size_t min_dts);

        //!
        //! Ignore transport stream errors such as discontinuities.
        //! By default, TS errors are not ignored. Discontinuities and other errors
        //! suspend the analysis until the stream is resynchronized. This can be
        //! summarized as follow:
        //! - When errors are not ignored (the default), the bitrate of the original
        //!   stream (before corruptions) is evaluated.
        //! - When errors are ignored, the bitrate of the received stream is evaluated,
        //!   "missing" packets being considered as non-existent.
        //! @param [in] ignore When true, ignore errors.
        //!
        void setIgnoreErrors(bool ignore);

        //!
        //! The following method feeds the analyzer with a TS packet.
        //! @param [in] pkt A new transport stream packet.
        //! @return True if we have collected enough packet to evaluate TS bitrate.
        //!
        bool feedPacket(const TSPacket& pkt);

        //!
        //! Check if we have collected enough packet to evaluate TS bitrate.
        //! @return True if we have collected enough packet to evaluate TS bitrate.
        //!
        bool bitrateIsValid() const {return _bitrate_valid;}

        //!
        //! Get the evaluated TS bitrate in bits/second based on 188-byte packets.
        //! @return The evaluated TS bitrate in bits/second based on 188-byte packets.
        //!
        BitRate bitrate188() const;

        //!
        //! Get the evaluated TS bitrate in bits/second based on 204-byte packets.
        //! @return The evaluated TS bitrate in bits/second based on 204-byte packets.
        //!
        BitRate bitrate204() const;

        //!
        //! Get the evaluated PID bitrate in bits/second based on 188-byte packets.
        //! @param [in] pid The PID to evaluate.
        //! @return The evaluated bitrate of @a pid in bits/second based on 188-byte packets.
        //!
        BitRate bitrate188(PID pid) const;

        //!
        //! Get the evaluated PID bitrate in bits/second based on 204-byte packets.
        //! @param [in] pid The PID to evaluate.
        //! @return The evaluated bitrate of @a pid in bits/second based on 204-byte packets.
        //!
        BitRate bitrate204(PID pid) const;

        //!
        //! Get the evaluated TS bitrate in bits/second based on 188-byte packets for the last second.
        //! @return The evaluated TS bitrate in bits/second based on 188-byte packets.
        //!
        BitRate instantaneousBitrate188() const;

        //!
        //! Get the evaluated TS bitrate in bits/second based on 204-byte packets for the last second.
        //! @return The evaluated TS bitrate in bits/second based on 204-byte packets.
        //!
        BitRate instantaneousBitrate204() const;

        //!
        //! Get the number of TS packets on a PID.
        //! @param [in] pid The PID to evaluate.
        //! @return The number of TS packets on @a pid.
        //!
        PacketCounter packetCount(PID pid) const;

        //!
        //! Structure containing the global PCR analysis results.
        //!
        struct TSDUCKDLL Status: public StringifyInterface
        {
            // Members:
            bool          bitrate_valid;   //!< True if bitrate was evaluated.
            BitRate       bitrate_188;     //!< The evaluated TS bitrate in bits/second based on 188-byte packets.
            BitRate       bitrate_204;     //!< The evaluated TS bitrate in bits/second based on 204-byte packets.
            PacketCounter packet_count;    //!< The total number of analyzed TS packets.
            PacketCounter pcr_count;       //!< The number of analyzed PCR's.
            size_t        pcr_pids;        //!< The number of PID's with PCR's.
            size_t        discontinuities; //!< The number of discontinuities.
            BitRate       instantaneous_bitrate_188;  //!< The evaluated TS bitrate in bits/second based on 188-byte packets for the last second.
            BitRate       instantaneous_bitrate_204;  //!< The evaluated TS bitrate in bits/second based on 204-byte packets for the last second.

            //!
            //! Default constructor.
            //!
            Status();

            //!
            //! Constructor from the current status of PCRAnalyzer.
            //! @param [in] zer The PCRAnalyzer to get the status from.
            //!
            Status(const PCRAnalyzer& zer);

            // Implementation of StringifyInterface.
            virtual UString toString() const override;
        };

        //!
        //! Get the global PCR analysis results.
        //! @param [out] status The returned PCR analysis results.
        //!
        void getStatus(Status& status) const;

    private:
        // Process a discontinuity in the transport stream
        void processDiscontinuity();

        // Analysis of one PID
        struct PIDAnalysis
        {
            // Constructor:
            PIDAnalysis();
            // Members:
            uint64_t ts_pkt_cnt;       // Count of TS packets
            uint8_t  cur_continuity;   // Current continuity counter
            uint64_t last_pcr_value;   // Last PCR/DTS value in this PID
            uint64_t last_pcr_packet;  // Packet index containing last PCR/DTS
            BitRate  ts_bitrate_188;   // Sum of all computed TS bitrates (188-byte)
            BitRate  ts_bitrate_204;   // Sum of all computed TS bitrates (204-byte)
            uint64_t ts_bitrate_cnt;   // Count of computed TS bitrates
        };

        // Private members:
        bool     _use_dts;             // Use DTS instead of PCR
        bool     _ignore_errors;       // Ignore TS errors such as discontinuities.
        size_t   _min_pid;             // Min # of PID
        size_t   _min_pcr;             // Min # of PCR per PID
        bool     _bitrate_valid;       // Bitrate evaluation is valid
        uint64_t _ts_pkt_cnt;          // Total TS packets count
        BitRate  _ts_bitrate_188;      // Sum of all computed TS bitrates (188-byte)
        BitRate  _ts_bitrate_204;      // Sum of all computed TS bitrates (204-byte)
        uint64_t _ts_bitrate_cnt;      // Count of computed bitrates
        BitRate  _inst_ts_bitrate_188; // Sum of all computed TS bitrates (188-byte) for last second
        BitRate  _inst_ts_bitrate_204; // Sum of all computed TS bitrates (204-byte) for last second
        size_t   _completed_pids;      // Number of PIDs with enough PCRs
        size_t   _pcr_pids;            // Number of PIDs with PCRs
        size_t   _discontinuities;     // Number of discontinuities
        PIDAnalysis* _pid[PID_MAX];    // Per-PID stats
        std::map<uint64_t, uint64_t> _packet_pcr_index_map; // Map of PCR/DTS to packet index across entire TS
        static constexpr size_t FOOLPROOF_MAP_LIMIT = 1000; // Max number of entries in the PCR map
    };
}
