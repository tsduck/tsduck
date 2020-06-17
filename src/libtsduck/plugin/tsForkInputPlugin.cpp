//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsForkInputPlugin.h"
#include "tsPluginRepository.h"
TSDUCK_SOURCE;

TS_REGISTER_INPUT_PLUGIN(u"fork", ts::ForkInputPlugin);

// A dummy storage value to force inclusion of this module when using the static library.
const int ts::ForkInputPlugin::REFERENCE = 0;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ForkInputPlugin::ForkInputPlugin(TSP* tsp_) :
    InputPlugin(tsp_, u"Fork a process and receive TS packets from its standard output", u"[options] 'command'"),
    _command(),
    _nowait(false),
    _format(TSPacketFormat::AUTODETECT),
    _buffer_size(0),
    _pipe()
{
    option(u"", 0, STRING, 1, 1);
    help(u"", u"Specifies the command line to execute in the created process.");

    option(u"buffered-packets", 'b', POSITIVE);
    help(u"buffered-packets", u"Windows only: Specifies the pipe buffer size in number of TS packets.");

    option(u"format", 0, TSPacketFormatEnum);
    help(u"format", u"name",
         u"Specify the format of the input stream. "
         u"By default, the format is automatically detected. "
         u"But the auto-detection may fail in some cases "
         u"(for instance when the first time-stamp of an M2TS file starts with 0x47). "
         u"Using this option forces a specific format.");

    option(u"nowait", 'n');
    help(u"nowait", u"Do not wait for child process termination at end of its output.");
}


//----------------------------------------------------------------------------
// Input methods
//----------------------------------------------------------------------------

bool ts::ForkInputPlugin::getOptions()
{
    // Get command line arguments.
    _command = value(u"");
    _nowait = present(u"nowait");
    _format = enumValue<TSPacketFormat>(u"format", TSPacketFormat::AUTODETECT);
    _buffer_size = intValue<size_t>(u"buffered-packets", 0);
    return true;
}


bool ts::ForkInputPlugin::start()
{
    // Create pipe & process.
    return _pipe.open(_command,
                      _nowait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
                      PKT_SIZE * _buffer_size,  // Pipe buffer size (Windows only, zero meaning default).
                      *tsp,                     // Error reporting.
                      ForkPipe::STDOUT_PIPE,    // Output: send stdout to pipe, keep same stderr as tsp.
                      ForkPipe::STDIN_NONE,     // Input: null device (do not use the same stdin as tsp).
                      _format);                 // Expected TS format, usually autodetect.
}

bool ts::ForkInputPlugin::stop()
{
    return _pipe.close(*tsp);
}

bool ts::ForkInputPlugin::abortInput()
{
    _pipe.abortPipeReadWrite();
    return true;
}

size_t ts::ForkInputPlugin::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    return _pipe.readPackets(buffer, pkt_data, max_packets, *tsp);
}
