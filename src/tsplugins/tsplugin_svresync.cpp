//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Resynchronize the clock of a service using the clock of another service
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsSignalizationDemux.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SVResyncPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SVResyncPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        UString          _target_service {};  // Target service to resync.
        UString          _ref_service {};     // Reference service.
        PID              _ref_pid = PID_NULL; // Reference PID.
        TSPacketLabelSet _set_labels {};      // Labels to set on modified packets

        // Working data:
        PID                _cur_ref_pid = PID_NULL;     // Current reference PID.
        uint64_t           _last_ref_pcr = INVALID_PCR; // Last PCR value in the reference PID.
        PacketCounter      _last_ref_packet = 0;        // Packet index for _last_ref_pcr.
        uint64_t           _delta_pts = 0;              // Value to add in target PTS and DTS (modulo PTS_DTS_SCALE).
        bool               _bitrate_error = false;      // PCR adjustment does not take into account packet distance between ref and target PCR.
        PacketCounter      _pcr_adjust_count = 0;       // Number of adjusted PCR.
        PacketCounter      _pts_adjust_count = 0;       // Number of adjusted PTS.
        PacketCounter      _dts_adjust_count = 0;       // Number of adjusted DTS.
        PID                _target_pcr_pid = PID_NULL;  // Main PCR PID of target service, just to detect change.
        PIDSet             _target_pids {};             // Components of the target service, where to adjust PCR, PTS, DTS.
        PIDSet             _modified_pids {};           // PID's with actually modified packets.
        SignalizationDemux _demux {duck, this};         // Analyze the transport stream.

        // Implementation of SignalizationHandlerInterface
        virtual void handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"svresync", ts::SVResyncPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SVResyncPlugin::SVResyncPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Resynchronize the clock of a service based on another service", u"[options] service")
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"",
         u"Specifies the target service to resynchronize to the reference clock. "
         u"If the argument is an integer value, it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored.");

    option(u"pid-reference", 'p', PIDVAL);
    help(u"pid-reference",
         u"Specifies the PID containing the reference PCR clock. "
         u"Exactly one of --service-reference and --pid-reference must be specified.");

    option(u"service-reference", 's', STRING);
    help(u"service-reference",
         u"Specifies the service containing the reference clock. "
         u"Only the PCR PID is used in this service. Other components are ignored. "
         u"If the argument is an integer value, it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored. "
         u"Exactly one of --service-reference and --pid-reference must be specified.");

    option(u"set-label", 0, INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"set-label", u"label1[-label2]",
         u"Set the specified labels on the modified PID's. "
         u"On each PID, the label is first set on the first modified packet, and then on all packets of the PID. "
         u"Several --set-label options may be specified.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::SVResyncPlugin::getOptions()
{
    duck.loadArgs(*this);
    getValue(_target_service, u"");
    getValue(_ref_service, u"service-reference");
    getIntValue(_ref_pid, u"pid-reference", PID_NULL);
    getIntValues(_set_labels, u"set-label");
    if (count(u"service-reference") + count(u"pid-reference") != 1) {
        error(u"exactly one of --service-reference and --pid-reference must be specified");
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SVResyncPlugin::start()
{
    _cur_ref_pid = _ref_pid; // PID-NULL is the reference is a service
    _last_ref_pcr = INVALID_PCR;
    _last_ref_packet = 0;
    _delta_pts = 0;
    _bitrate_error = false;
    _target_pcr_pid = PID_NULL;
    _target_pids.reset();
    _modified_pids.reset();

    _demux.reset();
    _demux.addFullFilters();
    _demux.addFilteredService(_target_service);
    if (!_ref_service.empty()) {
        _demux.addFilteredService(_ref_service);
    }

    _pcr_adjust_count = _pts_adjust_count = _dts_adjust_count = 0;
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::SVResyncPlugin::stop()
{
    verbose(u"adjusted %'d PCR, %'d PTS, %'d DTS", _pcr_adjust_count, _pts_adjust_count, _dts_adjust_count);
    return true;
}


//----------------------------------------------------------------------------
// Invoked when a service is updated.
//----------------------------------------------------------------------------

void ts::SVResyncPlugin::handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed)
{
    debug(u"handling updated services, TS id: %n, service: %n, \"%s\"", ts_id, service.getId(), service.getName());

    if (service.match(_target_service) && pmt.isValid()) {
        // Found the target service. Get all components. We will adjust time stamps here.
        _target_pids.reset();
        for (const auto& it : pmt.streams) {
            _target_pids.set(it.first);
        }
        _target_pids.set(pmt.pcr_pid);

        // If the PCR PID changed, reset our PCR adjustment.
        if (pmt.pcr_pid != _target_pcr_pid) {
            _delta_pts = 0;
            _target_pcr_pid = pmt.pcr_pid;
        }
    }
    else if (_ref_pid == PID_NULL && service.match(_ref_service) && pmt.isValid() && pmt.pcr_pid != PID_NULL && pmt.pcr_pid != _cur_ref_pid) {
        // Found the reference service and a new reference PCR PID.
        verbose(u"using reference PCR PID %n from service %n", pmt.pcr_pid, pmt.service_id);
        _cur_ref_pid = pmt.pcr_pid;
        _last_ref_pcr = INVALID_PCR;
        _last_ref_packet = 0;
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SVResyncPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Pass all packets to the demux.
    _demux.feedPacket(pkt);

    // Collect PCR in the reference PID.
    if (_cur_ref_pid != PID_NULL && pid == _cur_ref_pid && pkt.hasPCR()) {
        _last_ref_pcr = pkt.getPCR();
        _last_ref_packet = tsp->pluginPackets();
    }

    // Adjust time stamps in the target service (if we have a reference).
    if (_last_ref_pcr != INVALID_PCR && _target_pids.test(pid)) {

        // If the target packet contains a PCR, adjust the time difference between the two services.
        if (pkt.hasPCR()) {

            // Get the extrapolated reference PCR at current packets.
            const uint64_t pcr = pkt.getPCR();
            uint64_t ref_pcr = _last_ref_pcr;
            const BitRate bitrate = tsp->bitrate();
            if (bitrate != 0) {
                if (_bitrate_error) {
                    info(u"bitrate now known (%'d b/s), PCR accuracy restored", bitrate);
                    _bitrate_error = false;
                }
                ref_pcr += (((tsp->pluginPackets() - _last_ref_packet) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / bitrate).toInt();
            }
            else if (!_bitrate_error) {
                warning(u"unknown bitrate, distance between reference and target PCR not included in PCR adjustment");
                _bitrate_error = true;
            }

            // Compute difference between target and reference PCR.
            _delta_pts = ref_pcr >= pcr ?
                (ref_pcr - pcr) / SYSTEM_CLOCK_SUBFACTOR :
                PTS_DTS_SCALE - (pcr - ref_pcr) / SYSTEM_CLOCK_SUBFACTOR;
            debug(u"new delta PTS/DTS: 0x%09X (%'<d)", _delta_pts);

            // Replace PCR with extrapolated reference PCR.
            pkt.setPCR(ref_pcr);
            _pcr_adjust_count++;
            _modified_pids.set(pid);
        }

        // Adjust PTS and DTS.
        if (pkt.hasPTS()) {
            pkt.setPTS((pkt.getPTS() + _delta_pts) % PTS_DTS_SCALE);
            _pts_adjust_count++;
            _modified_pids.set(pid);
        }
        if (pkt.hasDTS()) {
            pkt.setDTS((pkt.getDTS() + _delta_pts) % PTS_DTS_SCALE);
            _dts_adjust_count++;
            _modified_pids.set(pid);
        }
    }

    // Set label on modified PID's.
    if (_set_labels.any() && _modified_pids.test(pid)) {
        pkt_data.setLabels(_set_labels);
    }
    return TSP_OK;
}
