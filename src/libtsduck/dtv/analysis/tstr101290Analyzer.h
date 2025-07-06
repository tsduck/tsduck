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
#include "tstr101290.h"
#include "tsTSPacket.h"
#include "tsSectionDemux.h"
#include "tsContinuityAnalyzer.h"

namespace ts {
    class PAT;
    class CAT;
    class PMT;
}

namespace ts::tr101290 {
    //!
    //! A class which analyzes a transport stream according to ETSI TR 101 290.
    //! @ingroup libtsduck mpeg
    //! @see ETSI TR 101 290 V1.4.1 (2020-06): Digital Video Broadcasting(DVB); Measurement guidelines for DVB systems.
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
    class TSDUCKDLL Analyzer:
        private SectionHandlerInterface,
        private TableHandlerInterface,
        private InvalidSectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(Analyzer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside this instance.
        //!
        explicit Analyzer(DuckContext& duck);

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
        //! Set the maximum packet interval in "user PID's" before declaring PID_error.
        //! This value currently applies to video and audio PID's only.
        //! @param [in] interval The maximum interval as a duration value.
        //!
        template <class Rep, class Period>
        void setMaxPIDInterval(cn::duration<Rep, Period> interval) { _max_pid_interval = cn::duration_cast<PCR>(interval); }

        //!
        //! Set the number of consecutive invalid TS sync bytes before declaring TS sync loss.
        //! @param [in] count When that number of consecutive TS packets have a corrupted sync byte
        //! (the initial 0x47 value), we declare a TS synchronization loss.
        //!
        void setTSSyncLostCount(size_t count) { _bad_sync_max = count; }

    private:
        // Some counters can be incremented for a number of reasons but we want to count
        // at most one error of each kind per packet. The errors could be set in feedPacket()
        // or in a handler. We use a CounterFlags that can be set many times during the
        // processing of one packet but we count at most one error per flag.

        // One such structure is maintained per PID.
        class TSDUCKDLL PIDContext
        {
        public:
            PIDContext() = default;
            bool     is_pmt = false;                 // This PID contains a PMT.
            bool     user_pid = false;               // This PID is subject to PID_error check.
            PIDClass type = PIDClass::UNDEFINED;     // Type of data in that PID.
            PCR      first_timestamp = PCR(-1);      // Timestamp of first packet in that PID.
            PCR      last_timestamp = PCR(-1);       // Timestamp of last packet in that PID.
            PCR      last_pts_timestamp = PCR(-1);   // Timestamp of last packet with a PTS in that PID.
            PCR      last_pcr_timestamp = PCR(-1);   // Timestamp of last packet with a PCR in that PID.
            uint64_t last_pcr_value = INVALID_PCR;   // Last PCR value in that PID.
            PCR      last_disc_timestamp = PCR(-1);  // Timestamp of last packet with a discontinuity_indicator in that PID.
            std::set<uint16_t> services {};          // Set of services which reference that PID.
        };

        // One such structure is maintained per TID/TIDext (XTID).
        class TSDUCKDLL XTIDContext
        {
        public:
            XTIDContext() = default;

            // Check if current timestamp is at an invalid distance from last_timestamp.
            bool invMin(PCR current, PCR min) const;
            bool invMax(PCR current, PCR max) const;

            PCR last_timestamp = PCR(-1);            // Timestamp of last packet of a section with that XTID.
            PCR last_time_01[2] {PCR(-1), PCR(-1)};  // Same as last_timestamp for sections #0 and #1 (when needed).
        };

        // Analyzer private data.
        DuckContext&       _duck;
        size_t             _bad_sync_max = DEFAULT_TS_SYNC_LOST;
        size_t             _bad_sync_count = 0;           // Last consecutive corrupted sync bytes.
        PCR                _last_timestamp = PCR(-1);     // Timestamp of last packet, negative means none.
        PCR                _current_timestamp = PCR(-1);  // Timestamp of current packet, negative means none.
        PCR                _last_nit_timestamp = PCR(-1); // Timestamp of last NIT section in NIT PID, regardless of network_id.
        Counters           _counters {};
        CounterFlags       _counters_flags {};
        SectionDemux       _demux {_duck, this, this};
        ContinuityAnalyzer _continuity {AllPIDs()};
        std::map<PID,PIDContext>   _pids {};
        std::map<XTID,XTIDContext> _xtids {};

        // These min / max intervals can be made configurable if necessary.
        PCR _max_pat_interval           = cn::duration_cast<PCR>(MAX_PAT_INTERVAL);
        PCR _max_pmt_interval           = cn::duration_cast<PCR>(MAX_PMT_INTERVAL);
        PCR _min_rst_interval           = cn::duration_cast<PCR>(MIN_RST_INTERVAL);
        PCR _max_nit_interval           = cn::duration_cast<PCR>(MAX_NIT_INTERVAL);
        PCR _min_nit_actual_interval    = cn::duration_cast<PCR>(MIN_NIT_ACTUAL_INTERVAL);
        PCR _max_nit_actual_interval    = cn::duration_cast<PCR>(MAX_NIT_ACTUAL_INTERVAL);
        PCR _max_nit_other_interval     = cn::duration_cast<PCR>(MAX_NIT_OTHER_INTERVAL);
        PCR _min_sdt_actual_interval    = cn::duration_cast<PCR>(MIN_SDT_ACTUAL_INTERVAL);
        PCR _max_sdt_actual_interval    = cn::duration_cast<PCR>(MAX_SDT_ACTUAL_INTERVAL);
        PCR _max_sdt_other_interval     = cn::duration_cast<PCR>(MAX_SDT_OTHER_INTERVAL);
        PCR _min_eitpf_actual_interval  = cn::duration_cast<PCR>(MIN_EIT_PF_ACTUAL_INTERVAL);
        PCR _max_eitpf_actual_interval  = cn::duration_cast<PCR>(MAX_EIT_PF_ACTUAL_INTERVAL);
        PCR _max_eitpf_other_interval   = cn::duration_cast<PCR>(MAX_EIT_PF_OTHER_INTERVAL);
        PCR _min_tdt_interval           = cn::duration_cast<PCR>(MIN_TDT_INTERVAL);
        PCR _max_tdt_interval           = cn::duration_cast<PCR>(MAX_TDT_INTERVAL);
        PCR _max_pts_interval           = cn::duration_cast<PCR>(MAX_PTS_INTERVAL);
        PCR _max_pcr_interval           = cn::duration_cast<PCR>(MAX_PCR_INTERVAL);
        PCR _max_pcr_difference         = cn::duration_cast<PCR>(MAX_PCR_DIFFERENCE);
        PCR _max_pid_reference_interval = cn::duration_cast<PCR>(MAX_PID_REFERENCE_INTERVAL);
        PCR _max_pid_interval           = cn::duration_cast<PCR>(DEFAULT_MAX_PID_INTERVAL);

        // Implementation of interfaces.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;
        virtual void handleInvalidSection(SectionDemux&, const DemuxedData&, Section::Status) override;

        // Table processing.
        void handlePAT(const PAT&, PID);
        void handleCAT(const CAT&, PID);
        void handlePMT(const PMT&, PID);

        // Declare ECM PID's in a descriptor list as part of a service.
        void searchECMPIDs(const DescriptorList& descs, uint16_t service_id);

        // Processing of detected errors.
        void logError(const UChar* reference, ErrorCounter error);
        void setError(const UChar* reference, ErrorCounter error);
        void addError(const UChar* reference, ErrorCounter error, Counters& counters);
    };
}
