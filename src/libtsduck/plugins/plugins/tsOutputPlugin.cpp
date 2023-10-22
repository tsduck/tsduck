//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsOutputPlugin.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::OutputPlugin::OutputPlugin(TSP* tsp_, const UString& description, const UString& syntax) :
    Plugin(tsp_, description, syntax)
{
}


//----------------------------------------------------------------------------
// Default implementations of virtual methods.
//----------------------------------------------------------------------------

ts::PluginType ts::OutputPlugin::type() const
{
    return PluginType::OUTPUT;
}
