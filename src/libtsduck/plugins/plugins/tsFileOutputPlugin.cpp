//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFileOutputPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_OUTPUT_PLUGIN(u"file", ts::FileOutputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::FileOutputPlugin::FileOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Write packets to a file", u"[options] [file-name]")
{
    _file.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Redirect all methods to _file.
//----------------------------------------------------------------------------

bool ts::FileOutputPlugin::getOptions()
{
    return _file.loadArgs(duck, *this);
}

bool ts::FileOutputPlugin::start()
{
    return _file.open(*this, tsp);
}

bool ts::FileOutputPlugin::stop()
{
    return _file.close(*this);
}

bool ts::FileOutputPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    return _file.write(buffer, pkt_data, packet_count, *this, tsp);
}
