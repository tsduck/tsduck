//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsForkPacketPlugin.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"fork", ts::ForkPacketPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ForkPacketPlugin::ForkPacketPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Fork a process and send TS packets to its standard input", u"[options] 'command'"),
    _command(),
    _nowait(false),
    _format(TSPacketFormat::TS),
    _buffer_size(0),
    _buffer_count(0),
    _buffer(),
    _mdata(),
    _pipe()
{
    DefineTSPacketFormatOutputOption(*this);

    option(u"", 0, STRING, 1, 1);
    help(u"", u"Specifies the command line to execute in the created process.");

    option(u"buffered-packets", 'b', UNSIGNED); // zero allowed
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
// Packet processor methods
//----------------------------------------------------------------------------

bool ts::ForkPacketPlugin::getOptions()
{
    // Get command line arguments
    getValue(_command, u"");
    getIntValue(_buffer_size, u"buffered-packets", tsp->realtime() ? 500 : 1000);
    _nowait = present(u"nowait");
    _format = LoadTSPacketFormatOutputOption(*this);
    _pipe.setIgnoreAbort(present(u"ignore-abort"));

    // If packet buffering is requested, allocate the buffer
    _buffer.resize(_buffer_size);
    _mdata.resize(_buffer_size);

    return true;
}


bool ts::ForkPacketPlugin::start()
{
    // Reset buffer usage.
    _buffer_count = 0;

    // Create pipe & process.
    return _pipe.open(_command,
                      _nowait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
                      PKT_SIZE * _buffer_size,  // Pipe buffer size (Windows only), same as internal buffer size.
                      *this,                    // Error reporting.
                      ForkPipe::KEEP_BOTH,      // Output: same stdout and stderr as tsp process.
                      ForkPipe::STDIN_PIPE,     // Input: use the pipe.
                      _format);
}


bool ts::ForkPacketPlugin::stop()
{
    // Flush buffered packets.
    if (_buffer_count > 0) {
        _pipe.writePackets(_buffer.data(), _mdata.data(), _buffer_count, *this);
    }

    // Close the pipe
    return _pipe.close(*this);
}


ts::ProcessorPlugin::Status ts::ForkPacketPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // If packets are sent one by one, just send it.
    if (_buffer_size == 0) {
        return _pipe.writePackets(&pkt, &pkt_data, 1, *this) ? TSP_OK : TSP_END;
    }

    // Add the packet to the buffer
    assert(_buffer_count < _buffer.size());
    _buffer[_buffer_count] = pkt;
    _mdata[_buffer_count++] = pkt_data;

    // Flush the buffer when full
    if (_buffer_count == _buffer.size()) {
        _buffer_count = 0;
        return _pipe.writePackets(_buffer.data(), _mdata.data(), _buffer.size(), *this) ? TSP_OK : TSP_END;
    }

    return TSP_OK;
}
