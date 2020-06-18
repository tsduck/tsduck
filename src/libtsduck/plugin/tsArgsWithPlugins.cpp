//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
    _min_inputs(min_inputs),
    _max_inputs(max_inputs),
    _min_plugins(min_plugins),
    _max_plugins(max_plugins),
    _min_outputs(min_outputs),
    _max_outputs(max_outputs),
    _plugins()
{
}


//----------------------------------------------------------------------------
// Get plugins of a given type, after command line analysis.
//----------------------------------------------------------------------------

size_t ts::ArgsWithPlugins::pluginCount(PluginType type) const
{
    const auto it = _plugins.find(type);
    return it == _plugins.end() ? 0 : it->second.size();
}

void ts::ArgsWithPlugins::getPlugins(PluginOptionsVector& plugins, PluginType type) const
{
    const auto it = _plugins.find(type);
    if (it == _plugins.end()) {
        plugins.clear();
    }
    else {
        plugins = it->second;
    }
}

void ts::ArgsWithPlugins::getPlugin(PluginOptions& plugin, PluginType type, const UChar* def_value, size_t index) const
{
    const auto it = _plugins.find(type);
    if (it == _plugins.end() || index >= it->second.size()) {
        // Index is not valid.
        plugin.name = def_value;
        plugin.args.clear();
    }
    else {
        plugin = it->second[index];
    }
}


//----------------------------------------------------------------------------
// Analyze the command line.
//----------------------------------------------------------------------------

bool ts::ArgsWithPlugins::analyze(const UString& command, bool processRedirections)
{
    // Call superclass which, in turn, calls our virtual analyze(UString, UStringVector, bool).
    return Args::analyze(command, processRedirections);
}

bool ts::ArgsWithPlugins::analyze(int argc, char* argv[], bool processRedirections)
{
    // Call superclass which, in turn, calls our virtual analyze(UString, UStringVector, bool).
    return Args::analyze(argc, argv, processRedirections);
}

bool ts::ArgsWithPlugins::analyze(const UString& app_name, const UStringVector& arguments, bool processRedirections)
{
    // Clear plugins.
    _plugins.clear();

    // Process redirections.
    ts::UStringVector args(arguments);
    if (processRedirections && !processArgsRedirection(args)) {
        return false;
    }

    // Locate the first processor option. All preceeding options are command-specific options and must be analyzed.
    PluginType plugin_type = PluginType::PROCESSOR;
    size_t plugin_index = nextProcOpt(args, 0, plugin_type);

    // Analyze the command-specifc options, not including the plugin options, not processing redirections.
    if (!Args::analyze(app_name, UStringVector(args.begin(), args.begin() + plugin_index), false)) {
        return false;
    }

    // Locate all plugins.
    while (plugin_index < args.size()) {

        // Check that a plugin name is present after the processor option.
        if (plugin_index + 1 >= args.size()) {
            error(u"missing plugin name for option %s", {args[plugin_index]});
            break;
        }

        // Reference to current list of plugins of that type.
        PluginOptionsVector& options(_plugins[plugin_type]);

        // Record plugin name and parameters.
        options.resize(options.size() + 1);
        PluginOptions& opt(options[options.size() - 1]);
        opt.name = args[plugin_index + 1];
        opt.args.clear();

        // Search for next plugin.
        const size_t start = plugin_index;
        plugin_index = nextProcOpt(args, plugin_index + 2, plugin_type);

        // Now set options of previous plugin.
        opt.args.insert(opt.args.begin(), args.begin() + start + 2, args.begin() + plugin_index);
    }

    // Load default plugins.
    loadDefaultPlugins(PluginType::INPUT, u"default.input");
    loadDefaultPlugins(PluginType::PROCESSOR, u"default.plugin");
    loadDefaultPlugins(PluginType::OUTPUT, u"default.output");

    const size_t in_count = pluginCount(PluginType::INPUT);
    const size_t proc_count = pluginCount(PluginType::PROCESSOR);
    const size_t out_count = pluginCount(PluginType::OUTPUT);

    // Check min and max number of occurences of each plugin type.
    if (in_count < _min_inputs) {
        error(u"not enough input plugins, need at least %d", {_min_inputs});
        return false;
    }
    if (out_count < _min_outputs) {
        error(u"not enough output plugins, need at least %d", {_min_outputs});
        return false;
    }
    if (proc_count < _min_plugins) {
        error(u"not enough packet processor plugins, need at least %d", {_min_plugins});
        return false;
    }
    if (in_count > _max_inputs) {
        error(u"too many input plugins, need at most %d", {_max_inputs});
        return false;
    }
    if (out_count > _max_outputs) {
        error(u"too many output plugins, need at most %d", {_max_outputs});
        return false;
    }
    if (proc_count > _max_plugins) {
        error(u"too many packet processor plugins, need at most %d", {_max_plugins});
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Search the next plugin option.
//----------------------------------------------------------------------------

size_t ts::ArgsWithPlugins::nextProcOpt(const UStringVector& args, size_t index, PluginType& type)
{
    while (index < args.size()) {
        const UString& arg(args[index]);
        if (arg == u"-I" || arg == u"--input") {
            type = PluginType::INPUT;
            return index;
        }
        if (arg == u"-O" || arg == u"--output") {
            type = PluginType::OUTPUT;
            return index;
        }
        if (arg == u"-P" || arg == u"--processor") {
            type = PluginType::PROCESSOR;
            return index;
        }
        index++;
    }
    return std::min(args.size(), index);
}


//----------------------------------------------------------------------------
// Load default list of plugins by type.
//----------------------------------------------------------------------------

void ts::ArgsWithPlugins::loadDefaultPlugins(PluginType type, const ts::UString& entry)
{
    // Reference to current list of plugins of that type.
    PluginOptionsVector& options(_plugins[type]);

    // Get default plugins only when none where specified for that type.
    if (options.empty()) {
        UStringVector lines;
        DuckConfigFile::Instance()->getValues(entry, lines);
        // Loop on all default plugins for that type.
        for (size_t i = 0; i < lines.size(); ++i) {
            // Got one plugin specification. Parse its arguments.
            PluginOptions opt;
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
