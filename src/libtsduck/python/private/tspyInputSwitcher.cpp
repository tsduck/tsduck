//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    args.fast_switch = bool(pyargs->fast_switch);
    args.delayed_switch = bool(pyargs->delayed_switch);
    args.remote_control.reuse_port = bool(pyargs->reuse_port);
    args.first_input = size_t(std::max<long>(0, pyargs->first_input));
    args.primary_input = pyargs->primary_input < 0 ? ts::NPOS : size_t(pyargs->primary_input);
    args.cycle_count = size_t(std::max<long>(0, pyargs->cycle_count));
    args.buffered_packets = size_t(std::max<long>(0, pyargs->buffered_packets));
    args.max_input_packets = size_t(std::max<long>(0, pyargs->max_input_packets));
    args.max_output_packets = size_t(std::max<long>(0, pyargs->max_output_packets));
    args.sock_buffer_size = size_t(std::max<long>(0, pyargs->sock_buffer));
    args.receive_timeout = cn::milliseconds(cn::milliseconds::rep(std::max<long>(0, pyargs->receive_timeout)));
    if (pyargs->remote_server_port > 0 && pyargs->remote_server_port < 0xFFFF) {
        args.remote_control.server_addr.setPort(uint16_t(pyargs->remote_server_port));
    }
    args.event_command = ts::py::ToString(pyargs->event_command, pyargs->event_command_size);
    ts::UString addr(ts::py::ToString(pyargs->event_udp_addr, pyargs->event_udp_addr_size));
    if (!addr.empty() && !args.event_udp.resolve(addr, isw->report())) {
        return false;
    }
    if (pyargs->event_udp_port > 0 && pyargs->event_udp_port < 0xFFFF) {
        args.event_udp.setPort(uint16_t(pyargs->event_udp_port));
    }
    addr = ts::py::ToString(pyargs->local_addr, pyargs->local_addr_size);
    if (!addr.empty() && !args.event_local_address.resolve(addr, isw->report())) {
        return false;
    }
    args.event_ttl = int(pyargs->event_ttl);

    // Default output plugins.
    args.output.set(u"drop");

    // Split plugins strings.
    const ts::UStringList fields(ts::py::ToStringList(pyargs->plugins, pyargs->plugins_size));

    // Analyze list of strings.
    auto it = fields.begin();
    if (it != fields.end() && !it->starts_with(u"-")) {
        // First element is application name.
        args.app_name = *it++;
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
            isw->report().error(u"unexpected argument '%s'", *it);
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
