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
//  Transport stream processor: Execution context of a plugin
//
//----------------------------------------------------------------------------

#include "tspPluginExecutor.h"
#include "tsGuardCondition.h"
#include "tsGuard.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tsp::PluginExecutor::PluginExecutor(Options* options,
                                        const Options::PluginOptions* pl_options,
                                        const ThreadAttributes& attributes,
                                        Mutex& global_mutex) :
    RingNode(),
    JointTermination(options, global_mutex),
    Thread(attributes),
    PluginSharedLibrary(pl_options->name, *options),
    _name(pl_options->name),
    _shlib(0),
    _buffer(0),
    _report(options),
    _to_do(),
    _pkt_first(0),
    _pkt_cnt(0),
    _input_end(false),
    _bitrate(0)
{
    const char* shell = 0;

    // If shared library not loaded, give up.

    if (!isLoaded()) {
        return;
    }

    // Create the plugin instance object

    switch (pl_options->type) {
        case Options::INPUT:
            if (new_input != 0) {
                _shlib = new_input(this);
                shell = "tsp -I";
            }
            break;
        case Options::OUTPUT:
            if (new_output != 0) {
                _shlib = new_output(this);
                shell = "tsp -O";
            }
            break;
        case Options::PROCESSOR:
            if (new_processor != 0) {
                _shlib = new_processor(this);
                shell = "tsp -P";
            }
            break;
        default:
            assert(false);
    }

    if (_shlib == 0) {
        error(u"plugin does not implement the %s function", {Options::PluginTypeNames.name(pl_options->type)});
        unload();
        return;
    }
    else {
        _shlib->setShell(shell);
    }

    // Submit the plugin arguments for analysis.
    // The process should terminate on argument error.

    _shlib->analyze(pl_options->name, pl_options->args);
    assert(_shlib->valid());

    // Define thread stack size
    ThreadAttributes attr;
    Thread::getAttributes(attr);
    attr.setStackSize(STACK_SIZE_OVERHEAD + _shlib->stackUsage());
    Thread::setAttributes(attr);
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::tsp::PluginExecutor::~PluginExecutor()
{
    // Deallocate plugin instance, if allocated.
    if (_shlib != 0) {
        delete _shlib;
        _shlib = 0;
    }
}


//----------------------------------------------------------------------------
// Set the initial state of the buffer. Must be executed in
// synchronous environment, before starting all executor threads.
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::initBuffer(PacketBuffer* buffer,
                                         size_t        pkt_first,
                                         size_t        pkt_cnt,
                                         bool          input_end,
                                         bool          aborted,
                                         BitRate       bitrate)
{
    _buffer = buffer;
    _pkt_first = pkt_first;
    _pkt_cnt = pkt_cnt;
    _input_end = input_end;
    _tsp_aborting = aborted;
    _bitrate = bitrate;
    _tsp_bitrate = bitrate;
}


//----------------------------------------------------------------------------
// Invoked by shared library to log messages
// Inherited from Report (via TSP)
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::writeLog(int severity, const UString& msg)
{
    _report->log(severity, u"%s: %s", {_name, msg});
}


//----------------------------------------------------------------------------
// This method signals that the specified number of packets have been
// processed by this processor. These packets are passed to the next processor
// (which is notified that there is something to do)
//
// Note that, if the caller thread is the output processor, the semantic of
// the operation is "these buffers are no longer used and can be reused by
// the input thread".
//
// Here, "input_end" means "this processor will no longer produce packets".
// And "aborted" means "this processor has encountered an error and will
// cease to accept packets".
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::passPackets(size_t count,     // of packets to pass
                                          BitRate bitrate,  // pass to next processor
                                          bool input_end,   // pass to next processor
                                          bool aborted)     // set to current processor

{
    assert(count <= _pkt_cnt);
    assert(_pkt_first + count <= _buffer->count());

    log(10, u"passPackets (count = %'d, bitrate = %'d, input_end = %'d, aborted = %'d)", {count, bitrate, input_end, aborted});

    // We access data under the protection of the global mutex.

    Guard lock(_global_mutex);

    // Update our buffer

    _pkt_first = (_pkt_first + count) % _buffer->count();
    _pkt_cnt -= count;

    // Update next processor's buffer.

    PluginExecutor* next = ringNext<PluginExecutor>();
    next->_pkt_cnt += count;
    next->_input_end = next->_input_end || input_end;
    next->_bitrate = bitrate;

    // Wake the next processor when there is some data

    if (count > 0 || input_end) {
        next->_to_do.signal();
    }

    // Wake the previous processor when we abort

    if (aborted) {
        _tsp_aborting = true; // volatile bool in TSP superclass
        ringPrevious<PluginExecutor>()->_to_do.signal();
    }
}


//----------------------------------------------------------------------------
// This method sets the current processor in an abort state.
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::setAbort()
{
    Guard lock(_global_mutex);
    _tsp_aborting = true;
    ringPrevious<PluginExecutor>()->_to_do.signal();
}


//----------------------------------------------------------------------------
// This method makes the calling processor thread waiting for packets
// to process or some error condition. Always return a contiguous array
// of packets. If the circular buffer wrap-over occurs in the middle of
// the caller's area, only return the first part, up the buffer's highest
// address. The next call to waitWork will return the second part.
//----------------------------------------------------------------------------

void ts::tsp::PluginExecutor::waitWork(size_t& pkt_first,
                                       size_t& pkt_cnt,
                                       BitRate& bitrate,
                                       bool& input_end,
                                       bool& aborted)    // get from next processor
{
    log(10, u"waitWork(...)");

    // We access data under the protection of the global mutex.

    GuardCondition lock(_global_mutex, _to_do);

    while (_pkt_cnt == 0 && !_input_end && !ringNext<PluginExecutor>()->_tsp_aborting) {

        // If packet area for this processor is empty, wait for some packet.
        // The mutex is implicitely released, we wait for the condition
        // '_to_do' and, once we get it, implicitely relock the mutex.
        // We loop on this until packets are actually available.

        lock.waitCondition();
    }

    pkt_first = _pkt_first;
    pkt_cnt = std::min(_pkt_cnt, _buffer->count() - _pkt_first);
    bitrate = _bitrate;
    input_end = _input_end && pkt_cnt == _pkt_cnt;
    aborted = ringNext<PluginExecutor>()->_tsp_aborting;

    log(10, u"waitWork (pkt_first = %'d, pkt_cnt = %'d, bitrate = %'d, input_end = %'d, aborted = %'d)", {pkt_first, pkt_cnt, bitrate, input_end, aborted});
}
