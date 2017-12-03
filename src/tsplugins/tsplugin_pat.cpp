//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Various transformations on the PAT.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsService.h"
#include "tsPAT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PATPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        PATPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        bool                  _abort;        // Error (service not found, etc)
        std::vector<uint16_t> _remove_serv;  // Set of services to remove
        ServiceVector         _add_serv;     // Set of services to add
        PID                   _new_nit_pid;  // New PID for NIT
        bool                  _remove_nit;   // Remove NIT from PAT
        bool                  _set_tsid;     // Set a new TS id
        uint16_t              _new_tsid;     // New TS id
        bool                  _incr_version; // Increment table version
        bool                  _set_version;  // Set a new table version
        uint8_t               _new_version;  // New table version
        SectionDemux          _demux;        // Section demux
        CyclingPacketizer     _pzer;         // Packetizer for modified PAT

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&) override;

        // Inaccessible operations
        PATPlugin() = delete;
        PATPlugin(const PATPlugin&) = delete;
        PATPlugin& operator=(const PATPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::PATPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PATPlugin::PATPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Perform various transformations on the PAT", u"[options]"),
    _abort(false),
    _remove_serv(),
    _add_serv(),
    _new_nit_pid(PID_NIT),
    _remove_nit(false),
    _set_tsid(false),
    _new_tsid(0),
    _incr_version(false),
    _set_version(false),
    _new_version(0),
    _demux(this),
    _pzer()
{
    option(u"add-service",       'a', STRING, 0, UNLIMITED_COUNT);
    option(u"increment-version", 'i');
    option(u"nit",               'n', PIDVAL);
    option(u"remove-service",    'r', UINT16, 0, UNLIMITED_COUNT);
    option(u"remove-nit",        'u');
    option(u"tsid",              't', UINT16);
    option(u"new-version",       'v', INTEGER, 0, 1, 0, 31);

    setHelp(u"Options:\n"
            u"\n"
            u"  -a sid/pid\n"
            u"  --add-service sid/pid\n"
            u"      Add the specified service_id / PMT-PID in the PAT. Several --add-service\n"
            u"      options may be specified to add several services.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --increment-version\n"
            u"      Increment the version number of the PAT.\n"
            u"\n"
            u"  -n pid\n"
            u"  --nit pid\n"
            u"      Add or modify the NIT PID in the PAT.\n"
            u"\n"
            u"  -r sid\n"
            u"  --remove-service sid\n"
            u"      Remove the specified service_id from the PAT. Several --remove-service\n"
            u"      options may be specified to remove several services.\n"
            u"\n"
            u"  -u\n"
            u"  --remove-nit\n"
            u"      Remove the NIT PID from the PAT.\n"
            u"\n"
            u"  -t id\n"
            u"  --tsid id\n"
            u"      Specify a new value for the transport stream id in the PAT.\n"
            u"\n"
            u"  -v value\n"
            u"  --new-version value\n"
            u"      Specify a new value for the version of the PAT.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PATPlugin::start()
{
    // Get option values
    _new_nit_pid = intValue<PID>(u"nit", PID_NULL);
    _remove_nit = present(u"remove-nit");
    _set_tsid = present(u"tsid");
    _new_tsid = intValue<uint16_t>(u"tsid", 0);
    _incr_version = present(u"increment-version");
    _set_version = present(u"new-version");
    _new_version = intValue<uint8_t>(u"new-version", 0);
    getIntValues(_remove_serv, u"remove-service");

    // Get list of services to add
    const size_t add_count = count(u"add-service");
    UString sidpid;
    _add_serv.clear();
    _add_serv.reserve (add_count);
    for (size_t n = 0; n < add_count; n++) {
        getValue(sidpid, u"add-service", u"", n);
        int sid = 0, pid = 0;
        if (!sidpid.scan("%i/%i", {&sid, &pid}) || sid < 0 || sid > 0xFFFF || pid < 0 || pid >= PID_MAX) {
            Args::error(u"invalid \"service_id/PID\" value \"%s\"", {sidpid});
            return false;
        }
        Service serv;
        serv.setId(uint16_t(sid));
        serv.setPMTPID(PID(pid));
        _add_serv.push_back (serv);
    }

    // Initialize the demux and packetizer
    _demux.reset();
    _demux.addPID(PID_PAT);
    _pzer.reset();
    _pzer.setPID(PID_PAT);

    _abort = false;
    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::PATPlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    if (table.tableId() != TID_PAT || table.sourcePID() != PID_PAT) {
        return;
    }

    PAT pat(table);
    if (!pat.isValid()) {
        return;
    }

    // Modify the PAT
    if (_set_tsid) {
        pat.ts_id = _new_tsid;
    }
    if (_incr_version) {
        pat.version = (pat.version + 1) & 0x1F;
    }
    else if (_set_version) {
        pat.version = _new_version;
    }
    if (_remove_nit) {
        pat.nit_pid = PID_NULL;
    }
    if (_new_nit_pid != PID_NULL) {
        pat.nit_pid = _new_nit_pid;
    }
    for (std::vector<uint16_t>::const_iterator it = _remove_serv.begin(); it != _remove_serv.end(); ++it) {
        pat.pmts.erase(*it);
    }
    for (ServiceVector::const_iterator it = _add_serv.begin(); it != _add_serv.end(); ++it) {
        assert(it->hasId());
        assert(it->hasPMTPID());
        pat.pmts[it->getId()] = it->getPMTPID();
    }

    // Place modified PAT in the packetizer
    tsp->verbose(u"PAT version %d modified", {pat.version});
    _pzer.removeSections(TID_PAT);
    _pzer.addTable(pat);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PATPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Filter interesting sections
    _demux.feedPacket (pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // Replace packets using packetizer
    if (pkt.getPID() == PID_PAT) {
        _pzer.getNextPacket (pkt);
    }

    return TSP_OK;
}
