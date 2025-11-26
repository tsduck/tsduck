//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Experimental DVB-NIP (Native IP) analyzer.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsNIPAnalyzer.h"
#include "tsServiceDiscovery.h"
#include "tsMPEDemux.h"
#include "tsMPEPacket.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class NIPPlugin: public ProcessorPlugin, private MPEHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(NIPPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        NIPAnalyzerArgs  _opt_nip {};
        PID              _opt_pid = PID_NULL;
        UString          _opt_service {};

        // Plugin private fields.
        bool             _abort = false;             // Error, abort asap.
        bool             _wait_for_service = false;  // Wait for MPE service id to be identified.
        PID              _mpe_pid = PID_NULL;        // Actual MPE PID.
        ServiceDiscovery _service {duck, nullptr};   // Service containing the MPE PID.
        MPEDemux         _mpe_demux {duck, this};    // MPE demux to extract MPE datagrams.
        NIPAnalyzer      _nip_analyzer {duck};       // DVB-NIP analyzer.

        // Inherited methods.
        virtual void handleMPENewPID(MPEDemux&, const PMT&, PID) override;
        virtual void handleMPEPacket(MPEDemux&, const MPEPacket&) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"nip", ts::NIPPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::NIPPlugin::NIPPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"DVB-NIP (Native IP) analyzer", u"[options]")
{
    _opt_nip.defineArgs(*this);

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the MPE PID containing the DVB-NIP stream. "
         u"By default, if neither --pid nor --service is specified, use the first MPE PID which is found."
         u"Options --pid and --service are mutually exclusive.");

    option(u"service", 's', STRING);
    help(u"service", u"name-or-id",
         u"Specify the service containing the DVB-NIP stream in a MPE PID. "
         u"If the argument is an integer value (either decimal or hexadecimal), it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored. "
         u"By default, if neither --pid nor --service is specified, use the first MPE PID which is found."
         u"Options --pid and --service are mutually exclusive.");

}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::NIPPlugin::getOptions()
{
    // Get command line arguments
    bool ok = _opt_nip.loadArgs(duck, *this);
    getIntValue(_opt_pid, u"pid", PID_NULL);
    getValue(_opt_service, u"service");

    // Check parameter consistency.
    if (_opt_pid != PID_NULL && !_opt_service.empty()) {
        error(u"--pid and --service are mutually exclusive");
        ok = false;
    }

    return ok;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::NIPPlugin::start()
{
    _abort = false;
    _wait_for_service = false;
    _mpe_pid = _opt_pid;
    _service.clear();
    _mpe_demux.reset();

    if (!_nip_analyzer.reset(_opt_nip)) {
        return false;
    }

    if (_mpe_pid != PID_NULL) {
        // MPE PID already known.
        _mpe_demux.addPID(_mpe_pid);
    }
    else if (!_opt_service.empty()) {
        // MPE service is specified.
        _service.set(_opt_service);
        // Wait for service id if identified by name.
        _wait_for_service = !_service.hasId();
    }
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::NIPPlugin::stop()
{
    if (_opt_nip.summary) {
        _nip_analyzer.printSummary(std::cout);
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::NIPPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (_wait_for_service) {
        // Service id not yet found.
        _service.feedPacket(pkt);
        _wait_for_service = !_service.hasId();
    }
    else {
        // Feed the MPE demux.
        _mpe_demux.feedPacket(pkt);
    }
    return _abort ? TSP_END : TSP_OK;
}


//----------------------------------------------------------------------------
// Process new MPE PID.
//----------------------------------------------------------------------------

void ts::NIPPlugin::handleMPENewPID(MPEDemux& demux, const PMT& pmt, PID pid)
{
    debug(u"found new MPE PID %n, service %n", pid, pmt.service_id);

    // Check if this MPE PID is the one to monitor.
    if (_mpe_pid == PID_NULL && (!_service.hasId() || _service.hasId(pmt.service_id))) {
        verbose(u"using MPE PID %n, service %n", pid, pmt.service_id);
        _mpe_pid = pid;
        _mpe_demux.addPID(pid);
    }
}


//----------------------------------------------------------------------------
// Process a MPE packet.
//----------------------------------------------------------------------------

void ts::NIPPlugin::handleMPEPacket(MPEDemux& demux, const MPEPacket& mpe)
{
    const IPSocketAddress destination(mpe.destinationSocket());
    debug(u"MPE packet on PID %n, for address %s, %d bytes", mpe.sourcePID(), destination, mpe.datagramSize());

    if (!_abort && mpe.sourcePID() == _mpe_pid) {
        _nip_analyzer.feedPacket(mpe.sourceSocket(), mpe.destinationSocket(), mpe.udpMessage(), mpe.udpMessageSize());
    }
}
