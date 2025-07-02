//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A class which analyzes a transport stream according to ETSI TR 101 290.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTableHandlerInterface.h"
#include "tsSectionHandlerInterface.h"
#include "tsTSPacket.h"
#include "tsSectionDemux.h"
#include "tsContinuityAnalyzer.h"

namespace ts {
    //!
    //! A class which analyzes a transport stream according to ETSI TR 101 290.
    //! @ingroup libtsduck mpeg
    //! @see ETSI TR 101 290 V1.4.1 (2020-06): Digital Video Broadcasting(DVB); Measurement guidelines for DVB systems.
    //!
    //! The document ETSI TR 101 290 defines measurement criteria for MPEG/DVB transport streams.
    //! It is used by TV operators to monitor the stability of their network. It also used as a
    //! commonly accepted criteria of quality for broadcast networks.
    //!
    //! Limitations
    //! -----------
    //! Although this is not clearly explained that way, ETSI TR 101 290 defines two distinct classes
    //! of measurements: physical transport errors and transport stream logical errors. As a software
    //! tool, TSDuck can only detect transport stream logical errors. As an example, section 4 of ETSI
    //! TR 101 290 says: "Most of the parameters can be measured with standard equipment such as spectrum
    //! analysers or constellation analysers. Other parameters are defined in a new way as a request to
    //! test and measurement equipment manufacturers to integrate this functionality in their products."
    //! It is obvious that TSDuck cannot detect physical transport errors that require "spectrum analysers
    //! or constellation analysers". Therefore, this class implements only a subset of ETSI TR 101 290.
    //!
    //! The "TS_sync_loss" and "Sync_byte_error" indicators are inaccessible in most cases. It depends
    //! on the input plugin. Many plugins, such as "dvb" or all IP-based plugins ("ip", "srt", "rist", etc.)
    //! use the 0x47 sync byte to resynchronize in the data stream or to locate TS packets in datagrams.
    //! Therefore, with these input plugins, corrupted sync bytes are filtered upstream and never reach
    //! the analyzer classes.
    //!
    class TSDUCKDLL TR101290Analyzer: private SectionHandlerInterface, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TR101290Analyzer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside this instance.
        //!
        explicit TR101290Analyzer(DuckContext& duck);

        //!
        //! Reset the analyzer.
        //!
        void reset();

        //!
        //! The following method feeds the analyzer with a TS packet.
        //! @param [in] timestamp A timestamp for the packet, in PCR units.
        //! There is no specific relation with the PCR values in the stream.
        //! This must be a monotonic clock which never wraps (unlike 33-bits PCR value which wrap after 26 hours).
        //! @param [in] pkt A TS packet.
        //!
        void feedPacket(const PCR& timestamp, const TSPacket& pkt);

        //!
        //! Aggregate of error counters as defined in ETSI TR 101 290.
        //! The field names are the same as the counter names in ETSI TR 101 290.
        //!
        class TSDUCKDLL Counters
        {
        public:
            //!
            //! Default contructor.
            //!
            Counters() = default;

            //!
            //! Reset all counters.
            //!
            void clear() { *this = Counters(); }

            //!
            //! Get the total number of errors.
            //! @return The total number of errors. This can be less than the sum of all fields because
            //! an error can be included in several counters.
            //!
            size_t errorCount() const;

            // Section 5.2.1 - First priority: necessary for de-codability (basic monitoring)
            size_t TS_sync_loss = 0;            //!< No 1.1 (mostly unreliable)
            size_t Sync_byte_error = 0;         //!< No 1.2 (mostly unreliable)
            size_t PAT_error = 0;               //!< No 1.3 (TODO)
            size_t PAT_error_2 = 0;             //!< No 1.3.a (TODO)
            size_t Continuity_count_error = 0;  //!< No 1.4
            size_t PMT_error = 0;               //!< No 1.5 (TODO)
            size_t PMT_error_2 = 0;             //!< No 1.5.a (TODO)
            size_t PID_error = 0;               //!< No 1.6 (TODO)

