//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tstsmuxCore.h"
#include "tsFatal.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsmux::Core::Core(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, Report& log) :
    _log(log),
    _opt(opt),
    _terminate(false),
    _bitrate(0),
    _inputs(_opt.inputs.size(), nullptr),
    _output(_opt, handlers, _log)
{
    // Load all input plugins, analyze their options.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i] = new InputExecutor(opt, handlers, i, log);
        CheckNonNull(_inputs[i]);
    }
}

ts::tsmux::Core::~Core()
{
    // Wait for termination of all threads.
    waitForTermination();

    // Deallocate all input plugins.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        delete _inputs[i];
    }
    _inputs.clear();
}


//----------------------------------------------------------------------------
// Start the tsmux processing.
//----------------------------------------------------------------------------

bool ts::tsmux::Core::start()
{
    // Initialize the output plugin.
    if (!_output.plugin()->getOptions() || !_output.plugin()->start()) {
        return false;
    }

    // Make sure that we have an output bitrate.
    const BitRate br = _output.plugin()->getBitrate();
    if (br != 0) {
        // The output plugin reports an output bitrate, always use this one.
        _bitrate = br;
        if (_opt.outputBitRate == 0) {
            _log.verbose(u"output bitrate is %'d b/s, as reported by output plugin", {br});
        }
        else if (_opt.outputBitRate != br) {
            _log.warning(u"output bitrate is %'d b/s, as reported by output plugin, overrides %'d b/s from command line", {br, _opt.outputBitRate});
        }
    }
    else if (_opt.outputBitRate == 0) {
        _log.error(u"no output bitrate specified and none reported by output plugin");
        _output.plugin()->stop();
        return false;
    }
    else {
        _bitrate = _opt.outputBitRate;
    }

    // Get all plugin command line options and start them
    // (start the plugins but do not start the plugin executor threads).
    for (size_t i = 0; i < _inputs.size(); ++i) {
        if (!_inputs[i]->plugin()->getOptions() || !_inputs[i]->plugin()->start()) {
            // Error, close previous plugins.
            for (size_t prev = 0; prev < i; ++prev) {
                _inputs[prev]->plugin()->stop();
            }
            _output.plugin()->stop();
            return false;
        }
    }

    // Now that all plugins are open, start all executor threads.
    bool success = _output.start();
    for (size_t i = 0; success && i < _inputs.size(); ++i) {
        success = _inputs[i]->start();
    }

    // Now start the Core internal thread, the one that does the multiplexing.
    success = success && Thread::start();

    if (!success) {
        stop();
    }
    return success;
}


//----------------------------------------------------------------------------
// Stop the tsmux processing.
//----------------------------------------------------------------------------

void ts::tsmux::Core::stop()
{
    // Request termination of all plugin executor threads.
    _output.terminate();
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i]->terminate();
    }

    // Stop our internal thread. We only set the terminate flag, actual termination
    // will occur at the next muxing iteration.
    _terminate = true;
}


//----------------------------------------------------------------------------
// Wait for completion of all plugins.
//----------------------------------------------------------------------------

void ts::tsmux::Core::waitForTermination()
{
    // Wait for output termination.
    _output.waitForTermination();

    // Wait for all input termination.
    for (size_t i = 0; i < _inputs.size(); ++i) {
        _inputs[i]->waitForTermination();
    }

    // Wait for our internal thread.
    Thread::waitForTermination();
}


//----------------------------------------------------------------------------
// Invoked in the context of the core thread.
//----------------------------------------------------------------------------

void ts::tsmux::Core::main()
{
    _log.debug(u"core thread started");

    // Loop until we are instructed to stop.
    while (!_terminate) {

        // @@@@@@@@@
    }

    _log.debug(u"core thread terminated");
}
