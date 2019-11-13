//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsFilePacketPlugin.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Packet processor constructor
//----------------------------------------------------------------------------

ts::FilePacketPlugin::FilePacketPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Write packets to a file and pass them to next plugin", u"[options] file-name"),
    _name(),
    _flags(TSFile::NONE),
    _file()
{
    option(u"", 0, STRING, 1, 1);
    help(u"", u"Name of the created output file.");

    option(u"append", 'a');
    help(u"append", u"If the file already exists, append to the end of the file. By default, existing files are overwritten.");

    option(u"keep", 'k');
    help(u"keep", u"Keep existing file (abort if the specified file already exists). By default, existing files are overwritten.");
}


//----------------------------------------------------------------------------
// Packet processor plugin methods
//----------------------------------------------------------------------------

bool ts::FilePacketPlugin::getOptions()
{
    getValue(_name);
    _flags = TSFile::WRITE | TSFile::SHARED;
    if (present(u"append")) {
        _flags |= TSFile::APPEND;
    }
    if (present(u"keep")) {
        _flags |= TSFile::KEEP;
    }
    return true;
}

bool ts::FilePacketPlugin::start()
{
    return _file.open(_name, _flags, *tsp);
}

bool ts::FilePacketPlugin::stop()
{
    return _file.close(*tsp);
}

ts::ProcessorPlugin::Status ts::FilePacketPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    return _file.write(&pkt, 1, *tsp) ? TSP_OK : TSP_END;
}
