//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to perform various transformations on the NIT.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTablePlugin.h"
#include "tsServiceListDescriptor.h"
#include "tsTransportStreamId.h"
#include "tsNIT.h"
#include "tsPAT.h"
#include "tsSDT.h"

namespace ts {
    //!
    //! Plugin to perform various transformations on the NIT.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL NITPlugin: public AbstractTablePlugin
    {
        TS_PLUGIN_CONSTRUCTORS(NITPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;

    protected:
        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

    private:
        // A map of service list descriptors, indexed by ts id / original netwok id.
        using SLDMap = std::map<TransportStreamId, ServiceListDescriptor>;

        PID                _nit_pid = PID_NIT;            // PID for the NIT (default: read PAT)
        UString            _new_netw_name {};             // New network name
        bool               _set_netw_id = false;          // Change network id
        uint16_t           _new_netw_id = 0;              // New network id
        bool               _set_onetw_id = false;         // Change original network id of all TS
        uint16_t           _new_onetw_id = 0;             // New original network id
        bool               _use_nit_other = false;        // Use a NIT Other, not the NIT Actual
        uint16_t           _nit_other_id = 0;             // Network id of the NIT Other to hack
        int                _lcn_oper = 0;                 // Operation on LCN descriptors
        int                _sld_oper = 0;                 // Operation on service_list_descriptors
        std::set<uint16_t> _remove_serv {};               // Set of services to remove
        std::set<uint16_t> _remove_ts {};                 // Set of transport streams to remove
        std::vector<DID>   _removed_desc {};              // Set of descriptor tags to remove
        PDS                _pds = 0;                      // Private data specifier for removed descriptors
        bool               _cleanup_priv_desc = false;    // Remove private desc without preceding PDS desc
        bool               _update_mpe_fec = false;       // In terrestrial delivery
        uint8_t            _mpe_fec = 0;
        bool               _update_time_slicing = false;  // In terrestrial delivery
        uint8_t            _time_slicing = 0;
        bool               _build_sld = false;            // Build service list descriptors.
        bool               _add_all_srv_in_sld = false;   // Add all services in service list descriptors, even when the type in unknown.
        uint8_t            _default_srv_type = 0;         // Default service type in service list descriptors.
        SectionDemux       _demux {duck, this};           // Section demux to collect PAT and SDT to build service list descriptors.
        NIT                _last_nit {};                  // Last valid NIT found, after modification.
        PAT                _last_pat {};                  // Last valid input PAT.
        SDT                _last_sdt_act {};              // Last valid input SDT Actual.
        SLDMap             _collected_sld {};             // A map of service list descriptors per TS id.

        // Values for _lcn_oper and _sld_oper.
        enum {
            LCN_NONE          = 0,
            LCN_REMOVE        = 1,
            LCN_REMOVE_ODD    = 2,
            LCN_DUPLICATE_ODD = 3  // LCN only
        };

        // Process a list of descriptors according to the command line options.
        void processDescriptorList(DescriptorList&);

        // Merge last collected PAT in the collected services.
        // Return true if the list of collected services has been modified.
        bool mergeLastPAT();

        // Merge an SDTT in the collected services.
        // Return true if the list of collected services has been modified.
        bool mergeSDT(const SDT&);

        // Update the service list descriptors from collected services.
        void updateServiceList(NIT&);
    };
}
