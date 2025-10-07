//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Inter-packet Arrival Time (IAT) analysis for datagram-based inputs
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsSingleDataStatistics.h"

namespace ts {
    //!
    //! Inter-packet Arrival Time (IAT) analysis for datagram-based inputs.
    //! IAT measures the interval between two input datagrams. Each datagram typically
    //! contains several TS packets. Therefore, the IAT is *not* an interval between
    //! TS packets. IAT analysis is possible only when the origin of the TS packets is
    //! a datagram-based input such as UDP (live or from pcap file), SRT, RIST.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL IATAnalyzer
    {
        TS_NOBUILD_NOCOPY(IATAnalyzer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors. A reference is kept in this object instance.
        //!
        IATAnalyzer(Report& report);

        //!
        //! Reset all collected information.
        //!
        void reset();

        //!
        //! The following method feeds the analyzer with a TS packet.
        //! @param [in] pkt A new transport stream packet.
        //! @param [in] mdata Associated metadata.
        //!
        void feedPacket(const TSPacket& pkt, const TSPacketMetadata& mdata);

        //!
        //! Check if the IAT analysis in progress is valid.
        //! @return True if the analysis is either not started or if valid IAT analysis is in progress.
        //! False in case of invalid input: no timestamp, not datagrams, inconsistent timestamps, etc.
        //!
        bool isValid() const { return !_invalid; }

        //!
        //! Structure containing the IAT analysis results.
        //!
        class TSDUCKDLL Status
        {
        public:
            Status() = default;                 //!< Default constructor.
            cn::microseconds mean_iat {};       //!< Mean inter-packet arrival time.
            cn::microseconds dev_iat {};        //!< Standard deviation of inter-packet arrival time.
            cn::microseconds min_iat {};        //!< Min inter-packet arrival time.
            cn::microseconds max_iat {};        //!< Max inter-packet arrival time.
            size_t           mean_packets = 0;  //!< Mean packet count per datagram.
            size_t           dev_packets = 0;   //!< Standard deviation of packet count per datagram.
            size_t           min_packets = 0;   //!< Min packet count per datagram.
            size_t           max_packets = 0;   //!< Max packet count per datagram.
            TimeSource       source = TimeSource::UNDEFINED;  //!< Time source.
        };

        //!
        //! Get the IAT since start or the last getStatusRestart().
        //! This is just a snapshot. The status will continue to evolve.
        //! @param [out] status Returned status.
        //! @return True on success, false if there is no IAT information (not started, no packet, not datagram, etc).
        //!
        bool getStatus(Status& status);

        //!
        //! Get the IAT since start or the last getStatusRestart().
        //! The status is reset and a new measurement period starts.
        //! @param [out] status Returned status.
        //! @return True on success, false if there is no IAT information (not started, no packet, not datagram, etc).
        //!
        bool getStatusRestart(Status& status);

    private:
        Report&    _report;
        bool       _started = false;
        bool       _invalid = false;
        PCR        _last_timestamp {};
        size_t     _packets_since_last = 0;
        TimeSource _source = TimeSource::UNDEFINED;
        SingleDataStatistics<size_t> _stats_packets {};
        SingleDataStatistics<cn::microseconds> _stats_iat {};
    };
}
