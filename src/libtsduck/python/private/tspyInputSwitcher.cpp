//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  TSDuck Python bindings: encapsulates InputSwitcher objects for Python.
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tsInputSwitcher.h"
#include "tsNullReport.h"

TS_MSC_NOWARNING(4091) // '__declspec(dllexport)': ignored on left of 'struct type' when no variable is declared

//
// Argument structure (plain C structure) for start parameters.
// Use same names as ts::InputSwitcherArgs but only long's to avoid interface issues.
//
TSDUCKPY struct tspyInputSwitcherArgs
{
    long fast_switch;         // Fast switch between input plugins.
    long delayed_switch;      // Delayed switch between input plugins.
    long terminate;           // Terminate when one input plugin completes.
    long reuse_port;          // Reuse-port socket option.
    long first_input;         // Index of first input plugin.
    long primary_input;       // Index of primary input plugin, negative if there is none.
    long cycle_count;         // Number of input cycles to execute (0;
    long buffered_packets;    // Input buffer size in packets (0=default).
    long max_input_packets;   // Maximum input packets to read at a time (0=default).
    long max_output_packets;  // Maximum input packets to send at a time (0=default).
    long sock_buffer;         // Socket buffer size (0=default).
    long remote_server_port;  // UDP server port for remote control (0=none).
    long receive_timeout;     // Receive timeout before switch (0=none).
    const uint8_t* plugins;   // Address of UTF-16 multi-strings buffer for plugins.
    size_t         plugins_size;         // Size in bytes of plugins buffer.
    const uint8_t* event_command;        // Address of UTF-16 multi-strings buffer for event command.
    size_t         event_command_size;   // Size in bytes of event_command.
    const uint8_t* event_udp_addr;       // Address of UTF-16 multi-strings buffer for event UDP IP addresds.
    size_t         event_udp_addr_size;  // Size in bytes of event_udp_addr.
    long           event_udp_port;       // Associated UDP port number.
    const uint8_t* local_addr;           // Address of UTF-16 multi-strings buffer for event UDP outgoing interface.
    size_t         local_addr_size;      // Size in bytes of local_addr.
    long           event_ttl;            // Time-to-live socket option for event UDP.
};

//-----------------------------------------------------------------------------
// Interface to InputSwitcher.
//-----------------------------------------------------------------------------

TSDUCKPY void* tspyNewInputSwitcher(void* report)
{
    ts::Report* rep = reinterpret_cast<ts::Report*>(report);
    return new ts::InputSwitcher(rep == nullptr ? NULLREP : *rep);
}

TSDUCKPY void tspyDeleteInputSwitcher(void* pyobj)
{
    delete reinterpret_cast<ts::InputSwitcher*>(pyobj);
}

TSDUCKPY void tspyStopInputSwitcher(void* pyobj)
{
    ts::InputSwitcher* isw = reinterpret_cast<ts::InputSwitcher*>(pyobj);
    if (isw != nullptr) {
        isw->stop();
    }
}

TSDUCKPY void tspyWaitInputSwitcher(void* pyobj)
{
    ts::InputSwitcher* isw = reinterpret_cast<ts::InputSwitcher*>(pyobj);
    if (isw != nullptr) {
        isw->waitForTermination();
    }
}

TSDUCKPY void tspyInputSwitcherSetInput(void* pyobj, size_t index)
{
    ts::InputSwitcher* isw = reinterpret_cast<ts::InputSwitcher*>(pyobj);
    if (isw != nullptr) {
        isw->setInput(index);
    }
}

TSDUCKPY void tspyInputSwitcherNextInput(void* pyobj)
{
    ts::InputSwitcher* isw = reinterpret_cast<ts::InputSwitcher*>(pyobj);
    if (isw != nullptr) {
        isw->nextInput();
    }
}

TSDUCKPY void tspyInputSwitcherPreviousInput(void* pyobj)
{
    ts::InputSwitcher* isw = reinterpret_cast<ts::InputSwitcher*>(pyobj);
    if (isw != nullptr) {
        isw->previousInput();
    }
}

