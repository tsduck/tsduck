//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tspyTSProcessor.h"
#include "tsTSProcessor.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Interface to TSProcessor.
//-----------------------------------------------------------------------------

void* tspyNewTSProcessor(void* report)
{
    ts::Report* rep = reinterpret_cast<ts::Report*>(report);
    return new ts::TSProcessor(rep == nullptr ? NULLREP : *rep);
}

void tspyDeleteTSProcessor(void* tsp)
{
    delete reinterpret_cast<ts::TSProcessor*>(tsp);
}

void tspyAbortTSProcessor(void* tsp)
{
    ts::TSProcessor* proc = reinterpret_cast<ts::TSProcessor*>(tsp);
    if (proc != nullptr) {
        proc->abort();
    }
}

void tspyWaitTSProcessor(void* tsp)
{
    ts::TSProcessor* proc = reinterpret_cast<ts::TSProcessor*>(tsp);
    if (proc != nullptr) {
        proc->waitForTermination();
    }
}


//-----------------------------------------------------------------------------
// Start the TS processing and decode arguments.
//-----------------------------------------------------------------------------

bool tspyStartTSProcessor(void* tsp, const tspyTSProcessorArgs* args, const uint8_t* plugins, size_t plugins_size)
{
    ts::TSProcessor* proc = reinterpret_cast<ts::TSProcessor*>(tsp);
    if (proc == nullptr || args == nullptr) {
        return false;
    }

    // Build TSProcessor arguments.
    ts::TSProcessorArgs tsargs;
    tsargs.monitor = bool(args->monitor);
    tsargs.ignore_jt = bool(args->ignore_joint_termination);
    tsargs.ts_buffer_size = args->buffer_size == 0 ? ts::TSProcessorArgs::DEFAULT_BUFFER_SIZE : size_t(args->buffer_size);
    tsargs.max_flush_pkt = size_t(args->max_flushed_packets);
    tsargs.max_input_pkt = size_t(args->max_input_packets);
    tsargs.init_input_pkt = size_t(args->initial_input_packets);
    tsargs.instuff_nullpkt = size_t(args->add_input_stuffing_0);
    tsargs.instuff_inpkt = size_t(args->add_input_stuffing_1);
    tsargs.instuff_start = size_t(args->add_start_stuffing);
    tsargs.instuff_stop = size_t(args->add_stop_stuffing);
    tsargs.fixed_bitrate = ts::BitRate(args->bitrate);
    tsargs.bitrate_adj = ts::MilliSecond(args->bitrate_adjust_interval);
    tsargs.receive_timeout = ts::MilliSecond(args->receive_timeout);

    // Split plugins strings.
    const ts::UStringList fields(tspy::ToStringList(plugins, plugins_size));

    // Analyze list of strings.
    auto it = fields.begin();
    if (it != fields.end() && !it->startWith(u"-")) {
        // First element is application name.
        tsargs.app_name = *it++;
    }
    ts::PluginOptions* current = nullptr;
    for (; it != fields.end(); ++it) {
        if (*it == u"-I") {
            current = &tsargs.input;
            current->clear();
            continue;
        }
        else if (*it == u"-O") {
            current = &tsargs.output;
            current->clear();
            continue;
        }
        else if (*it == u"-P") {
            tsargs.plugins.resize(tsargs.plugins.size() + 1);
            current = &tsargs.plugins.back();
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

    // Debug message.
    if (proc->report().debug()) {
        ts::UString cmd(tsargs.app_name);
        cmd.append(u" ");
        cmd.append(tsargs.input.toString(ts::PluginType::INPUT));
        for (auto it2 = tsargs.plugins.begin(); it2 != tsargs.plugins.end(); ++it2) {
            cmd.append(u" ");
            cmd.append(it2->toString(ts::PluginType::PROCESSOR));
        }
        cmd.append(u" ");
        cmd.append(tsargs.output.toString(ts::PluginType::OUTPUT));
        proc->report().debug(u"starting: %s", {cmd});
    }

    // Finally start the TSProcessor.
    return proc->start(tsargs);
}
