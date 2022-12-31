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
//  Various transformations on the SDT.
//
//----------------------------------------------------------------------------

#include "tsAbstractTablePlugin.h"
#include "tsPluginRepository.h"
#include "tsServiceDescriptor.h"
#include "tsService.h"
#include "tsAlgorithm.h"
#include "tsSDT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SDTPlugin: public AbstractTablePlugin
    {
        TS_NOBUILD_NOCOPY(SDTPlugin);
    public:
        // Implementation of plugin API
        SDTPlugin(TSP*);
        virtual bool getOptions() override;

    private:
        // Command line options:
        bool                  _use_other;          // Modify an SDT Other, not the SDT Actual.
        uint16_t              _other_ts_id;        // TS id of the SDT Other to modify.
        Service               _service;            // New or modified service properties.
        std::vector<uint16_t> _remove_serv;        // Set of services to remove
        bool                  _cleanup_priv_desc;  // Remove private desc without preceding PDS desc

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"sdt", ts::SDTPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SDTPlugin::SDTPlugin(TSP* tsp_) :
    AbstractTablePlugin(tsp_, u"Perform various transformations on the SDT", u"[options]", u"SDT", PID_SDT),
    _use_other(false),
    _other_ts_id(false),
    _service(),
    _remove_serv(),
    _cleanup_priv_desc(false)
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"cleanup-private-descriptors");
    help(u"cleanup-private-descriptors",
         u"Remove all private descriptors without preceding private_data_specifier descriptor.");

    option(u"eit-pf", 0, INTEGER, 0, 1, 0, 1);
    help(u"eit-pf",
         u"Specify a new EIT_present_following_flag value for the added or modified "
         u"service. For new services, the default is 0.");

    option(u"eit-schedule", 0, INTEGER, 0, 1, 0, 1);
    help(u"eit-schedule",
         u"Specify a new EIT_schedule_flag value for the added or modified "
         u"service. For new services, the default is 0.");

    option(u"free-ca-mode", 'f', INTEGER, 0, 1, 0, 1);
    help(u"free-ca-mode",
         u"Specify a new free_CA_mode value for the added or modified service. "
         u"For new services, the default is 0.");

    option(u"name", 'n', STRING);
    help(u"name",
         u"Specify a new service name for the added or modified service. "
         u"For new services, the default is an empty string.");

    option(u"original-network-id", 0, UINT16);
    help(u"original-network-id", u"id",
         u"Modify the original network id in the SDT with the specified value.");

    option(u"other", 'o', UINT16);
    help(u"other", u"id",
         u"Modify the SDT Other with the specified TS id. "
         u"By default, modify the SDT Actual.");

    option(u"provider", 'p', STRING);
    help(u"provider",
         u"Specify a new provider name for the added or modified service. "
         u"For new services, the default is an empty string.");

    option(u"remove-service", 0, UINT16, 0, UNLIMITED_COUNT);
    help(u"remove-service", u"id",
         u"Remove the specified service_id from the SDT. Several --remove-service "
         u"options may be specified to remove several services.");

    option(u"running-status", 'r', INTEGER, 0, 1, 0, 7);
    help(u"running-status",
         u"Specify a new running_status (0 to 7) for the added or modified service. "
         u"For new services, the default is 4 (\"running\").");

    option(u"service-id", 's', UINT16);
    help(u"service-id", u"id",
         u"Add a new service or modify the existing service with the specified service-id.");

    option(u"ts-id", 0, UINT16);
    help(u"ts-id", u"id",
         u"Modify the transport stream id in the SDT with the specified value.");

    option(u"type", 't', UINT8);
    help(u"type",
         u"Specify a new service type for the added or modified service. For new "
         u"services, the default is 0x01 (\"digital television service\").");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::SDTPlugin::getOptions()
{
    _service.clear();

    // Global properties.
    duck.loadArgs(*this);
    _cleanup_priv_desc = present(u"cleanup-private-descriptors");
    _use_other = present(u"other");
    getIntValue(_other_ts_id, u"other");
    getIntValues(_remove_serv, u"remove-service");
    if (present(u"service-id")) {
        _service.setId(intValue<uint16_t>(u"service-id"));
    }
    if (present(u"ts-id")) {
        _service.setTSId(intValue<uint16_t>(u"ts-id"));
    }
    if (present(u"original-network-id")) {
        _service.setONId(intValue<uint16_t>(u"original-network-id"));
    }

    // Properties of the service to add or modify.
    bool use_service_properties = false;
    if (present(u"eit-pf")) {
        _service.setEITpfPresent(intValue<int>(u"eit-pf") != 0);
        use_service_properties = true;
    }
    if (present(u"eit-schedule")) {
        _service.setEITsPresent(intValue<int>(u"eit-schedule") != 0);
        use_service_properties = true;
    }
    if (present(u"free-ca-mode")) {
        _service.setCAControlled(intValue<int>(u"free-ca-mode") != 0);
        use_service_properties = true;
    }
    if (present(u"name")) {
        _service.setName(value(u"name"));
        use_service_properties = true;
    }
    if (present(u"provider")) {
        _service.setProvider(value(u"provider"));
        use_service_properties = true;
    }
    if (present(u"running-status")) {
        _service.setRunningStatus (intValue<uint8_t>(u"running-status"));
        use_service_properties = true;
    }
    if (present(u"type")) {
        _service.setTypeDVB(intValue<uint8_t>(u"type"));
        use_service_properties = true;
    }
    if (use_service_properties && !_service.hasId()) {
        tsp->error(u"specify --service-id when changing service properties");
        return false;
    }

    // Get options for superclass.
    return AbstractTablePlugin::getOptions();
}


