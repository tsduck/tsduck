//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"

// Displayable names of plugin types.
const ts::TypedEnumeration<ts::PluginType> ts::PluginTypeNames({
    {u"input",            ts::PluginType::INPUT},
    {u"output",           ts::PluginType::OUTPUT},
    {u"packet processor", ts::PluginType::PROCESSOR},
});


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::Plugin::Plugin(TSP* to_tsp, const UString& description, const UString& syntax) :
    Args(description, syntax, NO_DEBUG | NO_VERBOSE | NO_VERSION | NO_CONFIG_FILE),
    tsp(to_tsp),
    duck(to_tsp)
{
}


//----------------------------------------------------------------------------
// Report implementation.
//----------------------------------------------------------------------------

void ts::Plugin::writeLog(int severity, const UString& message)
{
    // Force message to go through tsp
    tsp->log(severity, message);
}


//----------------------------------------------------------------------------
// Reset the internal TSDuck execution context of this plugin.
//----------------------------------------------------------------------------

void ts::Plugin::resetContext(const DuckContext::SavedArgs& state)
{
    duck.reset();
    duck.restoreArgs(state);
}


//----------------------------------------------------------------------------
// Default implementations of virtual methods.
//----------------------------------------------------------------------------

size_t ts::Plugin::stackUsage() const
{
    return DEFAULT_STACK_USAGE;
}

bool ts::Plugin::getOptions()
{
    return true;
}

bool ts::Plugin::start()
{
    return true;
}

bool ts::Plugin::stop()
{
    return true;
}

ts::BitRate ts::Plugin::getBitrate()
{
    return 0;
}

ts::BitRateConfidence ts::Plugin::getBitrateConfidence()
{
    return BitRateConfidence::LOW;
}

bool ts::Plugin::isRealTime()
{
    return false;
}

bool ts::Plugin::handlePacketTimeout()
{
    return false;
}
