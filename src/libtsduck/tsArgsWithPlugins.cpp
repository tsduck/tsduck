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
//  Transport stream processor command-line options
//
//----------------------------------------------------------------------------

#include "tsArgsWithPlugins.h"
#include "tsDuckConfigFile.h"
#include "tsSysUtils.h"
#include "tsPlugin.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ArgsWithPlugins::ArgsWithPlugins(size_t min_inputs,
                                     size_t max_inputs,
                                     size_t min_plugins,
                                     size_t max_plugins,
                                     size_t min_outputs,
                                     size_t max_outputs,
                                     const ts::UString& description,
                                     const ts::UString& syntax,
                                     int flags) :
    Args(description, syntax, flags),
    inputs(),
    plugins(),
    outputs(),
    _min_inputs(min_inputs),
    _max_inputs(max_inputs),
    _min_plugins(min_plugins),
    _max_plugins(max_plugins),
    _min_outputs(min_outputs),
    _max_outputs(max_outputs)
{
}


//----------------------------------------------------------------------------
// Analyze the command line.
//----------------------------------------------------------------------------

bool ts::ArgsWithPlugins::analyze(int argc, char* argv[], bool processRedirections)
{
    // Load arguments.
    const UString app_name(argc > 0 ? BaseName(UString::FromUTF8(argv[0]), TS_EXECUTABLE_SUFFIX) : UString());
    UStringVector args;
    if (argc > 1) {
        UString::Assign(args, argc - 1, argv + 1);
    }

    return analyze(app_name, args, processRedirections);
}

bool ts::ArgsWithPlugins::analyze(const ts::UString& app_name, const ts::UStringVector& arguments, bool processRedirections)
{
    // Clear plugins.
    inputs.clear();
    plugins.clear();
    outputs.clear();

    // Process redirections.
    ts::UStringVector args(arguments);
    if (processRedirections && !processArgsRedirection(args)) {
        return false;
    }

    // Locate the first processor option. All preceeding options are command-specific options and must be analyzed.
    PluginType plugin_type = PROCESSOR_PLUGIN;
    PluginOptionsVector* options = nullptr;
    size_t plugin_index = nextProcOpt(args, 0, plugin_type, options);

    // Analyze the command-specifc options, not including the plugin options, not processing redirections.
    if (!Args::analyze(app_name, UStringVector(args.begin(), args.begin() + plugin_index), false)) {
        return false;
    }

    // Locate all plugins.
    inputs.reserve(args.size());
    plugins.reserve(args.size());
    outputs.reserve(args.size());

    while (plugin_index < args.size()) {

        // Check that a plugin name is present after the processor option.
        if (plugin_index + 1 >= args.size()) {
            error(u"missing plugin name for option %s", {args[plugin_index]});
            break;
        }

        // Record plugin name and parameters.
        options->resize(options->size() + 1);
        PluginOptions& opt((*options)[options->size() - 1]);
        opt.type = plugin_type;
        opt.name = args[plugin_index + 1];
        opt.args.clear();

        // Search for next plugin.
        const size_t start = plugin_index;
        plugin_index = nextProcOpt(args, plugin_index + 2, plugin_type, options);

        // Now set options of previous plugin.
        opt.args.insert(opt.args.begin(), args.begin() + start + 2, args.begin() + plugin_index);
    }

    // Load default plugins.
    loadDefaultPlugins(INPUT_PLUGIN, u"default.input", inputs);
    loadDefaultPlugins(PROCESSOR_PLUGIN, u"default.plugin", plugins);
    loadDefaultPlugins(OUTPUT_PLUGIN, u"default.output", outputs);

    // Check min and max number of occurences of each plugin type.
    if (inputs.size() < _min_inputs) {
        error(u"not enough input plugins, need at least %d", {_min_inputs});
        return false;
    }
    if (outputs.size() < _min_outputs) {
        error(u"not enough output plugins, need at least %d", {_min_outputs});
        return false;
    }
    if (plugins.size() < _min_plugins) {
        error(u"not enough packet processor plugins, need at least %d", {_min_plugins});
        return false;
    }
    if (inputs.size() > _max_inputs) {
        error(u"too many input plugins, need at most %d", {_max_inputs});
        return false;
    }
    if (outputs.size() > _max_outputs) {
        error(u"too many output plugins, need at most %d", {_max_outputs});
        return false;
    }
    if (plugins.size() > _max_plugins) {
        error(u"too many packet processor plugins, need at most %d", {_max_plugins});
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Search the next plugin option.
//----------------------------------------------------------------------------

size_t ts::ArgsWithPlugins::nextProcOpt(const UStringVector& args, size_t index, PluginType& type, PluginOptionsVector*& opts)
{
    while (index < args.size()) {
        const UString& arg(args[index]);
        if (arg == u"-I" || arg == u"--input") {
            type = INPUT_PLUGIN;
            opts = &inputs;
            return index;
        }
        if (arg == u"-O" || arg == u"--output") {
            type = OUTPUT_PLUGIN;
            opts = &outputs;
            return index;
        }
        if (arg == u"-P" || arg == u"--processor") {
            type = PROCESSOR_PLUGIN;
            opts = &plugins;
            return index;
        }
        index++;
    }
    opts = nullptr;
    return std::min(args.size(), index);
}


//----------------------------------------------------------------------------
// Load default list of plugins by type.
//----------------------------------------------------------------------------

void ts::ArgsWithPlugins::loadDefaultPlugins(PluginType type, const ts::UString& entry, ts::PluginOptionsVector& options)
{
    // Get default plugins only when none where specified.
    if (options.empty()) {
        UStringVector lines;
        DuckConfigFile::Instance()->getValues(entry, lines);
        for (size_t i = 0; i < lines.size(); ++i) {
            // Got one plugin specification. Parse its arguments.
            PluginOptions opt(type);
            lines[i].splitShellStyle(opt.args);
            if (!opt.args.empty()) {
                // Found a complete plugin spec.
                opt.name = opt.args.front();
                opt.args.erase(opt.args.begin());
                options.push_back(opt);
            }
        }
    }
}
