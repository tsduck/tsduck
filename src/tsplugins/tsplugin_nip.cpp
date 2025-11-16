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
#include "tsServiceDiscovery.h"
#include "tsNames.h"
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
    if (destination == NIPSignallingAddress4() || destination.sameMulticast6(NIPSignallingAddress6())) {

        // UDP payload.
        const uint8_t* data = mpe.udpMessage();
        size_t size = mpe.udpMessageSize();

        // Get LCT header.
        LCTHeader lct;
        if (!lct.deserialize(data, size)) {
            error(u"invalid LCT header from %s", mpe.sourceSocket());
            return;
        }

        // The FEC Encoding ID is stored in codepoint (RFC 3926, section 5.1).
        // We currently only support the default one, value 0.
        if (lct.codepoint != FEI_COMPACT_NOCODE) {
            error(u"unsupported FEC Encoding ID %d from %s", lct.codepoint, mpe.sourceSocket());
            return;
        }

        // The LCT header is followed by the FEC Payload Id (RFC 5775, section 2).
        // The FEC Payload ID for FEC Encoding IDs 0 and 130 (RFC 3695, section 2.1) is made of 2 16-bit integers.
        if (size < 4) {
            error(u"truncated FED Payload ID from %s, %d bytes", mpe.sourceSocket(), size);
            return;
        }
        const uint16_t source_block_number = GetUInt16(data);
        const uint16_t encoding_symbol_id = GetUInt16(data + 2);
        data += 4;
        size -= 4;

        // Display debug message on packet format.
        UString line;
        line.format(u"PID %n, src: %s, dst: %s, version: %d, psi: %d, cci: %d bytes, tsi: %d (%d bytes), toi: %d (%d bytes)\n"
                    u"    codepoint: %d, close sess: %s, close obj: %s, extensions: ",
                    mpe.sourcePID(), mpe.sourceSocket(), mpe.destinationSocket(),
                    lct.lct_version, lct.psi, lct.cci.size(), lct.tsi, lct.tsi_length, lct.toi, lct.toi_length,
                    lct.codepoint, lct.close_session, lct.close_object);
        bool got_ext = false;
        for (const auto& e : lct.ext) {
            if (got_ext) {
                line += u", ";
            }
            got_ext = true;
            line.format(u"%d (%s, %d bytes)", e.first, NameFromSection(u"dtv", u"lct_het", e.first), e.second.size());
        }
        if (!got_ext) {
            line += u"none";
        }
        line.format(u"\n    source block number: %d, encoding symbol id: %d", source_block_number, encoding_symbol_id);
        NIPActualCarrierInformation naci;
        if (naci.deserialize(lct)) {
            line.format(u"\n    naci: network: %n, carrier: %n, link: %n, service: %n, provider: \"%s\"",
                        naci.nip_network_id, naci.nip_carrier_id, naci.nip_link_id, naci.nip_service_id, naci.nip_stream_provider_name);
        }
        line.format(u"\n    payload: %d bytes", size);
        if (size > 0) {
            line += u'\n';
            line.appendDump(data, size, UString::ASCII | UString::HEXA | UString::BPL, 4, 16);
            line.trim(false, true);
        }
        info(line);
    }
}
