//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractSingleMPEPlugin.h"
#include "tsMPEPacket.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AbstractSingleMPEPlugin::AbstractSingleMPEPlugin(TSP* tsp_, const UString& description, const UString& syntax, const UString& stream_name) :
    ProcessorPlugin(tsp_, description, syntax)
{
    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the MPE PID containing the " + stream_name + ". "
         u"By default, if neither --pid nor --service is specified, use the first MPE PID which is found."
         u"Options --pid and --service are mutually exclusive.");

    option(u"service", 's', STRING);
    help(u"service", u"name-or-id",
         u"Specify the service containing the " + stream_name + " in a MPE PID. "
         u"If the argument is an integer value (either decimal or hexadecimal), it is interpreted as a service id. "
         u"Otherwise, it is interpreted as a service name, as specified in the SDT. "
         u"The name is not case sensitive and blanks are ignored. "
         u"By default, if neither --pid nor --service is specified, use the first MPE PID which is found."
         u"Options --pid and --service are mutually exclusive.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::AbstractSingleMPEPlugin::getOptions()
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

bool ts::AbstractSingleMPEPlugin::start()
{
    _abort = false;
    _wait_for_service = false;
    _mpe_pid = _opt_pid;
    _last_timestamp = PCR::zero();
    _last_time_source = TimeSource::UNDEFINED;
    _service.clear();
    _mpe_demux.reset();

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
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::AbstractSingleMPEPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& mdata)
{
    if (mdata.hasInputTimeStamp()) {
        _last_timestamp = mdata.getInputTimeStamp();
        _last_time_source = mdata.getInputTimeSource();
    }

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

void ts::AbstractSingleMPEPlugin::handleMPENewPID(MPEDemux& demux, const PMT& pmt, PID pid)
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

void ts::AbstractSingleMPEPlugin::handleMPEPacket(MPEDemux& demux, const MPEPacket& mpe)
{
    if (!_abort && mpe.sourcePID() == _mpe_pid) {
        handleSingleMPEPacket(_last_timestamp, _last_time_source, mpe);
    }
}
