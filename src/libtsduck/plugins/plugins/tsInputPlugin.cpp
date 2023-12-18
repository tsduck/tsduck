//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInputPlugin.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::InputPlugin::InputPlugin(TSP* tsp_, const UString& description, const UString& syntax) :
    Plugin(tsp_, description, syntax)
{
}


//----------------------------------------------------------------------------
// Default implementations of virtual methods.
//----------------------------------------------------------------------------

bool ts::InputPlugin::setReceiveTimeout(std::chrono::milliseconds timeout)
{
    return false;
}

bool ts::InputPlugin::abortInput()
{
    return false;
}

ts::PluginType ts::InputPlugin::type() const
{
    return PluginType::INPUT;
}
