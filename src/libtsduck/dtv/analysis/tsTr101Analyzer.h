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

    class TR101_Options {

    public:
        json::OutputArgs json {};            //!< Options -\-json and -\-json-line
        bool show_report;

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

    class TR101_290Analyzer final:
        TableHandlerInterface,
        SectionHandlerInterface,
        InvalidSectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TR101_290Analyzer);

        public:

        struct IntMinMax
        {
            bool is_ms = true;
            long count = 0;
            long min = LONG_MAX;
            long max = LONG_MIN;
            long curr = 0;

            UString to_string() const;
            void defineJson(json::Value& value) const;
            void pushNs(long val);
            void pushSysClockFreq(long val);
            void clear();
        };

        class Indicator {
            public:
            UString name;
            bool show_value;
            uint64_t value_timeout; ///< how long to wait before the data is no longer valid.

        public:
            explicit Indicator(UString name, bool show_value, uint64_t value_timeout = 5 * SYSTEM_CLOCK_FREQ);

            uint64_t prev_ts = INVALID_PCR; ///< The timestamp of the last element provided.
            IntMinMax minMax{}; ///< Value Min/Max
            bool in_timeout = false; /// < in a timeout state.
            int in_err_count = 0; ///< Number of times we faulted so far.

            void timeout(bool timeout);
            void timeoutAfter(uint64_t now, uint64_t max_val);
            void update(uint64_t now, bool in_error);
            void update(uint64_t now, bool in_error, int64_t value);
            void clear();
        };

        class ServiceContext
        {
            TS_NOBUILD_NOCOPY(ServiceContext);

        public:
            enum ServiceContextType
            {
                Pmt,
                Pat,
                Assigned,
                Unassigned
            };

            ServiceContext(PID pid, ServiceContextType type);
            void clear();

            PID                pid;
            ServiceContextType type;
            int                pmt_service_id = -1;

            bool            last_repeat = false;
            bool            has_discontinuity = false;
            uint64_t        last_pts_ts = INVALID_PTS;
            uint64_t        last_packet_ts = INVALID_PCR;
            uint64_t        last_pcr_ts = INVALID_PCR;
            uint64_t        last_pcr_val = INVALID_PCR;
            PacketCounter   last_pcr_ctr = INVALID_PACKET_COUNTER;
            uint64_t        last_table_ts = INVALID_PCR;
            int             last_cc = -1;

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
        };

    private:
        DuckContext&  _duck;
        SectionDemux _demux {_duck, this, this};
        PacketCounter         _lastCatIndex = INVALID_PACKET_COUNTER;
        uint64_t     _currentTimestamp = INVALID_PTS;
        std::map<PID, std::shared_ptr<ServiceContext>> _services {};  ///< Services std::map<PMT_PID, ServiceContext>
        BitRate _bitrate = 0;
        PacketCounter _packetIndex = 0;

        void processPacket(ServiceContext& ctx, const TSPacket& pkt, const TSPacketMetadata& mdata) const;
        void processTimeouts(ServiceContext& ctx);

    protected:
        // CALLBACKS
        void handleTable(SectionDemux& demux, const BinaryTable& table) override;
        void handleSection(SectionDemux& demux, const Section& section) override;
        void handleInvalidSection(SectionDemux& demux, const DemuxedData& data, Section::Status status) override;

        std::shared_ptr<ServiceContext> getService(PID pid);

    public:
        explicit TR101_290Analyzer(DuckContext& duck);

        //!
        //! The following method feeds the analyzer with a TS packet.
        //! The stream is analyzed by repeatedly calling feedPacket().
        //! @param [in] packet One TS packet from the stream.
        //! @param [in] mdata Associated metadata.
        //! @param [in] bitrate The current bitrate of the TS (typically returned by tsp->bitrate()).
        //! @param packetIndex The packetIndex of the current TSPacket (typically returned by tsp->pluginPackets()).
        //!
        void feedPacket(const TSPacket& packet, const TSPacketMetadata& mdata, const BitRate& bitrate, PacketCounter packetIndex);

        //!
        //! General reporting method, using the specified options.
        //! @param [in,out] strm Output text stream.
        //! @param [in,out] opt Analysis options.
        //! @param [in,out] rep Where to report errors.
        //!
        void report(std::ostream& strm, int& opt, Report& rep = NULLREP) const;

        void reportJSON(TR101_Options& opt, std::ostream& stm, const UString& title, Report& rep) const;

        //! Reset the table to defaults, clearing any counters.
        void reset();
    };
}  // namespace ts

#endif  //TSTR101ANALYZER_H
