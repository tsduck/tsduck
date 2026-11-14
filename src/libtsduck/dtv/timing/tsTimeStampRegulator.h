//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Regulate execution speed based on packet input timestamps.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractRegulator.h"

namespace ts {
    //!
    //! Regulate execution speed based on packet input timestamps.
    //! This class is only useful when the input timestamps come from stored media,
    //! for instance a PCAP or M2TS file. If the timestamps come from a real-time
    //! source (e.g., IP, RTP, SRT), the stream is already regulated.
    //! @ingroup libtsduck mpeg
    //! @see BitRateRegulator
    //! @see PCRRegulator
    //!
    class TSDUCKDLL TimeStampRegulator: public AbstractRegulator
    {
        TS_NOBUILD_NOCOPY(TimeStampRegulator);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] log_level Severity level for information messages.
        //!
        explicit TimeStampRegulator(Report* report, int log_level = Severity::Verbose) : AbstractRegulator(report, log_level) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] log_level Severity level for information messages.
        //!
        explicit TimeStampRegulator(ReporterBase* delegate, int log_level = Severity::Verbose) : AbstractRegulator(delegate, log_level) {}

        //!
        //! Destructor.
        //!
        virtual ~TimeStampRegulator() override;

        // Inherited methods.
        virtual bool reset() override;
        virtual void regulate(TSPacket& pkt, TSPacketMetadata& mdata, const BitRate& current_bitrate) override;

    private:
        PacketCounter  _burst_pkt_cnt = 0;     // Number of packets in current burst
        bool           _started = false;       // First packet found, regulation started.
        bool           _no_timestamp = false;  // At least one packet has no timestamp.
        PCR            _first_timestamp {};    // First timestamp value.
        PCR            _last_timestamp {};     // Last timestamp value.
        monotonic_time _first_clock {};        // System time at first timestamp.
        monotonic_time _last_clock {};         // System time at last wait
    };
}
