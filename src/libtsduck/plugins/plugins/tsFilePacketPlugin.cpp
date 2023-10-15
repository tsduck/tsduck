//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFilePacketPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"file", ts::FilePacketPlugin);


//----------------------------------------------------------------------------
// Packet processor constructor
//----------------------------------------------------------------------------

ts::FilePacketPlugin::FilePacketPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Write packets to a file and pass them to next plugin", u"[options] file-name")
{
    _file.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Redirect all methods to _file.
//----------------------------------------------------------------------------

bool ts::FilePacketPlugin::getOptions()
{
    return _file.loadArgs(duck, *this);
}

bool ts::FilePacketPlugin::start()
{
    return _file.open(*tsp, tsp);
}

bool ts::FilePacketPlugin::stop()
{
    return _file.close(*tsp);
}

ts::ProcessorPlugin::Status ts::FilePacketPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    return _file.write(&pkt, &pkt_data, 1, *tsp, tsp) ? TSP_OK : TSP_END;
}
