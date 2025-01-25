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

#include <tsDuckContext.h>
#include <tsNullReport.h>
#include <tsPlugin.h>
#include <tsSectionDemux.h>
#include <tsTSP.h>
#include <tsTableHandlerInterface.h>

namespace ts {
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

                // todo: to_json

                std::string to_string() const
                {
                    if (is_ms) {
                        std::string maxStr;
                        if (this->max != LONG_MIN) {
                            maxStr = " MAX: " + std::format("{:.2f}", this->max / 1000000.0) + "ms";
                        }
                        std::string minStr;
                        if (this->min != LONG_MAX) {
                            minStr = " MIN: " + std::format("{:.2f}", this->min / 1000000.0) + "ms";
                        }

                        return minStr + maxStr + " CURR: " + std::format("{:.2f}", curr / 1000000.0) + "ms";
                    } else {
                        std::string maxStr;
                        if (this->max != LONG_MIN) {
                            maxStr = " MAX: " + std::to_string(this->max) + "ns";
                        }
                        std::string minStr;
                        if (this->min != LONG_MAX) {
                            minStr = " MIN: " + std::to_string(this->min) + "ns";
                        }
                        return minStr + maxStr + " CURR: " + std::to_string(curr) + "ns";
                    }
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
        bool         _has_cat = false;
        uint64_t     _currentTimestamp = INVALID_PTS;
        std::map<PID, std::shared_ptr<ServiceContext>> _services {};  ///< Services std::map<PMT_PID, ServiceContext>
        BitRate _bitrate = 0;
        PacketCounter _packetIndex = 0;

        void processPacket(ServiceContext& ctx, const TSPacket& pkt, const TSPacketMetadata& mdata) const;

    protected:
        // CALLBACKS
        void                            handleTable(SectionDemux& demux, const BinaryTable& table) override;
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

        //! Reset the table to defaults, clearing any counters.
        void reset();
    };
}  // namespace ts

#endif  //TSTR101ANALYZER_H