//----------------------------------------------------------------------------
// Invoked by the superclass to create an empty table.
//----------------------------------------------------------------------------

void ts::SDTPlugin::createNewTable(BinaryTable& table)
{
    SDT sdt;

    // If we must modify one specific SDT, this is the one we need to create.
    if (_use_other) {
        sdt.setActual(false);
        sdt.ts_id = _other_ts_id;
    }

    sdt.serialize(duck, table);
}


//----------------------------------------------------------------------------
// Invoked by the superclass when a table is found in the target PID.
//----------------------------------------------------------------------------

void ts::SDTPlugin::modifyTable(BinaryTable& table, bool& is_target, bool& reinsert)
{
    // If not an SDT (typically a BAT) or not the SDT we are looking for, reinsert without modification.
    is_target = (!_use_other && table.tableId() == TID_SDT_ACT) ||
                (_use_other && table.tableId() == TID_SDT_OTH && table.tableIdExtension() == _other_ts_id);
    if (!is_target) {
        return;
    }

    // Process the SDT.
    SDT sdt(duck, table);
    if (!sdt.isValid()) {
        tsp->warning(u"found invalid SDT");
        reinsert = false;
        return;
    }

    // Modify global values.
    if (_service.hasTSId()) {
        sdt.ts_id = _service.getTSId();
    }
    if (_service.hasONId()) {
        sdt.onetw_id = _service.getONId();
    }

    // Add / modify a service
    if (_service.hasId()) {

        // Create new service is not existing
        if (!Contains(sdt.services, _service.getId())) {
            // Service did not exist, create a new one with all defaults
            SDT::ServiceEntry& sv(sdt.services[_service.getId()]);
            sv.EITs_present = false;
            sv.EITpf_present = false;
            sv.running_status = 4; // running
            sv.CA_controlled = false;
            sv.descs.add(duck, ServiceDescriptor(0x01, u"", u""));
        }

        // Locate service to modify
        SDT::ServiceEntry& sv(sdt.services[_service.getId()]);

        // Modify service characteristics
        if (_service.hasEITpfPresent()) {
            sv.EITpf_present = _service.getEITpfPresent();
        }
        if (_service.hasEITsPresent()) {
            sv.EITs_present = _service.getEITsPresent();
        }
        if (_service.hasCAControlled()) {
            sv.CA_controlled = _service.getCAControlled();
        }
        if (_service.hasName()) {
            sv.setName(duck, _service.getName());
        }
        if (_service.hasProvider()) {
            sv.setProvider(duck, _service.getProvider());
        }
        if (_service.hasRunningStatus()) {
            sv.running_status = _service.getRunningStatus();
        }
        if (_service.hasTypeDVB()) {
            sv.setType(_service.getTypeDVB());
        }
    }

    // Remove selected services
    for (auto id : _remove_serv) {
        sdt.services.erase(id);
    }

    // Remove private descriptors without preceding PDS descriptor
    if (_cleanup_priv_desc) {
        for (auto& smi : sdt.services) {
            smi.second.descs.removeInvalidPrivateDescriptors();
        }
    }

    // Reserialize modified SDT.
    sdt.serialize(duck, table);
}
