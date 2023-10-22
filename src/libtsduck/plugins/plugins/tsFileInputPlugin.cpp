//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFileInputPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_INPUT_PLUGIN(u"file", ts::FileInputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::FileInputPlugin::FileInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Read packets from one or more files", u"[options] [file-name ...]")
{
    _file.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Redirect all methods to _file.
//----------------------------------------------------------------------------

bool ts::FileInputPlugin::getOptions()
{
    return _file.loadArgs(duck, *this);
}

bool ts::FileInputPlugin::start()
{
    return _file.open(*tsp);
}

bool ts::FileInputPlugin::stop()
{
    return _file.close(*tsp);
}

bool ts::FileInputPlugin::abortInput()
{
    _file.abort();
    return true;
}

size_t ts::FileInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    return _file.read(buffer, pkt_data, max_packets, *tsp);
}
