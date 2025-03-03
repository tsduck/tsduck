//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Staz Modrzynski
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  TR 101-290 analyzer.
//
//----------------------------------------------------------------------------

#ifndef TSTR101ANALYZER_H
#define TSTR101ANALYZER_H

#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"

#include "tsDuckContext.h"
#include "tsNullReport.h"
#include "tsSectionDemux.h"
#include "tsTableHandlerInterface.h"
#include "tsjsonOutputArgs.h"
#include "tsjsonValue.h"

#include <functional>

namespace ts {

    //!
    //! Report options for the class TR101_290Analyzer.
    //! @ingroup libtsduck mpeg
    //!
    //! The default options are
    //! -\-show-report
    //!
    class TSDUCKDLL TR101_Options {
        TS_NOCOPY(TR101_Options);

    public:
        //!
        //! Constructor.
        //!
        TR101_Options() = default;

        json::OutputArgs json {};            //!< Options -\-json and -\-json-line

        bool show_report = true;               //!< Option -\-show-report

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);
    };

    //!
    //! A class which analyzes a complete transport stream and produces a ETSI TR 101 290 report.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TR101_290Analyzer final:
        TableHandlerInterface,
        SectionHandlerInterface,
        InvalidSectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TR101_290Analyzer);

        private:

        //! Helper tool to track the Min/Max of a value.
        struct TSDUCKDLL IntMinMax
        {
            bool is_ms = true;
            long count = 0;
            long min = LONG_MAX;
            long max = LONG_MIN;
            long curr = 0;

            UString to_string() const;
            void defineJson(json::Value& value) const;
            void pushNs(long val);
            void pushSysClockFreq(int64_t val);
            void clear();
        };

        //! An Indicator is a core component of a report.
        //! Each row in a TR 101-290 report is backed by this Indicator, and tracks information about the Indicator itself.
        class TSDUCKDLL Indicator {
            public:
            UString  name;
            bool     show_value;
            bool     enabled = false;
            uint64_t value_timeout; ///< how long to wait before the data is no longer valid.
            uint64_t prev_timeout; ///< The previous timeout to apply.
            uint64_t timeout_start;

        public:
            explicit Indicator(UString name, bool show_value, bool enabled=true, bool is_ms=true, uint64_t value_timeout = 5 * SYSTEM_CLOCK_FREQ);

            uint64_t prev_ts = INVALID_PCR; ///< The timestamp of the last element provided.
            IntMinMax minMax{}; ///< Value Min/Max
            bool in_timeout = false; /// < in a timeout state.
            int in_err_count = 0; ///< Number of times we faulted so far.

            //! This Indicator has some kind of timeout condition that has been met.
            //! Once the Indicator enters a timeout state, future calls to timeout will be a noop until the Indicator exits the timeout condition.
            //! This offers a debouncer to prevent a repeated identification of a timeout to report as multiple timeouts.
            //! @param [in] now The current time.
            //! @param [in] timeout When true, this Indicator has met its timeout condition.
            //! @return true if a timeout occurred and we should print an error message.
            bool timeout(uint64_t now, bool timeout);

            //! Compare how long since the last call to `resetTimeout`, and if it was more than max_val seconds ago, trigger a timeout.
            //! This will also reset and re-trigger every max_val seconds if the error is continuous.
            //! @param [in] now The timestamp of the last received packet in PCR units.
            //! @param [in] max_val Upper limit of the timeout for this condition. If the calculated timeout exceeds max_val, trigger a timeout.
            //! @return true if a timeout occurred and we should print an error message.
            bool timeoutAfter(uint64_t now, uint64_t max_val);

            //! Reset the current timer for timeouts to the current time.
            //! @param [in] now the timestamp that reset the timeout.
            void resetTimeout(uint64_t now);

            //! Called when there was a recent measurement of this Indicator.
            //! Internally, this clears any timeouts and updates the current error state.
            //! @param [in] now The timestamp of the measurement in PCR units.
            //! @param [in] in_error Did this measurement fail some kind of condition, triggering an error?
            //! @return true if an error occurred (in_error is true) and we should print an error message.
            bool update(uint64_t now, bool in_error);

            //! Some Indicators have a value associated with their measurements. This function can provide the last value reported.
            //! Often times this may be either a direct measurement (i.e. a timestamp), or the amount of time since this report was measured.
            //! This is functionally equivalent to the update command without a value, but records the current value for a future report.
            //! @param [in] now The timestamp of the measurement in PCR units.
            //! @param [in] in_error Did this measurement fail some kind of condition, triggering an error?
            //! @param [in] value The current value, PCR units. This value is only used when printing a report.
            //! @return true if an error occurred (in_error is true) and we should print an error message.
            bool update(uint64_t now, bool in_error, int64_t value);

            //! Clear the Indicator for a new print of the report.
            void clear();

            void setEnabled(bool enabled);
            bool isEnabled() const;

            bool isOutdated(uint64_t now) const;
        };

        class TSDUCKDLL ServiceContext
        {
            TS_NOBUILD_NOCOPY(ServiceContext);

        public:
            enum ServiceContextType
            {
                Table,
                Pmt,
                Pat,

                Nit,
                Sdt,
                Eit,
                Rst,
                Tdt,
                TableEnd,

                Assigned,
                Unassigned
            };

            ServiceContext(PID pid, ServiceContextType type);
            void clear();
            void setType(ServiceContextType assignment);

            PID                 pid;
            ServiceContextType  type;
            int32_t             pmt_service_id = -1;

            bool            last_repeat = false;
            bool            has_discontinuity = false;
            uint64_t        last_packet_ts = INVALID_PCR;
            uint64_t        last_pcr_ts = INVALID_PCR;
            uint64_t        last_pcr_val = INVALID_PCR;
            uint64_t        last_table_ts = INVALID_PCR;
            int             last_cc = -1;
            uint64_t       last_table = INVALID_PCR;
            uint64_t       last_table_actual = INVALID_PCR;
            uint64_t       last_table_other = INVALID_PCR;

            // Priority 1 Errors
            // Indicator TS_sync_loss;
            // Indicator Sync_byte_error;
            Indicator PAT_error;
            Indicator PAT_error_2;
            Indicator CC_error;
            Indicator PMT_error;
            Indicator PMT_error_2;
            Indicator PID_error;

            // Priority 2 Errors
            Indicator Transport_error;
            Indicator CRC_error;
            Indicator PCR_error;
            Indicator PCR_repetition_error;
            Indicator PCR_discontinuity_indicator_error;
            Indicator PCR_accuracy_error;
            Indicator PTS_error;
            Indicator CAT_error;

            // Priority 3 Errors
            Indicator NIT_error;
            Indicator NIT_actual_error;
            Indicator NIT_other_error;
            // Indicator SI_repetition_error; // todo: Repetition rate of SI tables outside of specified limits.
            // Indicator Buffer_error; // todo
            Indicator Unreferenced_PID;
            Indicator SDT_error;
            Indicator SDT_actual_error;
            Indicator SDT_other_error;
            Indicator EIT_error;
            Indicator EIT_actual_error;
            Indicator EIT_other_error;
            Indicator EIT_PF_error;
            Indicator RST_error;
            Indicator TDT_error;
            // Indicator Empty_buffer_error; // todo
            // Indicator Data_delay_error; // todo

            Indicator _EIT_actual_error_sec1;  // ONLY USED FOR TIMEOUTS. NOT FOR USE OUTSIDE OF API.
            Indicator _EIT_other_error_sec1; // ONLY USED FOR TIMEOUTS. NOT FOR USE OUTSIDE OF API.
            int32_t       last_pid;
        };

    private:
        DuckContext&  _duck;
        SectionDemux _demux {_duck, this, this};
        PacketCounter         lastCatIndex = INVALID_PACKET_COUNTER;
        uint64_t     currentTimestamp = INVALID_PTS;
        std::map<PID, std::shared_ptr<ServiceContext>> _services {};  ///< Services std::map<PMT_PID, ServiceContext>
        BitRate                                        bitrate = 0;
        int                                            tableIdx;

        template <class... Args>
        void info(const ServiceContext& ctx, const Indicator& ind, const UChar* fmt, Args&&... args) const
        {
            _duck.report().info(u"PID %d: %s: %s", ctx.pid, ind.name, UString::Format(fmt, std::forward<ArgMixIn>(args)...));
        }

        void processPacket(ServiceContext& ctx, const TSPacket& pkt, const TSPacketMetadata& mdata) const;
        void processTimeouts(ServiceContext& ctx);
        std::shared_ptr<ServiceContext> getService(PID pid);

    protected:
        // CALLBACKS
        void handleTable(SectionDemux& demux, const BinaryTable& table) override;
        void handleSection(SectionDemux& demux, const Section& section) override;
        void handleInvalidSection(SectionDemux& demux, const DemuxedData& data, Section::Status status) override;

    public:
        //!
        //! Default constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the analyzer.
        explicit TR101_290Analyzer(DuckContext& duck);

        //!
        //! The following method feeds the analyzer with a TS packet.
        //! The stream is analyzed by repeatedly calling feedPacket().
        //! @param [in] packet One TS packet from the stream.
        //! @param [in] mdata Associated metadata.
        //! @param [in] bitrate The current bitrate of the TS (typically returned by tsp->bitrate()).
        //!
        void feedPacket(const TSPacket& packet, const TSPacketMetadata& mdata, const BitRate& bitrate);

        //!
        //! General reporting method, using the specified options.
        //! @param [in,out] strm Output text stream.
        //! @param [in,out] opt Analysis options.
        //! @param [in,out] rep Where to report errors.
        //!
        void report(std::ostream& strm, int& opt, Report& rep = NULLREP) const;

        //!
        //! This methods displays a JSON report.
        //! @param [in,out] opt Analysis options.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string.
        //! @param [in,out] rep Where to report errors.
        void reportJSON(TR101_Options& opt, std::ostream& strm, const UString& title, Report& rep) const;

        //! Reset the table to defaults, clearing any counters.
        void reset();

    private:
        void print(const char16_t* name, Indicator ServiceContext::*indicator,  std::ostream& stm, const std::map<PID, std::shared_ptr<ServiceContext>>& services) const;
        static long count(Indicator ServiceContext::*indicator,  const std::map<ts::PID, std::shared_ptr<ServiceContext>>& services);
        void print_real(const char16_t* name, Indicator ServiceContext::*indicator, std::ostream& stm, const std::map<PID, std::shared_ptr<ServiceContext>>& services) const;
        static void json(const char16_t* name, Indicator ServiceContext::*indicator,  ts::json::Value& stm, json::Value& pids, const std::map<PID, std::shared_ptr<ServiceContext>>& _services);
        static void json_real(const char* name, Indicator ServiceContext::*indicator, ts::json::Value& stm, const std::map<PID,  std::shared_ptr<ServiceContext>>& services);
    };
}  // namespace ts

#endif  //TSTR101ANALYZER_H
