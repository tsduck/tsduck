//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

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
TSPLUGIN_DECLARE_PROCESSOR(ts::ForkPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ForkPlugin::ForkPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, "Fork a process and send TS packets to its standard input.", "[options] 'command'"),
    _pipe(),
    _buffer_size(0),
    _buffer_count(0),
    _buffer(0)
{
    option ("",                  0,  STRING, 1, 1);
    option ("buffered-packets", 'b', POSITIVE);
    option ("ignore-abort",     'i');
    option ("nowait",           'n');

    setHelp ("Command:\n"
             "  Specifies the command line to execute in the created process.\n"
             "\n"
             "Options:\n"
             "\n"
             "  -b value\n"
             "  --buffered-packets value\n"
             "      Specifies the number of TS packets to buffer before sending them\n"
             "      through the pipe to the forked process. By default, the packets are\n"
             "      not buffered and sent one by one.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -i\n"
             "  --ignore-abort\n"
             "      Ignore early termination of child process. By default, if the child\n"
             "      process aborts and no longer reads the packets, tsp also aborts.\n"
             "\n"
             "  -n\n"
             "  --nowait\n"
             "      Do not wait for child process termination at end of input.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
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
    std::string command (value());
    bool synchronous = !present ("nowait");
    _buffer_size = intValue<size_t> ("buffered-packets", 0);
    _pipe.setIgnoreAbort (present ("ignore-abort"));

    // If packet buffering is requested, allocate the buffer
    _buffer = 0;
    _buffer_count = 0;
    if (_buffer_size > 0 && (_buffer = new TSPacket [_buffer_size]) == 0) {
        tsp->error ("cannot allocate packet buffer, reduce --buffered-packets value");
        return false;
    }

    // Create pipe & process
    return _pipe.open (command, synchronous, PKT_SIZE * _buffer_size, *tsp);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::ForkPlugin::stop()
{
    // Flush buffered packets
    if (_buffer_count > 0) {
        _pipe.write (_buffer, PKT_SIZE * _buffer_count, *tsp);
    }

    // Free packet buffer, if there is one
    if (_buffer != 0) {
        delete[] _buffer;
        _buffer = 0;
    }

    // Close the pipe
    return _pipe.close (*tsp);
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
