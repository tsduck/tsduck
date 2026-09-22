//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to perform various transformations on the PAT.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTablePlugin.h"
#include "tsService.h"

namespace ts {
    //!
    //! Plugin to perform various transformations on the PAT.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PATPlugin: public AbstractTablePlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PATPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;

    private:
        std::vector<uint16_t> _remove_serv {};        // Set of services to remove
        ServiceVector         _add_serv {};           // Set of services to add
        PID                   _new_nit_pid = PID_NIT; // New PID for NIT
        bool                  _remove_nit = false;    // Remove NIT from PAT
        bool                  _set_tsid = false;      // Set a new TS id
        uint16_t              _new_tsid = 0;          // New TS id

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;
    };
}
