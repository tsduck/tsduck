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
#include "tsTSPacket.h"
#include "tsSectionDemux.h"
#include "tsContinuityAnalyzer.h"

namespace ts {

    class PAT;
    class CAT;
    class PMT;

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
    class TSDUCKDLL TR101290Analyzer:
        private SectionHandlerInterface,
        private TableHandlerInterface,
        private InvalidSectionHandlerInterface
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
            size_t TS_sync_loss = 0;            //!< No 1.1 (maybe unreliable, depending on input plugin)
            size_t Sync_byte_error = 0;         //!< No 1.2 (maybe unreliable, depending on input plugin)
            size_t PAT_error = 0;               //!< No 1.3
            size_t PAT_error_2 = 0;             //!< No 1.3.a
            size_t Continuity_count_error = 0;  //!< No 1.4
            size_t PMT_error = 0;               //!< No 1.5
            size_t PMT_error_2 = 0;             //!< No 1.5.a
            size_t PID_error = 0;               //!< No 1.6

            // Section 5.2.2 - Second priority: recommended for continuous or periodic monitoring
            size_t Transport_error = 0;         //!< No 2.1
            size_t CRC_error = 0;               //!< No 2.2
            size_t CRC_error_2 = 0;             //!< CRC error in all other cases than CRC_error.
            size_t PCR_error = 0;               //!< No 2.3 (TODO)
            size_t PCR_repetition_error = 0;    //!< No 2.3.a (TODO)
            size_t PCR_discontinuity_indicator_error = 0; //!< No 2.3b (TODO)
            size_t PCR_accuracy_error = 0;      //!< No 2.4 (TODO)
            size_t PTS_error = 0;               //!< No 2.5
            size_t CAT_error = 0;               //!< No 2.6

            // Section 5.2.3 - Third priority: application dependant monitoring
            size_t NIT_error = 0;               //!< No 3.1
            size_t NIT_actual_error = 0;        //!< No 3.1.a
            size_t NIT_other_error = 0;         //!< No 3.1.b
            size_t SI_repetition_error = 0;     //!< No 3.2 (TODO)
            size_t Buffer_error = 0;            //!< No 3.3 (unimplemented)
            size_t Unreferenced_PID = 0;        //!< No 3.4
            size_t SDT_error = 0;               //!< No 3.5
            size_t SDT_actual_error = 0;        //!< No 3.5.a
            size_t SDT_other_error = 0;         //!< No 3.5.b
            size_t EIT_error = 0;               //!< No 3.6
            size_t EIT_actual_error = 0;        //!< No 3.6.a
            size_t EIT_other_error = 0;         //!< No 3.6.b
            size_t EIT_PF_error = 0;            //!< No 3.6.c
            size_t RST_error = 0;               //!< No 3.7
            size_t TDT_error = 0;               //!< No 3.8
            size_t Empty_buffer_error = 0;      //!< No 3.9 (unimplemented)
            size_t Data_delay_error = 0;        //!< No 3.10 (unimplemented)

            // Informational, not in ETSI TR 101 290.
            size_t packet_count = 0;            //!< Number of TS packets during measurement interval.

