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
        struct ServiceContext
        {
            enum ServiceContextType
            {
                Pmt,
                Pat,
                Assigned,
                Unassigned
            };

            struct IntMinMax
            {
                bool is_ms = true;
                long  count = 0;
                long  min = LONG_MAX;
                long  max = LONG_MIN;
                long  curr = 0;

                UString to_string() const
                {
                    if (is_ms) {
                        UString maxStr;
                        if (this->max != LONG_MIN) {
                            UString fmt;
                            fmt.format(u"{:.2f}", double(this->max) / 1000000.0);
                            maxStr = u" MAX: " + fmt + u"ms";
                        }
                        UString minStr;
                        if (this->min != LONG_MAX) {
                            UString fmt;
                            fmt.format(u"{:.2f}", double(this->min) / 1000000.0);
                            minStr = u" MIN: " + fmt + u"ms";
                        }

                        UString fmt;
                        fmt.format(u"{:.2f}", double(curr) / 1000000.0);
                        return minStr + maxStr + u" CURR: " + fmt + u"ms";
                    } else {
                        UString maxStr;
                        if (this->max != LONG_MIN) {
                            maxStr = u" MAX: " + UString::Decimal(this->max) + u"ns";
                        }
                        UString minStr;
                        if (this->min != LONG_MAX) {
                            minStr = u" MIN: " + UString::Decimal(this->min) + u"ns";
                        }
                        return minStr + maxStr + u" CURR: " + UString::Decimal(curr) + u"ns";
                    }
                }

                void defineJson(json::Value& value) const
                {
                    if (this->max != LONG_MIN) {
                        value.add(u"max", double(this->max) / 1e9);
                    }
                    if (this->min != LONG_MAX) {
                        value.add(u"min", double(this->min) / 1e9);
                    }
                    value.add(u"curr", double(this->curr) / 1e9);
                }

                void pushNs(long val)
                {
                    curr = val;
                    if (val < min)
                        min = val;
                    if (val > max)
                        max = val;
                }

                void pushSysClockFreq(long val)
                {
                    const int ns = (int)(val * 1e9l / SYSTEM_CLOCK_FREQ);
                    pushNs(ns);
                }

                void clear()
                {
                    curr = 0;
                    min = LONG_MAX;
                    max = LONG_MIN;
                    count = 0;
                }
            };

            PID                _pid;
            ServiceContextType _type;
            int                _pmt_service_id = -1;

            bool          first_packet = true;
            uint64_t      last_pts_ts = INVALID_PTS;
            uint64_t      last_packet_ts = INVALID_PCR;
            uint64_t      last_pcr_ts = INVALID_PCR;
            PacketCounter last_pcr_ctr = INVALID_PACKET_COUNTER;
            uint64_t      _last_table_ts = INVALID_PCR;

            uint64_t last_pcr_val = INVALID_PCR;
            uint8_t  last_cc = 0;
            bool     has_pcr = false;

            // Prio: 1
            int sync_byte_error = 0;
            // int pat_error = 0;
            // int pat_error_2 = 0;
            IntMinMax pat_err {true}, pat_err2 {true};
            int       cc_error = 0;
            // int pmt_error = 0;
            // int pmt_error_2 = 0;
            IntMinMax pmt_err {true}, pmt_err2 {true};
            IntMinMax pid_err {true};
            // int pid_error = 0;

            // Prio: 2
            int       transport_error = 0;
            int       crc_error = 0;
            int       pcr_error = 0;
            IntMinMax pcr_repetition_err {true}, pcr_discontinuity_err {true}, pcr_accuracy_err {false}, pts_err {true};
            // int pcr_repetition_error = 0,pcr_repetition_error_min=0,pcr_repetition_error_max=0;
            // int pcr_discontinuity_error = 0, pcr_discontinuity_error_curr=0,pcr_discontinuity_error_min=0, pcr_discontinuity_error_max=0;
            // int pcr_accuracy_error = 0, pcr_accuracy_cur=0,pcr_accuracy_min = 0, pcr_accuracy_max = 0;
            // int pts_error = 0,pts_error_curr=0,pts_error_min=0,pts_error_max=0;
            int cat_error = 0;
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

    protected:
        // CALLBACKS
        void handleTable(SectionDemux& demux, const BinaryTable& table) override;
        void handleSection(SectionDemux& demux, const Section& section) override;
        void handleInvalidSection(SectionDemux& demux, const DemuxedData& data) override;

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
        void feedPacket(const TSPacket& packet, const TSPacketMetadata& mdata, BitRate bitrate, PacketCounter packetIndex);

        //!
        //! General reporting method, using the specified options.
        //! @param [in,out] strm Output text stream.
        //! @param [in,out] opt Analysis options.
        //! @param [in,out] rep Where to report errors.
        //!
        void report(std::ostream& strm, int& opt, Report& rep = NULLREP);

        void reportJSON(TR101_Options& opt, std::ostream& stm, const UString& title, Report& rep);

        //! Reset the table to defaults, clearing any counters.
        void reset();
    };
}  // namespace ts

#endif  //TSTR101ANALYZER_H
