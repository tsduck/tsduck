//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPluginType.h"


//----------------------------------------------------------------------------
// Displayable names of plugin types.
//----------------------------------------------------------------------------

const ts::Names& ts::PluginTypeNames()
{
    static const Names data {
        {u"input",            PluginType::INPUT},
        {u"output",           PluginType::OUTPUT},
        {u"packet processor", PluginType::PROCESSOR},
    };
    return data;
}
