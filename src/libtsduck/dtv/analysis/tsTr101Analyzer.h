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
#include <tsSectionDemux.h>
#include <tsTSP.h>
#include <tsTableHandlerInterface.h>

namespace ts {

    class TR101_290Analyzer final:
      TableHandlerInterface {
      private:
      TSP* _tsp;
      DuckContext  _duck;
      SectionDemux _demux {_duck, this, nullptr};
      bool         _has_cat = false;

    public:
      struct ServiceContext {
        enum ServiceContextType {
          Pmt,
          Pat,
          Assigned,
          Unassigned
        };

        struct IntMinMax {
          bool is_ms;
          int count;
          int min;
          int max;
          int curr;

          std::string to_string() const {
            if (is_ms) {
              return "MIN: " + std::format("{:.2}", min/1000000.0) + "ms MAX: " + std::format("{:.2}", max/1000000.0) + "ms curr: " + std::format("{:.2}", curr/1000000.0) + "ms";
            } else {
              return "MIN: " + std::to_string(min) + "ns MAX: " + std::to_string(max) + "ns curr: " + std::to_string(curr) + "ns";
            }
          }

          void pushNs(int val)
          {
            curr = val;
            if (val < min)
              min = val;
            if (val > max)
              max = val;
          }

          void pushSysClockFreq(int64_t val)
          {
              pushNs(val * 1e9 / SYSTEM_CLOCK_FREQ);
          }

          void clear()
          {
            curr = 0;
            min = INT_MAX;
            max = INT_MIN;
            count = 0;
          }
        };

        int _pid;
        ServiceContextType _type;
        int _pmt_service_id;

        bool     first_packet = true;
        uint64_t last_pts_ts = INVALID_PTS;
        uint64_t last_packet_ts;
        uint64_t last_pcr_ts;
        PacketCounter last_pcr_ctr;

        uint64_t last_pcr_val = INVALID_PCR;
        uint8_t last_cc = 0;
        bool has_pcr = false;

        // Prio: 1
        int sync_byte_error = 0;
        // int pat_error = 0;
        // int pat_error_2 = 0;
        IntMinMax pat_err{true}, pat_err2{true};
        int cc_error = 0;
        // int pmt_error = 0;
        // int pmt_error_2 = 0;
        IntMinMax pmt_err{true}, pmt_err2{true};
        IntMinMax pid_err{true};
        // int pid_error = 0;

        // Prio: 2
        int transport_error = 0;
        int crc_error = 0;
        int pcr_error = 0;
        IntMinMax pcr_repetition_err{true}, pcr_discontinuity_err{true}, pcr_accuracy_err{false}, pts_err{true};
        // int pcr_repetition_error = 0,pcr_repetition_error_min=0,pcr_repetition_error_max=0;
        // int pcr_discontinuity_error = 0, pcr_discontinuity_error_curr=0,pcr_discontinuity_error_min=0, pcr_discontinuity_error_max=0;
        // int pcr_accuracy_error = 0, pcr_accuracy_cur=0,pcr_accuracy_min = 0, pcr_accuracy_max = 0;
        // int pts_error = 0,pts_error_curr=0,pts_error_min=0,pts_error_max=0;
        int cat_error = 0;
      };

      std::map<int, std::shared_ptr<ServiceContext>> _services; ///< Services std::map<PMT_PID, ServiceContext>

    protected:
      void handleTable(SectionDemux &demux, const BinaryTable &table) override;
      void processPacket(ServiceContext& ctx, const TSPacket& pkt, const TSPacketMetadata& mdata) const;
      std::shared_ptr<ServiceContext> getService(PID pid);

    public:
      explicit TR101_290Analyzer(TSP *tsp);

      //!
      //! The following method feeds the analyzer with a TS packet.
      //! The stream is analyzed by repeatedly calling feedPacket().
      //! @param [in] packet One TS packet from the stream.
      //! @param [in] mdata Associated metadata.
      //!
      void feedPacket(const TSPacket& packet, const TSPacketMetadata& mdata);

      //!
      //! General reporting method, using the specified options.
      //! @param [in,out] strm Output text stream.
      //! @param [in,out] opt Analysis options.
      //! @param [in,out] rep Where to report errors.
      //!
      void report(std::ostream& strm, int& opt, Report& rep = NULLREP);
      void reset();
    };
}

#endif //TSTR101ANALYZER_H
