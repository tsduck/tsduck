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

//
// This is an experimental plugin to get more familiar with DVB-NIP.
// Currently, all required declaractions are in this file. They will be moved
// to more appropriate modules in libtsduck when ready.
//
// Relevant standards:
// - IETF RFC 5651: "Layered Coding Transport (LCT) Building Block", October 2009
// - IETF RFC 5775: "Asynchronous Layered Coding (ALC) Protocol Instantiation", April 2010
// - IETF RFC 3926: "FLUTE - File Delivery over Unidirectional Transport", October 2004 (FLUTE v1)
// - ETSI TS 103 876 V1.1.1 (2024-09), Digital Video Broadcasting (DVB); Native IP Broadcasting
//
// FLUTE v2 is defined in RFC 6726. It is not backwards compatible with FLUTE v1.
// DVB-NIP uses FLUTE v1.
//
// DVB-NIP can be carried over GSE or MPE. TSDuck can only analyze DVB-NIP over MPE
// because MPE is encapsulated into TS while GSE is native to DVB-S2 or DVB-T2.
// A DVB-NIP stream is encapsulated into one single MPE stream. This MPE stream
// must be properly declared into the PMT of a service. This service must have
// one single MPE stream. A TS may carry several DVB-NIP streams but they must be
// in distinct services, one per DVB-NIP stream.

#include "tsPluginRepository.h"
#include "tsServiceDiscovery.h"
#include "tsNIP.h"
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
        PID     _opt_pid = PID_NULL;
        UString _opt_service {};

        // Plugin private fields.
        bool             _abort = false;            // Error, abort asap.
        bool             _wait_for_service = false; // Wait for MPE service id to be identified.
        PID              _mpe_pid = PID_NULL;       // Actual MPE PID.
        ServiceDiscovery _service {duck, nullptr};  // Service containing the MPE PID.
        MPEDemux         _demux {duck, this};       // MPE demux to extract MPE datagrams.

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
    ProcessorPlugin(tsp_, u"Experimental DVB-NIP (Native IP) analyzer", u"[options]")
{
    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the MPE containing the DVB-NIP stream. "
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
    getIntValue(_opt_pid, u"pid", PID_NULL);
    getValue(_opt_service, u"service");

    // Check parameter consistency.
    if (_opt_pid != PID_NULL && !_opt_service.empty()) {
        error(u"--pid and --service are mutually exclusive");
        return false;
    }

    return true;
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
    _demux.reset();

    if (_mpe_pid != PID_NULL) {
        // MPE PID already known.
        _demux.addPID(_mpe_pid);
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
        _demux.feedPacket(pkt);
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
        _demux.addPID(pid);
    }
}


//----------------------------------------------------------------------------
// Process a MPE packet.
//----------------------------------------------------------------------------

void ts::NIPPlugin::handleMPEPacket(MPEDemux& demux, const MPEPacket& mpe)
{
    const IPSocketAddress destination(mpe.destinationSocket());
    debug(u"MPE packet on PID %n, for address %s, %d bytes", mpe.sourcePID(), destination, mpe.datagramSize());

    if (_abort || mpe.sourcePID() != _mpe_pid) {
        return;
    }

    // Experimental code.
    if (destination == NIPSignallingAddress4() || destination == NIPSignallingAddress6()) {

        // UDP payload.
        // const uint8_t* const udp_data = mpe.udpMessage();
        const size_t udp_size = mpe.udpMessageSize();

        info(u"PID %n, address %s, %d bytes", mpe.sourcePID(), mpe.destinationSocket(), udp_size);
    }
}
