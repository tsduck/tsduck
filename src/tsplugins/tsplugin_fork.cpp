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
//
//  Transport stream processor shared library:
//  Fork a process and send TS packets to its standard output (pipe)
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsForkPipe.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class ForkPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        ForkPlugin (TSP*);
        ~ForkPlugin();
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        ForkPipe  _pipe;
        size_t    _buffer_size;   // Max number of packets in buffer
        size_t    _buffer_count;  // Number of packets currently in buffer
        TSPacket* _buffer;        // Packet buffer

        // Inaccessible operations
        ForkPlugin() = delete;
        ForkPlugin(const ForkPlugin&) = delete;
        ForkPlugin& operator=(const ForkPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(fork, ts::ForkPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ForkPlugin::ForkPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Fork a process and send TS packets to its standard input", u"[options] 'command'"),
    _pipe(),
    _buffer_size(0),
    _buffer_count(0),
    _buffer(0)
{
    option(u"",                  0,  STRING, 1, 1);
    option(u"buffered-packets", 'b', POSITIVE);
    option(u"ignore-abort",     'i');
    option(u"nowait",           'n');

    setHelp(u"Command:\n"
            u"  Specifies the command line to execute in the created process.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --buffered-packets value\n"
            u"      Specifies the number of TS packets to buffer before sending them\n"
            u"      through the pipe to the forked process. By default, the packets are\n"
            u"      not buffered and sent one by one.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --ignore-abort\n"
            u"      Ignore early termination of child process. By default, if the child\n"
            u"      process aborts and no longer reads the packets, tsp also aborts.\n"
            u"\n"
            u"  -n\n"
            u"  --nowait\n"
            u"      Do not wait for child process termination at end of input.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::ForkPlugin::~ForkPlugin()
{
    if (_buffer != 0) {
        delete[] _buffer;
        _buffer = 0;
    }
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::ForkPlugin::start()
{
    // Get command line arguments
    UString command(value());
    bool nowait = present(u"nowait");
    _buffer_size = intValue<size_t>(u"buffered-packets", 0);
    _pipe.setIgnoreAbort(present(u"ignore-abort"));

    // If packet buffering is requested, allocate the buffer
    _buffer = 0;
    _buffer_count = 0;
    if (_buffer_size > 0 && (_buffer = new TSPacket[_buffer_size]) == 0) {
        tsp->error(u"cannot allocate packet buffer, reduce --buffered-packets value");
        return false;
    }

    // Create pipe & process
    return _pipe.open(command, nowait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS, PKT_SIZE * _buffer_size, *tsp, ForkPipe::KEEP_BOTH, ForkPipe::STDIN_PIPE);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::ForkPlugin::stop()
{
    // Flush buffered packets
    if (_buffer_count > 0) {
        _pipe.write(_buffer, PKT_SIZE * _buffer_count, *tsp);
    }

    // Free packet buffer, if there is one
    if (_buffer != 0) {
        delete[] _buffer;
        _buffer = 0;
    }

    // Close the pipe
    return _pipe.close(*tsp);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::ForkPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // If packets are sent one by one, just send it
    if (_buffer_size == 0) {
        return _pipe.write (&pkt, PKT_SIZE, *tsp) ? TSP_OK : TSP_END;
    }

    // Add the packet to the buffer
    assert (_buffer_count < _buffer_size);
    _buffer [_buffer_count++] = pkt;

    // Flush the buffer when full
    if (_buffer_count == _buffer_size) {
        _buffer_count = 0;
        return _pipe.write (_buffer, PKT_SIZE * _buffer_size, *tsp) ? TSP_OK : TSP_END;
    }

    return TSP_OK;
}
