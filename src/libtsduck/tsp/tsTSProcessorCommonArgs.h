//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream processors common command-line options
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Common command line options for all transport stream processors.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL TSProcessorCommonArgs
    {
    public:
        TSProcessorCommonArgs() = default;  //!< Default constructor.
        UString app_name {};                //!< Application name, for help messages.
        bool    ignore_jt = false;          //!< Ignore "joint termination" options in plugins.
        bool    log_plugin_index = false;   //!< Log plugin index with plugin name.
    };
}
