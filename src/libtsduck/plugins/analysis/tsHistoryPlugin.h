//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  History plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"
#include "tsStreamType.h"
#include "tsCodecType.h"
#include "tsTDT.h"

namespace ts {
    //!
    //! History plugin for tsp.
    //! Report a history of major events on the transport stream
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL HistoryPlugin: public ProcessorPlugin, private TableHandlerInterface, private SectionHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(HistoryPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one PID
        struct PIDContext
        {
            PIDContext() = default;               // Constructor
            PacketCounter pkt_count = 0;          // Number of packets on this PID
            PacketCounter first_pkt = 0;          // First packet in TS
            PacketCounter last_pkt = 0;           // Last packet in TS
            PacketCounter last_iframe_pkt = 0;    // Last packet containing an intra-frame
            uint16_t      service_id = 0;         // One service the PID belongs to
            uint8_t       stream_type = ST_NULL;  // Stream type as found in the PMT
            uint8_t       scrambling = 0;         // Last scrambling control value
            TID           last_tid = TID_NULL;    // Last table on this PID
            CodecType     codec = CodecType::UNDEFINED; // Audio/video codec
            std::optional<uint8_t> pes_strid {};  // PES stream id
        };

        // Command line options
        bool          _report_eit = false;        // Report EIT
        bool          _report_cas = false;        // Report CAS events
        bool          _report_iframe = false;     // Report intra-frames in video PID's.
        bool          _time_all = false;          // Report all TDT/TOT
        bool          _ignore_stream_id = false;  // Ignore stream_id modifications
        bool          _use_milliseconds = false;  // Report playback time instead of packet number
        PacketCounter _suspend_threshold = 0;     // Number of missing packets after which a PID is considered as suspended
        fs::path      _outfile_name {};           // Output file name
        UString       _tag {};                    // Message tag.

        // Working data
        std::ofstream _outfile {};                // User-specified output file
        PacketCounter _suspend_after = 0;         // Number of missing packets after which a PID is considered as suspended
        TDT           _last_tdt {};               // Last received TDT
        PacketCounter _last_tdt_pkt = 0;          // Packet# of last TDT
        bool          _last_tdt_reported = false; // Last TDT already reported
        bool          _bitrate_error = false;     // Already reported an "unknown bitrate" error
        SectionDemux  _demux {duck, this, this};  // Section filter
        std::map<PID,PIDContext> _cpids {};       // Description of each PID

        // Number of packets after which we report a warning if the bitrate is unknown.
        // This is one second of content at 10 Mb/s.
        static constexpr PacketCounter INITIAL_PACKET_THRESHOLD = 10'000'000 / PKT_SIZE_BITS;

        // Invoked by the demux.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;

        // Analyze a list of descriptors, looking for ECM PID's
        void analyzeCADescriptors(const DescriptorList& dlist, uint16_t service_id);

        // Report a history line
        void report(PacketCounter pkt, const UString& line);

        template <class... Args>
        void report(const UChar* fmt, Args&&... args)
        {
            report(tsp->pluginPackets(), UString::Format(fmt, std::forward<ArgMixIn>(args)...));
        }

        template <class... Args>
        void report(PacketCounter pkt, const UChar* fmt, Args&&... args)
        {
            report(pkt, UString::Format(fmt, std::forward<ArgMixIn>(args)...));
        }
    };
}
