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
//
//  Transport stream processor shared library:
//  Fork a process and send TS packets to its standard input (pipe)
//  or receive packets from its standard output.
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

    // Input plugin
    class ForkInput: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(ForkInput);
    public:
        // Implementation of plugin API
        ForkInput(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput() override;

    private:
        UString  _command;      // The command to run.
        bool     _nowait;       // Don't wait for children termination.
        size_t   _buffer_size;  // Pipe buffer size in packets.
        ForkPipe _pipe;         // The pipe device.
    };

    // Output plugin
    class ForkOutput: public OutputPlugin
    {
        TS_NOBUILD_NOCOPY(ForkOutput);
    public:
        // Implementation of plugin API
        ForkOutput(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        UString  _command;      // The command to run.
        bool     _nowait;       // Don't wait for children termination.
        size_t   _buffer_size;  // Pipe buffer size in packets.
        ForkPipe _pipe;         // The pipe device.
    };

    // Packet processor plugin
    class ForkPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(ForkPlugin);
    public:
        // Implementation of plugin API
        ForkPlugin (TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        UString        _command;       // The command to run.
        bool           _nowait;        // Don't wait for children termination.
        size_t         _buffer_size;   // Max number of packets in buffer.
        size_t         _buffer_count;  // Number of packets currently in buffer.
        TSPacketVector _buffer;        // Packet buffer.
        ForkPipe       _pipe;          // The pipe device.
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_INPUT(fork, ts::ForkInput)
TSPLUGIN_DECLARE_OUTPUT(fork, ts::ForkOutput)
TSPLUGIN_DECLARE_PROCESSOR(fork, ts::ForkPlugin)


//----------------------------------------------------------------------------
// Input constructor
//----------------------------------------------------------------------------

ts::ForkInput::ForkInput(TSP* tsp_) :
    InputPlugin(tsp_, u"Fork a process and receive TS packets from its standard output", u"[options] 'command'"),
    _command(),
    _nowait(false),
    _buffer_size(0),
    _pipe()
{
    option(u"", 0, STRING, 1, 1);
    help(u"", u"Specifies the command line to execute in the created process.");

    option(u"buffered-packets", 'b', POSITIVE);
    help(u"buffered-packets", u"Windows only: Specifies the pipe buffer size in number of TS packets.");

    option(u"nowait", 'n');
    help(u"nowait", u"Do not wait for child process termination at end of its output.");
}


//----------------------------------------------------------------------------
// Output constructor
//----------------------------------------------------------------------------

ts::ForkOutput::ForkOutput(TSP* tsp_) :
    OutputPlugin(tsp_, u"Fork a process and send TS packets to its standard input", u"[options] 'command'"),
    _command(),
    _nowait(false),
    _buffer_size(0),
    _pipe()
{
    option(u"", 0, STRING, 1, 1);
    help(u"", u"Specifies the command line to execute in the created process.");

    option(u"buffered-packets", 'b', POSITIVE);
    help(u"buffered-packets", u"Windows only: Specifies the pipe buffer size in number of TS packets.");

    option(u"nowait", 'n');
    help(u"nowait", u"Do not wait for child process termination at end of input.");
}


//----------------------------------------------------------------------------
// Packet processor constructor
//----------------------------------------------------------------------------

ts::ForkPlugin::ForkPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Fork a process and send TS packets to its standard input", u"[options] 'command'"),
    _command(),
    _nowait(false),
    _buffer_size(0),
    _buffer_count(0),
    _buffer(),
    _pipe()
{
    option(u"", 0, STRING, 1, 1);
    help(u"", u"Specifies the command line to execute in the created process.");

    option(u"buffered-packets", 'b', POSITIVE);
    help(u"buffered-packets",
         u"Specifies the number of TS packets to buffer before sending them through "
         u"the pipe to the forked process. When set to zero, the packets are not "
         u"buffered and sent one by one. The default is 500 packets in real-time mode "
         u"and 1000 packets in offline mode.");

    option(u"ignore-abort", 'i');
    help(u"ignore-abort",
         u"Ignore early termination of child process. By default, if the child "
         u"process aborts and no longer reads the packets, tsp also aborts.");

    option(u"nowait", 'n');
    help(u"nowait", u"Do not wait for child process termination at end of input.");
}


//----------------------------------------------------------------------------
// Input methods
//----------------------------------------------------------------------------

bool ts::ForkInput::getOptions()
{
    // Get command line arguments.
    _command = value(u"");
    _nowait = present(u"nowait");
    _buffer_size = intValue<size_t>(u"buffered-packets", 0);
    return true;
}


bool ts::ForkInput::start()
{
    // Create pipe & process.
    return _pipe.open(_command,
                      _nowait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
                      PKT_SIZE * _buffer_size,  // Pipe buffer size (Windows only, zero meaning default).
                      *tsp,                     // Error reporting.
                      ForkPipe::STDOUT_PIPE,    // Output: send stdout to pipe, keep same stderr as tsp.
                      ForkPipe::STDIN_NONE);    // Input: null device (do not use the same stdin as tsp).
}

bool ts::ForkInput::stop()
{
    // Close the pipe
    return _pipe.close(*tsp);
}

bool ts::ForkInput::abortInput()
{
    // Abort current operations on the pipe.
    _pipe.abortPipeReadWrite();
    return true;
}

size_t ts::ForkInput::receive(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets)
{
    // Read always an integral number of TS packets.
    size_t ret_size = 0;
    bool success = _pipe.read(buffer, max_packets * PKT_SIZE, PKT_SIZE, ret_size, *tsp);
    return success ? ret_size / PKT_SIZE : 0;
}


//----------------------------------------------------------------------------
// Output methods
//----------------------------------------------------------------------------

bool ts::ForkOutput::getOptions()
{
    // Get command line arguments.
    _command = value(u"");
    _nowait = present(u"nowait");
    _buffer_size = intValue<size_t>(u"buffered-packets", 0);
    return true;
}


bool ts::ForkOutput::start()
{
    // Create pipe & process.
    return _pipe.open(_command,
                      _nowait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
                      PKT_SIZE * _buffer_size,  // Pipe buffer size (Windows only), same as internal buffer size.
                      *tsp,                     // Error reporting.
                      ForkPipe::KEEP_BOTH,      // Output: same stdout and stderr as tsp process.
                      ForkPipe::STDIN_PIPE);    // Input: use the pipe.
}

bool ts::ForkOutput::stop()
{
    // Close the pipe
    return _pipe.close(*tsp);
}

bool ts::ForkOutput::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    // Send packets in the pipe.
    return _pipe.write(buffer, PKT_SIZE * packet_count, *tsp);
}


//----------------------------------------------------------------------------
// Packet processor methods
//----------------------------------------------------------------------------

bool ts::ForkPlugin::getOptions()
{
    // Get command line arguments
    _command = value(u"");
    _nowait = present(u"nowait");
    _buffer_size = intValue<size_t>(u"buffered-packets", tsp->realtime() ? 500 : 1000);
    _pipe.setIgnoreAbort(present(u"ignore-abort"));

    // If packet buffering is requested, allocate the buffer
    _buffer.resize(_buffer_size);

    return true;
}


bool ts::ForkPlugin::start()
{
    // Reset buffer usage.
    _buffer_count = 0;

    // Create pipe & process.
    return _pipe.open(_command,
                      _nowait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
                      PKT_SIZE * _buffer_size,  // Pipe buffer size (Windows only), same as internal buffer size.
                      *tsp,                     // Error reporting.
                      ForkPipe::KEEP_BOTH,      // Output: same stdout and stderr as tsp process.
                      ForkPipe::STDIN_PIPE);    // Input: use the pipe.
}


bool ts::ForkPlugin::stop()
{
    // Flush buffered packets.
    if (_buffer_count > 0) {
        _pipe.write(_buffer.data(), PKT_SIZE * _buffer_count, *tsp);
    }

    // Close the pipe
    return _pipe.close(*tsp);
}


ts::ProcessorPlugin::Status ts::ForkPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // If packets are sent one by one, just send it.
    if (_buffer_size == 0) {
        return _pipe.write(&pkt, PKT_SIZE, *tsp) ? TSP_OK : TSP_END;
    }

    // Add the packet to the buffer
    assert(_buffer_count < _buffer.size());
    _buffer[_buffer_count++] = pkt;

    // Flush the buffer when full
    if (_buffer_count == _buffer.size()) {
        _buffer_count = 0;
        return _pipe.write(_buffer.data(), PKT_SIZE * _buffer.size(), *tsp) ? TSP_OK : TSP_END;
    }

    return TSP_OK;
}
