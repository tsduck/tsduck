//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Display ISDB-T Information (IIP and 16-byte trailer, aka "dummy byte").
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsSignalizationDemux.h"
#include "tsISDBTInformation.h"
#include "tsISDBTInformationPacket.h"
#include "tsISDB.h"
#include "tsIntegerMap.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class ISDBInfoPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(ISDBInfoPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

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

TS_REGISTER_PROCESSOR_PLUGIN(u"isdbinfo", ts::ISDBInfoPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ISDBInfoPlugin::ISDBInfoPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract ISDB-T information from the stream", u"[options]")
{
    option(u"continuity", 'c');
    help(u"continuity", u"Check presence and continuity of the 'dummy byte' trailers and packet counters.");

    option(u"iip", 'i');
    help(u"iip", u"Dump all ISDB-T Information Packets (IIP).");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file",
         u"Specify the output text file. "
         u"By default, use tsp log messages for --continuity warnings and the standard output for other reports.");

    option(u"pid-iip", 'p', PIDVAL);
    help(u"pid-iip",
         u"Specify the PID carrying ISDB-T Information Packets (IIP). "
         u"The default IIP PID is " + UString::Format(u"%n.", PID_IIP));

    option(u"statistics", 's');
    help(u"statistics", u"Display final statistics of ISDB-T information.");

    option(u"trailers", 't');
    help(u"trailers", u"Dump the ISDB-T information in all 'dummy byte' trailers.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::ISDBInfoPlugin::getOptions()
{
    _check_continuity = present(u"continuity");
    _statistics = present(u"statistics");
    _dump_trailers = present(u"trailers");
    _dump_iip = present(u"iip");
    getIntValue(_pid_iip, u"pid-iip", PID_IIP);
    getPathValue(_output_name, u"output-file");
    return true;
}


//----------------------------------------------------------------------------
// Start method.
//----------------------------------------------------------------------------

bool ts::ISDBInfoPlugin::start()
{
    // Reset state.
    _has_output = false;
    _last_dummy = INVALID_PACKET_COUNTER;
    _last_tsp_counter = 0;
    _iip_count = 0;
    _frames_by_size.clear();
    _pids.clear();
    _services.clear();
    _demux.reset();
    _demux.setHandler(this);

    // Open output file.
    if (_output_name.empty()) {
        _output = &std::cout;
    }
    else {
        _output = &_output_stream;
        _output_stream.open(_output_name);
        if (!_output_stream) {
            error(u"cannot create file %s", _output_name);
            return false;
        }
    }

    // Assume that the stream is an ISDB one.
    duck.addStandards(Standards::ISDB);
    return true;
}


//----------------------------------------------------------------------------
// Stop method.
//----------------------------------------------------------------------------

bool ts::ISDBInfoPlugin::stop()
{
    // Final messages.
    missingTrailers();

    // Produce final statistics.
    if (_statistics) {

        startOutputSection();
        *_output << UString::Format(u"PID for ISDB-T Information Packets (IIP): %n", _pid_iip) << std::endl;
        *_output << UString::Format(u"IIP packets: %'d / %'d", _iip_count, tsp->pluginPackets()) << std::endl;
        if (!_frames_by_size.empty()) {
            *_output << "Frames sizes (packets): " << _frames_by_size.toStringKeys() << std::endl;
        }
        *_output << std::endl;

        // Compute packets per layer in the TS and per service.
        ISDBTLayerCounter ts_layers;
        for (const auto& it : _pids) {
            ts_layers.accumulate(it.second->pkt_per_layer);
        }
        if (!ts_layers.empty()) {
            startOutputSection();
            *_output << "ISDB-T Layers:" << std::endl;
            *_output << "  All layers in TS: " << ts_layers.toStringKeys(tsp->pluginPackets()) << std::endl;
            for (const auto& it : _services) {
                ISDBTLayerCounter layers;
                PacketCounter total = 0;
                for (PID pid = 0; pid < it.second->pids.size(); ++pid) {
                    if (it.second->pids.test(pid)) {
                        const PIDContext& pc(getPID(pid));
                        layers.accumulate(pc.pkt_per_layer);
                        total += pc.pkt_count;
                    }
                }
                *_output << UString::Format(u"  Service %n", it.first);
                if (!it.second->name.empty()) {
                    *_output << " (" << it.second->name << ")";
                }
                *_output << ": " << layers.toStringKeys(total) << std::endl;
            }
            *_output << std::endl;
        }
    }

    // Close output file.
    if (!_output_name.empty() && _output_stream.is_open()) {
        _output_stream.close();
    }
    return true;
}


//----------------------------------------------------------------------------
// Get/create PID and service contexts.
//----------------------------------------------------------------------------

ts::ISDBInfoPlugin::PIDContext& ts::ISDBInfoPlugin::getPID(PID pid)
{
    const auto it = _pids.find(pid);
    if (it != _pids.end()) {
        return *it->second;
    }
    else {
        return *(_pids[pid] = std::make_shared<PIDContext>(pid));
    }
}

ts::ISDBInfoPlugin::ServiceContext& ts::ISDBInfoPlugin::getService(uint16_t id)
{
    const auto it = _services.find(id);
    if (it != _services.end()) {
        return *it->second;
    }
    else {
        return *(_services[id] = std::make_shared<ServiceContext>(id));
    }
}


//----------------------------------------------------------------------------
// Detect and report a sequence of missing 'dummy byte' trailers before current packet.
//----------------------------------------------------------------------------

size_t ts::ISDBInfoPlugin::missingTrailers()
{
    const PacketCounter miss_start = _last_dummy == INVALID_PACKET_COUNTER ? 0 : _last_dummy + 1;
    const size_t miss_count = size_t(tsp->pluginPackets() - miss_start);
    if (_check_continuity && miss_count > 0) {
        reportWarning(u"packet %'d: missing %'d 'dummy byte' trailers", miss_start, miss_count);
    }
    return miss_count;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::ISDBInfoPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Pass packets in the signalization demux.
    _demux.feedPacket(pkt);

    // Collect PID characteristics.
    PIDContext& pc(getPID(pkt.getPID()));
    pc.pkt_count++;

    // Save characteristics from 16-byte trailer.
    const ISDBTInformation info(duck, pkt_data, false);
    if (info.is_valid) {

        // Number of packets with missing trailer, just before this one.
        const size_t miss_count = missingTrailers();

        // Detect TSP_counter discontinuities.
        if (_last_dummy != INVALID_PACKET_COUNTER) {
            // Expected TSP counter:
            size_t tsp_next = _last_tsp_counter + miss_count + 1;
            // Detect new frame.
            if (info.frame_indicator != _last_frame_indicator) {
                _frames_by_size[tsp_next]++;  // Record the size of the previous frame.
                tsp_next = 0;                 // Expected TSP counter at start of frame.
            }
            // Detect TSP counter discontinuities.
            if (_check_continuity && info.TSP_counter > tsp_next) {
                reportWarning(u"packet %'d: TSP_counter discontinuity, missing %'d packets", tsp->pluginPackets(), info.TSP_counter - tsp_next);
            }
        }

        // Track statistics.
        pc.trailer_count++;
        pc.pkt_per_layer[info.layer_indicator]++;
        _last_dummy = tsp->pluginPackets();
        _last_tsp_counter = info.TSP_counter;
        _last_frame_indicator = info.frame_indicator;

        // Dump 'dummy byte' trailers.
        if (_dump_trailers) {
            startOutputSection();
            *_output << UString::Format(u"Packet %'d ISDB-T Information:", tsp->pluginPackets()) << std::endl;
            info.display(duck, *_output, u"  ");
            *_output << std::endl;
        }
    }

    // Process IIP packets.
    if (pc.pid == _pid_iip) {
        if (_dump_iip) {
            ISDBTInformationPacket iip(duck, pkt, false);
            if (iip.is_valid) {
                startOutputSection();
                *_output << UString::Format(u"Packet %'d, IIP %'d:", tsp->pluginPackets(), _iip_count) << std::endl;
                iip.display(duck, *_output, u"  ");
                *_output << std::endl;
            }
            else {
                reportWarning(u"Packet %'d: invalid IIP packet", tsp->pluginPackets());
            }
        }
        _iip_count++;
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Handle potential changes in the service list.
//----------------------------------------------------------------------------

void ts::ISDBInfoPlugin::handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed)
{
    debug(u"got service %s", service);
    ServiceContext& svc(getService(service.getId()));

    // Copy service name the first time.
    if (service.hasName() && svc.name.empty()) {
        svc.name = service.getName();
    }

    // Record all PID's in the service.
    // TODO: track CAS PID's.
    if (pmt.isValid()) {
        for (const auto& stm : pmt.streams) {
            svc.pids.set(stm.first);
        }
    }
}
