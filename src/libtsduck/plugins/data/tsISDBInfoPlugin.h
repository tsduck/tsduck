//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to display ISDB-T Information (IIP and 16-byte trailer).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSignalizationDemux.h"
#include "tsISDB.h"
#include "tsIntegerMap.h"

namespace ts {
    //!
    //! Plugin to display ISDB-T Information (IIP and 16-byte trailer, aka "dummy byte").
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL ISDBInfoPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(ISDBInfoPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Context per PID and service.
        class PIDContext;
        using PIDContextPtr = std::shared_ptr<PIDContext>;
        using PIDMap = std::map<PID, PIDContextPtr>;

        class ServiceContext;
        using ServiceContextPtr = std::shared_ptr<ServiceContext>;
        using ServiceMap = std::map<uint16_t, ServiceContextPtr>;

        // Command line options:
        bool     _check_continuity = false;
        bool     _statistics = false;
        bool     _dump_trailers = false;
        bool     _dump_iip = false;
        PID      _pid_iip = PID_IIP;
        fs::path _output_name {};

        // Working data:
        std::ofstream             _output_stream {};
        std::ostream*             _output = nullptr;
        bool                      _has_output = false;   // Some output has been produced.
        PacketCounter             _iip_count = 0;        // Number of IIP packets.
        PacketCounter             _last_dummy = 0;       // Last packet counter with a 'dummy byte' trailer.
        uint16_t                  _last_tsp_counter = 0; // Last value of TSP_counter field in 'dummy byte' trailer.
        bool                      _last_frame_indicator = false; // Last value of frame_indicator field in 'dummy byte' trailer.
        IntegerMap<size_t,size_t> _frames_by_size {};    // Number of frames per size: key: frame size in pkts, value: number of frames.
        PIDMap                    _pids {};
        ServiceMap                _services {};
        SignalizationDemux        _demux {duck};

        // Context per PID.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            PIDContext(PID p) : pid(p) {}

            const PID         pid;
            PacketCounter     pkt_count = 0;      // Number of packets in the PID.
            PacketCounter     trailer_count = 0;  // Number of packets in the PID with a 16-byte trailer.
            ISDBTLayerCounter pkt_per_layer {};   // Number of packets per ISDB-T layer.
        };

        // Context per service.
        class ServiceContext
        {
            TS_NOBUILD_NOCOPY(ServiceContext);
        public:
            ServiceContext(uint16_t id) : service_id(id) {}

            const uint16_t service_id;
            UString        name {};    // Service name.
            PIDSet         pids {};    // Set of all PID's in this service.
        };

        // Implementation of SignalizationHandlerInterface
        virtual void handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed) override;

        // Get/create PID and service contexts.
        PIDContext& getPID(PID pid);
        ServiceContext& getService(uint16_t id);

        // Detect and report a sequence of missing 'dummy byte' trailers before current packet.
        // Return number of missing trailers.
        size_t missingTrailers();

        // Start a new section of output.
        void startOutputSection()
        {
            if (!_has_output) {
                *_output << std::endl;
                _has_output = true;
            }
        }

        // Report a warning either in the output file or the log system.
        template <class... Args>
        void reportWarning(const UChar* fmt, Args&&... args)
        {
            if (_output_stream.is_open()) {
                _output_stream << "warning: " << UString::Format(fmt, std::forward<ArgMixIn>(args)...) << std::endl;
                _has_output = false; // forces a new-line later
            }
            else {
                warning(fmt, std::forward<ArgMixIn>(args)...);
            }
        }
    };
}