            // Section 5.2.2 - Second priority: recommended for continuous or periodic monitoring
            size_t Transport_error = 0;         //!< No 2.1 (TODO)
            size_t CRC_error = 0;               //!< No 2.2 (TODO)
            size_t PCR_error = 0;               //!< No 2.3 (TODO)
            size_t PCR_repetition_error = 0;    //!< No 2.3a (TODO)
            size_t PCR_discontinuity_indicator_error = 0; //!< No 2.3b (TODO)
            size_t PCR_accuracy_error = 0;      //!< No 2.4 (TODO)
            size_t PTS_error = 0;               //!< No 2.5 (TODO)
            size_t CAT_error = 0;               //!< No 2.6 (TODO)

            // Section 5.2.3 - Third priority: application dependant monitoring
            size_t NIT_error = 0;               //!< No 3.1 (TODO)
            size_t NIT_actual_error = 0;        //!< No 3.1a (TODO)
            size_t NIT_other_error = 0;         //!< No 3.1b (TODO)
            size_t SI_repetition_error = 0;     //!< No 3.2 (TODO)
            size_t Buffer_error = 0;            //!< No 3.3 (unimplemented)
            size_t Unreferenced_PID = 0;        //!< No 3.4 (TODO)
            size_t SDT_error = 0;               //!< No 3.5 (TODO)
            size_t SDT_actual_error = 0;        //!< No 3.5a (TODO)
            size_t SDT_other_error = 0;         //!< No 3.5b (TODO)
            size_t EIT_error = 0;               //!< No 3.6 (TODO)
            size_t EIT_actual_error = 0;        //!< No 3.6a (TODO)
            size_t EIT_other_error = 0;         //!< No 3.6b (TODO)
            size_t EIT_PF_error = 0;            //!< No 3.6c (TODO)
            size_t RST_error = 0;               //!< No 3.7 (TODO)
            size_t TDT_error = 0;               //!< No 3.8 (TODO)
            size_t Empty_buffer_error = 0;      //!< No 3.9 (unimplemented)
            size_t Data_delay_error = 0;        //!< No 3.10 (unimplemented)
        };

        //!
        //! Get the error counters since start or the last getCountersRestart().
        //! This is just a snapshot. Counters will continue to increment.
        //! @param [out] counters Returned error counters.
        //!
        void getCounters(Counters& counters);

        //!
        //! Get and restart the error counters since start or the last getCountersRestart().
        //! Error counters are reset and a new measurement period starts.
        //! @param [out] counters Returned error counters.
        //!
        void getCountersRestart(Counters& counters);

        //!
        //! Default number of consecutive invalid TS sync bytes before declaring TS sync loss.
        //!
        static constexpr size_t DEFAULT_TS_SYNC_LOST = 5;

        //!
        //! Set the number of consecutive invalid TS sync bytes before declaring TS sync loss.
        //! @param [in] count When that number of consecutive TS packets have a corrupted sync byte
        //! (the initial 0x47 value), we declare a TS synchronization loss.
        //! 
        void setTSSyncLostCount(size_t count) { _bad_sync_max = count; }

    private:
        DuckContext&       _duck;
        PacketCounter      _current_pkt = 0;        // Index of current packet in stream.
        size_t             _bad_sync_count = 0;     // Last consecutive corrupted sync bytes.
        size_t             _bad_sync_max = DEFAULT_TS_SYNC_LOST;
        PCR                _last_pcr = PCR(-1);     // PCR of last packet, negative means none.
        PCR                _current_pcr = PCR(-1);  // PCR of current packet, negative means none.
        Counters           _counters {};
        SectionDemux       _demux {_duck, this, this};
        ContinuityAnalyzer _continuity {AllPIDs()};

        // Implementation of interfaces.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;
        virtual void handleSection(SectionDemux& demux, const Section& section) override;
    };
}
