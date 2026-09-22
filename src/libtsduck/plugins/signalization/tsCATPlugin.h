//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to perform various transformations on the CAT.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTablePlugin.h"

namespace ts {
    //!
    //! Plugin to perform various transformations on the CAT.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL CATPlugin: public AbstractTablePlugin
    {
        TS_PLUGIN_CONSTRUCTORS(CATPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;

    private:
        // Command line options:
        bool                  _cleanup_priv_desc = false; // Remove private desc without preceding PDS desc
        std::vector<uint16_t> _remove_casid {};           // Set of CAS id to remove
        std::vector<uint16_t> _remove_pid {};             // Set of EMM PID to remove
        DescriptorList        _add_descs {nullptr};       // List of descriptors to add
    };
}
