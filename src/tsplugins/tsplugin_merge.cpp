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
//  Merge TS packets coming from the standard output of a command.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsForkPipe.h"
#include "tsTSPacketQueue.h"
#include "tsThread.h"
TSDUCK_SOURCE;

#define DEFAULT_MAX_QUEUED_PACKETS  1000
#define SERVER_THREAD_STACK_SIZE    (128 * 1024)


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MergePlugin: public ProcessorPlugin, private Thread
    {
    public:
        // Implementation of plugin API
        MergePlugin (TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        ForkPipe      _pipe;
        TSPacketQueue _queue;

        // There is one thread which receives packet from the created process and passes
        // them to the main plugin thread. The following method is the thread main code.
        virtual void main() override;

        // Inaccessible operations
        MergePlugin() = delete;
        MergePlugin(const MergePlugin&) = delete;
        MergePlugin& operator=(const MergePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(merge, ts::MergePlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MergePlugin::MergePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Merge TS packets coming from the standard output of a command", u"[options] 'command'"),
    _pipe(),
    _queue()
{
    option(u"",          0,  STRING, 1, 1);
    option(u"max-queue", 0,  POSITIVE);
    option(u"nowait",   'n');

    setHelp(u"Command:\n"
            u"  Specifies the command line to execute in the created process.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --max-queue value\n"
            u"      Specify the maximum number of queued TS packets before their\n"
            u"      insertion into the stream. The default is " TS_STRINGIFY(DEFAULT_MAX_QUEUED_PACKETS) u".\n"
            u"\n"
            u"  -n\n"
            u"  --nowait\n"
            u"      Do not wait for child process termination at end of processing.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MergePlugin::start()
{
    // Get command line arguments
    UString command(value());
    const bool nowait = present(u"nowait");
    const size_t max_queue = intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_PACKETS);

    // Resize the inter-thread packet queue.
    _queue.reset(max_queue);

    // Start the internal thread which receives the TS to merge.
    Thread::start();

    // Create pipe & process
    return _pipe.open(command,
                      nowait ? ForkPipe::ASYNCHRONOUS : ForkPipe::SYNCHRONOUS,
                      PKT_SIZE * DEFAULT_MAX_QUEUED_PACKETS,
                      *tsp,
                      ForkPipe::STDOUT_PIPE,
                      ForkPipe::STDIN_NONE);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::MergePlugin::stop()
{
    // Send the stop condition to the internal packet queue.
    _queue.stop();

    // Close the pipe and terminate the created process.
    _pipe.close(*tsp);

    // Wait for actual thread termination.
    Thread::waitForTermination();
    return true;
}

//----------------------------------------------------------------------------
// Implementation of the receiver thread.
// It simply reads TS packets and passes them to the plugin thread.
//----------------------------------------------------------------------------

void ts::MergePlugin::main()
{
    tsp->debug(u"receiver thread started");

    // Loop on packet reception until the plugin request to stop.
    while (!_queue.stopped()) {

        TSPacket* buffer = 0;
        size_t buffer_size = 0;  // In TS packets.
        size_t read_size = 0;    // In bytes.

        // Wait for free space in the internal packet queue.
        // We don't want to read too many small data sizes, so we wait for at least 16 packets.
        if (!_queue.lockWriteBuffer(buffer, buffer_size, 16)) {
            // The plugin thread has signalled a stop condition.
            break;
        }

        assert(buffer != 0);
        assert(buffer_size > 0);

        // Read TS packets from the pipe, up to buffer size (but maybe less).
        // We request to read only multiples of 188 bytes (the packet size).
        if (!_pipe.read(buffer, PKT_SIZE * buffer_size, PKT_SIZE, read_size, *tsp)) {
            // Read error or end of file, cannot continue in all cases.
            // Signal end-of-file to plugin thread.
            _queue.setEOF();
            break;
        }

        assert(read_size % PKT_SIZE == 0);

        // Pass the read packets to the inter-thread queue.
        // The read size was returned in bytes, we must give a number of packets.
        _queue.releaseWriteBuffer(read_size / PKT_SIZE);
    }

    tsp->debug(u"receiver thread completed");
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MergePlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    //@@ merge to be implemented.
    return TSP_OK;
}
