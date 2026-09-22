//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to perform various transformations on the BAT.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTablePlugin.h"

namespace ts {
    //!
    //! Plugin to perform various transformations on the BAT.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL BATPlugin: public AbstractTablePlugin
    {
        TS_PLUGIN_CONSTRUCTORS(BATPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;

    private:
        // Command line options:
        bool               _single_bat = false;        // Modify one single BAT only
        uint16_t           _bouquet_id = 0;            // Bouquet id of the BAT to modify (if _single_bat)
        std::set<uint16_t> _remove_serv_ids {};        // Set of services to remove
        std::set<uint16_t> _remove_ts_ids {};          // Set of transport streams to remove
        std::vector<DID>   _removed_desc_tags {};      // Set of descriptor tags to remove
        PDS                _pds = 0;                   // Private data specifier for removed descriptors
        bool               _cleanup_priv_desc = false; // Remove private desc without preceding PDS desc

        // Process a list of descriptors according to the command line options.
        void processDescriptorList(DescriptorList&);
    };
}