TSDUCKPY size_t tspyInputSwitcherCurrentInput(void* pyobj)
{
    ts::InputSwitcher* isw = reinterpret_cast<ts::InputSwitcher*>(pyobj);
    return isw == nullptr ? 0 : isw->currentInput();
}

//-----------------------------------------------------------------------------
// Start the input switcher and decode arguments.
//-----------------------------------------------------------------------------

TSDUCKPY bool tspyStartInputSwitcher(void* pyobj, const tspyInputSwitcherArgs* pyargs)
{
    ts::InputSwitcher* isw = reinterpret_cast<ts::InputSwitcher*>(pyobj);
    if (isw == nullptr || pyargs == nullptr) {
        return false;
    }

    // Build InputSwitcher arguments.
    ts::InputSwitcherArgs args;
    args.terminate = bool(pyargs->terminate);
    args.fastSwitch = bool(pyargs->fast_switch);
    args.delayedSwitch = bool(pyargs->delayed_switch);
    args.reusePort = bool(pyargs->reuse_port);
    args.firstInput = size_t(std::max<long>(0, pyargs->first_input));
    args.primaryInput = pyargs->primary_input < 0 ? ts::NPOS : size_t(pyargs->primary_input);
    args.cycleCount = size_t(std::max<long>(0, pyargs->cycle_count));
    args.bufferedPackets = size_t(std::max<long>(0, pyargs->buffered_packets));
    args.maxInputPackets = size_t(std::max<long>(0, pyargs->max_input_packets));
    args.maxOutputPackets = size_t(std::max<long>(0, pyargs->max_output_packets));
    args.sockBuffer = size_t(std::max<long>(0, pyargs->sock_buffer));
    args.receiveTimeout = ts::MilliSecond(std::max<long>(0, pyargs->receive_timeout));
    if (pyargs->remote_server_port > 0 && pyargs->remote_server_port < 0xFFFF) {
        args.remoteServer.setPort(uint16_t(pyargs->remote_server_port));
    }
    args.eventCommand = ts::py::ToString(pyargs->event_command, pyargs->event_command_size);
    ts::UString addr(ts::py::ToString(pyargs->event_udp_addr, pyargs->event_udp_addr_size));
    if (!addr.empty() && !args.eventUDP.resolve(addr, isw->report())) {
        return false;
    }
    if (pyargs->event_udp_port > 0 && pyargs->event_udp_port < 0xFFFF) {
        args.eventUDP.setPort(uint16_t(pyargs->event_udp_port));
    }
    addr = ts::py::ToString(pyargs->local_addr, pyargs->local_addr_size);
    if (!addr.empty() && !args.eventLocalAddress.resolve(addr, isw->report())) {
        return false;
    }
    args.eventTTL = int(pyargs->event_ttl);

    // Default output plugins.
    args.output.set(u"drop");

    // Split plugins strings.
    const ts::UStringList fields(ts::py::ToStringList(pyargs->plugins, pyargs->plugins_size));

    // Analyze list of strings.
    auto it = fields.begin();
    if (it != fields.end() && !it->startWith(u"-")) {
        // First element is application name.
        args.appName = *it++;
    }
    ts::PluginOptions* current = nullptr;
    for (; it != fields.end(); ++it) {
        if (*it == u"-O") {
            current = &args.output;
            current->clear();
            continue;
        }
        else if (*it == u"-I") {
            args.inputs.resize(args.inputs.size() + 1);
            current = &args.inputs.back();
            current->clear();
            continue;
        }
        if (current == nullptr) {
            isw->report().error(u"unexpected argument '%s'", {*it});
            return false;
        }
        if (current->name.empty()) {
            current->name = *it;
        }
        else {
            current->args.push_back(*it);
        }
    }

    // Fix missing default values.
    args.enforceDefaults();

    // Finally start the InputSwitcher.
    return isw->start(args);
}
