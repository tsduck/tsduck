//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to perform various transformations on the SDT.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTablePlugin.h"
#include "tsService.h"

namespace ts {
    //!
    //! Plugin to perform various transformations on the SDT.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SDTPlugin: public AbstractTablePlugin
    {
        TS_PLUGIN_CONSTRUCTORS(SDTPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;

    private:
        // Command line options:
        bool                  _use_other = false;          // Modify an SDT Other, not the SDT Actual.
        uint16_t              _other_ts_id = false;        // TS id of the SDT Other to modify.
        Service               _service {};                 // New or modified service properties.
        std::vector<uint16_t> _remove_serv {};             // Set of services to remove
        bool                  _cleanup_priv_desc = false;  // Remove private desc without preceding PDS desc

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;
    };
}
