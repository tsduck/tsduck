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
//  TSDuck Python bindings: encapsulates TSProcessor objects for Python.
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tsTSProcessor.h"
#include "tsNullReport.h"

TS_MSC_NOWARNING(4091) // '__declspec(dllexport)': ignored on left of 'struct type' when no variable is declared

//
// Argument structure (plain C structure) for start parameters.
// Use same names as ts::TSProcessorArgs but only long's to avoid interface issues.
//
TSDUCKPY struct tspyTSProcessorArgs
{
    long ignore_joint_termination; // Ignore "joint termination" options in plugins (bool).
    long buffer_size;              // Size in bytes of the global TS packet buffer.
    long max_flushed_packets;      // Max processed packets before flush.
    long max_input_packets;        // Max packets per input operation.
    long max_output_packets;       // Max packets per output operation.
    long initial_input_packets;    // Initial number of input packets to read before starting the processing (zero means default).
    long add_input_stuffing_0;     // Add input stuffing: add @a add_input_stuffing_0 null packets ...
    long add_input_stuffing_1;     // ... every @a add_input_stuffing_1 input packets.
    long add_start_stuffing;       // Add null packets before actual input.
    long add_stop_stuffing;        // Add null packets after end of actual input.
    long bitrate;                  // Fixed input bitrate (user-specified).
    long bitrate_adjust_interval;  // Bitrate adjust interval in (milliseconds).
    long receive_timeout;          // Timeout on input operations (in milliseconds).
    long log_plugin_index;         // Log plugin index with plugin name (bool).
    const uint8_t* plugins;        // Address of UTF-16 multi-strings buffer for plugins.
    size_t plugins_size;           // Size in bytes of plugins multi-strings buffer.
};

//-----------------------------------------------------------------------------
// Interface to TSProcessor.
//-----------------------------------------------------------------------------

TSDUCKPY void* tspyNewTSProcessor(void* report)
{
    ts::Report* rep = reinterpret_cast<ts::Report*>(report);
    return new ts::TSProcessor(rep == nullptr ? NULLREP : *rep);
}

TSDUCKPY void tspyDeleteTSProcessor(void* tsp)
{
    delete reinterpret_cast<ts::TSProcessor*>(tsp);
}

TSDUCKPY void tspyAbortTSProcessor(void* tsp)
{
    ts::TSProcessor* proc = reinterpret_cast<ts::TSProcessor*>(tsp);
    if (proc != nullptr) {
        proc->abort();
    }
}

TSDUCKPY void tspyWaitTSProcessor(void* tsp)
{
    ts::TSProcessor* proc = reinterpret_cast<ts::TSProcessor*>(tsp);
    if (proc != nullptr) {
        proc->waitForTermination();
    }
}

//-----------------------------------------------------------------------------
// Start the TS processing and decode arguments.
//-----------------------------------------------------------------------------

TSDUCKPY bool tspyStartTSProcessor(void* tsp, const tspyTSProcessorArgs* pyargs)
{
    ts::TSProcessor* proc = reinterpret_cast<ts::TSProcessor*>(tsp);
    if (proc == nullptr || pyargs == nullptr) {
        return false;
    }

    // Build TSProcessor arguments.
    ts::TSProcessorArgs args;
    args.ignore_jt = bool(pyargs->ignore_joint_termination);
    args.ts_buffer_size = pyargs->buffer_size == 0 ? ts::TSProcessorArgs::DEFAULT_BUFFER_SIZE : size_t(pyargs->buffer_size);
    args.max_flush_pkt = size_t(pyargs->max_flushed_packets);
    args.max_input_pkt = size_t(pyargs->max_input_packets);
    args.max_output_pkt = pyargs->max_output_packets == 0 ? ts::NPOS : size_t(pyargs->max_output_packets);
    args.init_input_pkt = size_t(pyargs->initial_input_packets);
    args.instuff_nullpkt = size_t(pyargs->add_input_stuffing_0);
    args.instuff_inpkt = size_t(pyargs->add_input_stuffing_1);
    args.instuff_start = size_t(pyargs->add_start_stuffing);
    args.instuff_stop = size_t(pyargs->add_stop_stuffing);
    args.fixed_bitrate = ts::BitRate(pyargs->bitrate);
    args.bitrate_adj = ts::MilliSecond(pyargs->bitrate_adjust_interval);
    args.receive_timeout = ts::MilliSecond(pyargs->receive_timeout);
    args.log_plugin_index = bool(pyargs->log_plugin_index);

    // Default input and output plugins.
    args.input.set(u"null");
    args.output.set(u"drop");

    // Split plugins strings.
    const ts::UStringList fields(ts::py::ToStringList(pyargs->plugins, pyargs->plugins_size));

    // Analyze list of strings.
    auto it = fields.begin();
    if (it != fields.end() && !it->startWith(u"-")) {
        // First element is application name.
        args.app_name = *it++;
    }
    ts::PluginOptions* current = nullptr;
    for (; it != fields.end(); ++it) {
        if (*it == u"-I") {
            current = &args.input;
            current->clear();
            continue;
        }
        else if (*it == u"-O") {
            current = &args.output;
            current->clear();
            continue;
        }
        else if (*it == u"-P") {
            args.plugins.resize(args.plugins.size() + 1);
            current = &args.plugins.back();
            current->clear();
            continue;
        }
        if (current == nullptr) {
            proc->report().error(u"unexpected argument '%s'", {*it});
            return false;
        }
        if (current->name.empty()) {
            current->name = *it;
        }
        else {
            current->args.push_back(*it);
        }
    }

    // Apply default values when unspecified.
    args.applyDefaults(true);

    // Debug message.
    if (proc->report().debug()) {
        ts::UString cmd(args.app_name);
        cmd.append(u" ");
        cmd.append(args.input.toString(ts::PluginType::INPUT));
        for (const auto& it2 : args.plugins) {
            cmd.append(u" ");
            cmd.append(it2.toString(ts::PluginType::PROCESSOR));
        }
        cmd.append(u" ");
        cmd.append(args.output.toString(ts::PluginType::OUTPUT));
        proc->report().debug(u"starting: %s", {cmd});
    }

    // Finally start the TSProcessor.
    return proc->start(args);
}
