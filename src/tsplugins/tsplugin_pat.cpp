//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsAbstractTablePlugin.h"
#include "tsPluginRepository.h"
#include "tsService.h"
#include "tsPAT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PATPlugin: public AbstractTablePlugin
    {
        TS_NOBUILD_NOCOPY(PATPlugin);
    public:
        // Implementation of plugin API
        PATPlugin(TSP*);
        virtual bool start() override;

    private:
        std::vector<uint16_t> _remove_serv;  // Set of services to remove
        ServiceVector         _add_serv;     // Set of services to add
        PID                   _new_nit_pid;  // New PID for NIT
        bool                  _remove_nit;   // Remove NIT from PAT
        bool                  _set_tsid;     // Set a new TS id
        uint16_t              _new_tsid;     // New TS id

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pat", ts::PATPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PATPlugin::PATPlugin(TSP* tsp_) :
    AbstractTablePlugin(tsp_, u"Perform various transformations on the PAT", u"[options]", u"PAT", PID_PAT),
    _remove_serv(),
    _add_serv(),
    _new_nit_pid(PID_NIT),
    _remove_nit(false),
    _set_tsid(false),
    _new_tsid(0)
{
    option(u"add-service", 'a', STRING, 0, UNLIMITED_COUNT);
    help(u"add-service", u"service-id/pid",
         u"Add the specified service_id / PMT-PID in the PAT. Several --add-service "
         u"options may be specified to add several services.");

    option(u"nit", 'n', PIDVAL);
    help(u"nit",
         u"Add or modify the NIT PID in the PAT.");

    option(u"remove-service", 'r', UINT16, 0, UNLIMITED_COUNT);
    help(u"remove-service", u"id",
         u"Remove the specified service_id from the PAT. Several --remove-service "
         u"options may be specified to remove several services.");

    option(u"remove-nit", 'u');
    help(u"remove-nit",
         u"Remove the NIT PID from the PAT.");

    option(u"ts-id", 't', UINT16);
    help(u"ts-id", u"id",
         u"Specify a new value for the transport stream id in the PAT.");

    option(u"tsid", 0, UINT16);
    help(u"tsid", u"id",
         u"Same as --ts-id (for compatibility).");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PATPlugin::start()
{
    // Get option values
    getIntValue(_new_nit_pid, u"nit", PID_NULL);
    _remove_nit = present(u"remove-nit");
    _set_tsid = present(u"ts-id") || present(u"tsid");
    getIntValue(_new_tsid, u"ts-id", intValue<uint16_t>(u"tsid", 0));
    getIntValues(_remove_serv, u"remove-service");

    // Get list of services to add
    const size_t add_count = count(u"add-service");
    UString sidpid;
    _add_serv.clear();
    _add_serv.reserve (add_count);
    for (size_t n = 0; n < add_count; n++) {
        getValue(sidpid, u"add-service", u"", n);
        int sid = 0, pid = 0;
        if (!sidpid.scan(u"%i/%i", {&sid, &pid}) || sid < 0 || sid > 0xFFFF || pid < 0 || pid >= PID_MAX) {
            Args::error(u"invalid \"service_id/PID\" value \"%s\"", {sidpid});
            return false;
        }
        Service serv;
        serv.setId(uint16_t(sid));
        serv.setPMTPID(PID(pid));
        _add_serv.push_back (serv);
    }

    // Start superclass.
    return AbstractTablePlugin::start();
}


//----------------------------------------------------------------------------
// Invoked by the superclass to create an empty table.
//----------------------------------------------------------------------------

void ts::PATPlugin::createNewTable(BinaryTable& table)
{
    PAT pat;
    pat.serialize(duck, table);
}


//----------------------------------------------------------------------------
// Invoked by the superclass when a table is found in the target PID.
//----------------------------------------------------------------------------

void ts::PATPlugin::modifyTable(BinaryTable& table, bool& is_target, bool& reinsert)
{
    // Warn about non-PAT tables in the PAT PID but keep them.
    if (table.tableId() != TID_PAT) {
        tsp->warning(u"found table id 0x%X (%d) in the PAT PID", {table.tableId(), table.tableId()});
        is_target = false;
        return;
    }

    // Process the PAT.
    PAT pat(duck, table);
    if (!pat.isValid()) {
        tsp->warning(u"found invalid PAT");
        reinsert = false;
        return;
    }

    // Modify the PAT
    if (_set_tsid) {
        pat.ts_id = _new_tsid;
    }
    if (_remove_nit) {
        pat.nit_pid = PID_NULL;
    }
    if (_new_nit_pid != PID_NULL) {
        pat.nit_pid = _new_nit_pid;
    }
    for (auto id : _remove_serv) {
        pat.pmts.erase(id);
    }
    for (const auto& it : _add_serv) {
        assert(it.hasId());
        assert(it.hasPMTPID());
        pat.pmts[it.getId()] = it.getPMTPID();
    }

    // Reserialize modified PAT.
    pat.serialize(duck, table);
}
