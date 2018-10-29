//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::TSP::TSP(int max_severity) :
    Report(max_severity),
    _use_realtime(false),
    _tsp_bitrate(0),
    _tsp_aborting(false)
{
}

ts::Plugin::Plugin(TSP* to_tsp, const UString& description, const UString& syntax) :
    Args(description, syntax, NO_DEBUG | NO_VERBOSE | NO_VERSION | NO_CONFIG_FILE),
    tsp(to_tsp)
{
}

ts::InputPlugin::InputPlugin(TSP* tsp_, const UString& description, const UString& syntax) :
    Plugin(tsp_, description, syntax)
{
}

ts::OutputPlugin::OutputPlugin(TSP* tsp_, const UString& description, const UString& syntax) :
    Plugin(tsp_, description, syntax)
{
}

ts::ProcessorPlugin::ProcessorPlugin(TSP* tsp_, const UString& description, const UString& syntax) :
    Plugin(tsp_, description, syntax)
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
// Displayable names of plugin types.
//----------------------------------------------------------------------------

const ts::Enumeration ts::PluginTypeNames({
    {u"input",            ts::INPUT_PLUGIN},
    {u"output",           ts::OUTPUT_PLUGIN},
    {u"packet processor", ts::PROCESSOR_PLUGIN},
});

