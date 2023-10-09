//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsForkOutputPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_OUTPUT_PLUGIN(u"fork", ts::ForkOutputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ForkOutputPlugin::ForkOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Fork a process and send TS packets to its standard input", u"[options] 'command'"),
    _command(),
    _nowait(false),
    _format(TSPacketFormat::TS),
    _buffer_size(0),
    _pipe()
{
    DefineTSPacketFormatOutputOption(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"", u"Specifies the command line to execute in the created process.");

    option(u"buffered-packets", 'b', POSITIVE);
    help(u"buffered-packets", u"Windows only: Specifies the pipe buffer size in number of TS packets.");

    option(u"nowait", 'n');
    help(u"nowait", u"Do not wait for child process termination at end of input.");
}


//----------------------------------------------------------------------------
// Output methods
//----------------------------------------------------------------------------

bool ts::ForkOutputPlugin::getOptions()
{
    // Get command line arguments.
    getValue(_command, u"");
    getIntValue(_buffer_size, u"buffered-packets", 0);
    _nowait = present(u"nowait");
    _format = LoadTSPacketFormatOutputOption(*this);
    return true;
}


bool ts::ForkOutputPlugin::start()
{
    // Create pipe & process.
    return _pipe.open(_command,
                      _nowait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
                      PKT_SIZE * _buffer_size,  // Pipe buffer size (Windows only), same as internal buffer size.
                      *tsp,                     // Error reporting.
                      ForkPipe::KEEP_BOTH,      // Output: same stdout and stderr as tsp process.
                      ForkPipe::STDIN_PIPE,     // Input: use the pipe.
                      _format);
}

bool ts::ForkOutputPlugin::stop()
{
    return _pipe.close(*tsp);
}

bool ts::ForkOutputPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    return _pipe.writePackets(buffer, pkt_data, packet_count, *tsp);
}
