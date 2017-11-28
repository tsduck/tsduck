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
//  Various transformations on the SDT.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsServiceDescriptor.h"
#include "tsService.h"
#include "tsPAT.h"
#include "tsSDT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SDTPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        SDTPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        bool                  _abort;             // Error (service not found, etc)
        Service               _service;           // New or modified service
        std::vector<uint16_t> _remove_serv;       // Set of services to remove
        bool                  _incr_version;      // Increment table version
        bool                  _set_version;       // Set a new table version
        uint8_t               _new_version;       // New table version
        bool                  _cleanup_priv_desc; // Remove private desc without preceding PDS desc
        SectionDemux          _demux;             // Section demux
        CyclingPacketizer     _pzer;              // Packetizer for modified SDT/BAT

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        void processSDT(SDT&);

        // Inaccessible operations
        SDTPlugin() = delete;
        SDTPlugin(const SDTPlugin&) = delete;
        SDTPlugin& operator=(const SDTPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::SDTPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SDTPlugin::SDTPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Perform various transformations on the SDT Actual.", u"[options]"),
    _abort(false),
    _service(),
    _remove_serv(),
    _incr_version(false),
    _set_version(false),
    _new_version(0),
    _cleanup_priv_desc(false),
    _demux(this),
    _pzer()
{
    option(u"cleanup-private-descriptors", 0);
    option(u"eit-pf",                      0,  INTEGER, 0, 1, 0, 1);
    option(u"eit-schedule",                0,  INTEGER, 0, 1, 0, 1);
    option(u"free-ca-mode",               'f', INTEGER, 0, 1, 0, 1);
    option(u"increment-version",          'i');
    option(u"name",                       'n', STRING);
    option(u"new-version",                'v', INTEGER, 0, 1, 0, 31);
    option(u"provider",                   'p', STRING);
    option(u"remove-service",              0,  UINT16, 0, UNLIMITED_COUNT);
    option(u"running-status",             'r', INTEGER, 0, 1, 0, 7);
    option(u"service-id",                 's', UINT16);
    option(u"type",                       't', UINT8);

    setHelp(u"Options:\n"
            u"\n"
            u"  --cleanup-private-descriptors\n"
            u"      Remove all private descriptors without preceding private_data_specifier\n"
            u"      descriptor.\n"
            u"\n"
            u"  --eit-pf value\n"
            u"      Specify a new EIT_present_following_flag value for the added or modified\n"
            u"      service. For new services, the default is 0.\n"
            u"\n"
            u"  --eit-schedule value\n"
            u"      Specify a new EIT_schedule_flag value for the added or modified\n"
            u"      service. For new services, the default is 0.\n"
            u"\n"
            u"  -f value\n"
            u"  --free-ca-mode value\n"
            u"      Specify a new free_CA_mode value for the added or modified service.\n"
            u"      For new services, the default is 0.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --increment-version\n"
            u"      Increment the version number of the SDT.\n"
            u"\n"
            u"  -n value\n"
            u"  --name value\n"
            u"      Specify a new service name for the added or modified service.\n"
            u"      For new services, the default is an empty string.\n"
            u"\n"
            u"  -v value\n"
            u"  --new-version value\n"
            u"      Specify a new value for the version of the SDT.\n"
            u"\n"
            u"  -p value\n"
            u"  --provider value\n"
            u"      Specify a new provider name for the added or modified service.\n"
            u"      For new services, the default is an empty string.\n"
            u"\n"
            u"  --remove-service sid\n"
            u"      Remove the specified service_id from the SDT. Several --remove-service\n"
            u"      options may be specified to remove several services.\n"
            u"\n"
            u"  -r value\n"
            u"  --running-status value\n"
            u"      Specify a new running_status (0 to 7) for the added or modified service.\n"
            u"      For new services, the default is 4 (\"running\").\n"
            u"\n"
            u"  -s value\n"
            u"  --service-id value\n"
            u"      Add a new service or modify the existing service with the specified\n"
            u"      service-id.\n"
            u"\n"
            u"  -t value\n"
            u"  --type value\n"
            u"      Specify a new service type for the added or modified service. For new\n"
            u"      services, the default is 0x01 (\"digital television service\").\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SDTPlugin::start()
{
    // Get option values
    _incr_version = present(u"increment-version");
    _set_version = present(u"new-version");
    _new_version = intValue<uint8_t>(u"new-version", 0);
    _cleanup_priv_desc = present(u"cleanup-private-descriptors");
    getIntValues(_remove_serv, u"remove-service");
    _service.clear();
    if (present(u"eit-pf")) {
        _service.setEITpfPresent(intValue<int>(u"eit-pf") != 0);
    }
    if (present(u"eit-schedule")) {
        _service.setEITsPresent(intValue<int>(u"eit-schedule") != 0);
    }
    if (present(u"free-ca-mode")) {
        _service.setCAControlled(intValue<int>(u"free-ca-mode") != 0);
    }
    if (present(u"name")) {
        _service.setName(value(u"name"));
    }
    if (present(u"provider")) {
        _service.setProvider(value(u"provider"));
    }
    if (present(u"running-status")) {
        _service.setRunningStatus (intValue<uint8_t>(u"running-status"));
    }
    if (present(u"service-id")) {
        _service.setId(intValue<uint16_t>(u"service-id"));
    }
    if (present(u"type")) {
        _service.setType(intValue<uint8_t>(u"type"));
    }

    // Initialize the demux and packetizer
    _demux.reset();
    _demux.addPID(PID_SDT);
    _pzer.reset();
    _pzer.setPID(PID_SDT);

    _abort = false;
    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::SDTPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_SDT_ACT: {
            if (table.sourcePID() == PID_SDT) {
                SDT sdt(table);
                if (sdt.isValid()) {
                    // Modify SDT Actual
                    _pzer.removeSections(TID_SDT_ACT, table.tableIdExtension());
                    processSDT(sdt);
                    _pzer.addTable(sdt);
                }
            }
            break;
        }

        case TID_SDT_OTH: {
            if (table.sourcePID() == PID_SDT) {
                // SDT Other are passed unmodified
                _pzer.removeSections(TID_SDT_OTH, table.tableIdExtension());
                _pzer.addTable(table);
            }
            break;
        }

        case TID_BAT: {
            if (table.sourcePID() == PID_BAT) {
                // Do not modify BAT
                _pzer.removeSections(TID_BAT, table.tableIdExtension());
                _pzer.addTable(table);
            }
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a SDT
//----------------------------------------------------------------------------

void ts::SDTPlugin::processSDT(SDT& sdt)
{
    // Update SDT version
    if (_incr_version) {
        sdt.version = (sdt.version + 1) & 0x1F;
    }
    else if (_set_version) {
        sdt.version = _new_version;
    }

    // Add / modify a service
    if (_service.hasId()) {

        // Create new service is not existing
        if (sdt.services.find(_service.getId()) == sdt.services.end()) {
            // Service did not exist, create a new one with all defaults
            SDT::Service sv;
            sv.EITs_present = false;
            sv.EITpf_present = false;
            sv.running_status = 4; // running
            sv.CA_controlled = false;
            sv.descs.add(ServiceDescriptor(0x01, "", ""));
            sdt.services.insert(std::make_pair(_service.getId(), sv));
        }

        // Locate service to modify
        SDT::Service& sv(sdt.services[_service.getId()]);

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
            sv.setName(_service.getName());
        }
        if (_service.hasProvider()) {
            sv.setProvider(_service.getProvider());
        }
        if (_service.hasRunningStatus()) {
            sv.running_status = _service.getRunningStatus();
        }
        if (_service.hasType()) {
            sv.setType(_service.getType());
        }
    }

    // Remove selected services
    for (std::vector<uint16_t>::const_iterator it = _remove_serv.begin(); it != _remove_serv.end(); ++it) {
        sdt.services.erase(*it);
    }

    // Remove private descriptors without preceding PDS descriptor
    if (_cleanup_priv_desc) {
        for (SDT::ServiceMap::iterator smi = sdt.services.begin(); smi != sdt.services.end(); ++smi) {
            smi->second.descs.removeInvalidPrivateDescriptors();
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SDTPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Filter interesting sections
    _demux.feedPacket(pkt);

    // If a fatal error occured during section analysis, give up.
    if (_abort) {
        return TSP_END;
    }

    // Replace packets using packetizer
    if (pkt.getPID() == PID_SDT) {
        _pzer.getNextPacket(pkt);
    }

    return TSP_OK;
}
