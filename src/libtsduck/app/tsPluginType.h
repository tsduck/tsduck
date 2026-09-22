//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definition of tsp plugins types.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"

namespace ts {
    //!
    //! Each plugin has one of the following types
    //! @ingroup plugin
    //!
    enum class PluginType {
        INPUT,     //!< Input plugin.
        OUTPUT,    //!< Output plugin.
        PROCESSOR  //!< Packet processor plugin.
    };

    //!
    //! Displayable names of plugin types.
    //! @ingroup plugin
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& PluginTypeNames();
}