            //! Pseudo-severity for informational (non-error) data.
            static constexpr int INFO_SEVERITY = 4;
        };

        //!
        //! Description of one counter value in a Counters instance.
        //!
        class TSDUCKDLL CounterDescription
        {
        public:
            int                severity;  //!< Severity: 1, 2, 3, same as in ETSI TR 101 290.
            size_t Counters::* counter;   //!< Pointer to member inside Counters.
            UString            name;      //!< Counter name, same as in ETSI TR 101 290.
        };

        //!
        //! Get a list of descriptions for all counters in a Counters instance.
        //! @return A constant reference to a vector of CounterDescription.
        //!
        static const std::vector<CounterDescription>& CounterDescriptions();

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
        //! Maximum interval between two PAT.
        //!
        static constexpr cn::milliseconds MAX_PAT_INTERVAL = cn::milliseconds(500);

        //!
        //! Maximum interval between two PMT.
        //!
        static constexpr cn::milliseconds MAX_PMT_INTERVAL = cn::milliseconds(500);

        //!
        //! Minimum interval between two RST.
        //!
        static constexpr cn::milliseconds MIN_RST_INTERVAL = cn::milliseconds(25);

        //!
        //! Maximum interval between two NIT sections, regardless of type.
        //!
        static constexpr cn::seconds MAX_NIT_INTERVAL = cn::seconds(10);

        //!
        //! Minimum interval between two NIT Actual.
        //!
        static constexpr cn::milliseconds MIN_NIT_ACTUAL_INTERVAL = cn::milliseconds(25);

        //!
        //! Maximum interval between two NIT Actual.
        //!
        static constexpr cn::seconds MAX_NIT_ACTUAL_INTERVAL = cn::seconds(10);

        //!
        //! Maximum interval between two NIT Other.
        //!
        static constexpr cn::seconds MAX_NIT_OTHER_INTERVAL = cn::seconds(10);

        //!
        //! Minimum interval between two SDT Actual.
        //!
        static constexpr cn::milliseconds MIN_SDT_ACTUAL_INTERVAL = cn::milliseconds(25);

        //!
        //! Maximum interval between two SDT Actual.
        //!
        static constexpr cn::seconds MAX_SDT_ACTUAL_INTERVAL = cn::seconds(2);

        //!
        //! Maximum interval between two SDT Other.
        //!
        static constexpr cn::seconds MAX_SDT_OTHER_INTERVAL = cn::seconds(10);

        //!
        //! Minimum interval between two EIT p/f Actual.
        //!
        static constexpr cn::milliseconds MIN_EIT_PF_ACTUAL_INTERVAL = cn::milliseconds(25);

        //!
        //! Maximum interval between two EIT p/f Actual.
        //!
        static constexpr cn::seconds MAX_EIT_PF_ACTUAL_INTERVAL = cn::seconds(2);

        //!
        //! Maximum interval between two EIT p/f Other.
        //!
        static constexpr cn::seconds MAX_EIT_PF_OTHER_INTERVAL = cn::seconds(10);

        //!
        //! Minimum interval between two TDT.
        //!
        static constexpr cn::milliseconds MIN_TDT_INTERVAL = cn::milliseconds(25);

        //!
        //! Maximum interval between two TDT.
        //!
        static constexpr cn::seconds MAX_TDT_INTERVAL = cn::seconds(30);

        //!
        //! Maximum interval between two PTS in the same PID.
        //!
        static constexpr cn::milliseconds MAX_PTS_INTERVAL = cn::milliseconds(700);

        //!
        //! Maximum interval between the first packet of a PID and the time it is referenced.
        //! When jumping into a transport stream, we get audio, video, etc. packets possibly
        //! before the corresponding PMT. The PID is initially unreferenced but we need to
        //! find the reference (in the PMT) within that interval.
        //!
        static constexpr cn::milliseconds MAX_PID_REFERENCE_INTERVAL = cn::milliseconds(500);

        //!
        //! Default maximum packet interval in "user PID's" as defined by PID_error.
        //!
        static constexpr cn::seconds DEFAULT_MAX_PID_INTERVAL = cn::seconds(5);

        //!
        //! Set the maximum packet interval in "user PID's" before declaring PID_error.
        //! This value currently applies to video and audio PID's only.
        //! @param [in] interval The maximum interval as a duration value.
        //!
        template <class Rep, class Period>
        void setMaxPIDInterval(cn::duration<Rep, Period> interval) { _max_pid_interval = cn::duration_cast<PCR>(interval); }

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
        // Some counters can be incremented for a number of reasons but we want to count
        // at most one error of each kind per packet. The errors could be set in feedPacket()
        // or in a handler. We use a structure with error flags that can be set many times
        // during the processing one packet but we count at most one error per flag.
        class TSDUCKDLL CounterFlags
        {
        public:
            CounterFlags() = default;
            void clear() { *this = CounterFlags(); }
            void update(Counters&);

            bool PAT_error = false;
            bool PAT_error_2 = false;
            bool PMT_error = false;
            bool PMT_error_2 = false;
            bool PID_error = false;
            bool PTS_error = false;
            bool CAT_error = false;
            bool NIT_error = false;
            bool NIT_actual_error = false;
            bool NIT_other_error = false;
            bool CRC_error = false;
            bool CRC_error_2 = false;
            bool SDT_error = false;
            bool SDT_actual_error = false;
            bool SDT_other_error = false;
            bool EIT_error = false;
            bool EIT_actual_error = false;
            bool EIT_other_error = false;
            bool EIT_PF_error = false;
            bool RST_error = false;
            bool TDT_error = false;
        };

        // One such structure is maintained per PID.
        class TSDUCKDLL PIDContext
        {
        public:
            PIDContext() = default;
            bool     is_pmt = false;                // This PID contains a PMT.
            bool     user_pid = false;              // This PID is subject to PID_error check.
            PIDClass type = PIDClass::UNDEFINED;    // Type of data in that PID.
            PCR      first_timestamp = PCR(-1);     // Timestamp of first packet in that PID.
            PCR      last_timestamp = PCR(-1);      // Timestamp of last packet in that PID.
            PCR      last_pts_timestamp = PCR(-1);  // Timestamp of last packet with a PTS in that PID.
            PCR      last_pcr_timestamp = PCR(-1);  // Timestamp of last packet with a PCR in that PID.
            uint64_t last_pcr_value = INVALID_PCR;  // Last PCR value in that PID.
            std::set<uint16_t> services {};         // Set of services which reference that PID.
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

        // TR101290Analyzer private data.
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
        void logError(const UChar* reference, const char* error);
        void setError(const UChar* reference, const char* error, bool CounterFlags::* field);
        void addError(const UChar* reference, const char* error, size_t Counters::* field, Counters& block);
    };
}
