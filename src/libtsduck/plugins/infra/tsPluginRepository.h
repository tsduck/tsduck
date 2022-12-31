//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!
//!  @file
//!  TSP plugin repository
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"
#include "tsProcessorPlugin.h"
#include "tsOutputPlugin.h"
#include "tsReport.h"
#include "tsSingletonManager.h"
#include "tsVersionInfo.h"

namespace ts {
    //!
    //! A repository of TSP plugins, either statically or dynamically linked.
    //! @ingroup plugin
    //!
    //! This class is a singleton. Use static Instance() method to access the single instance.
    //!
    class TSDUCKDLL PluginRepository
    {
        TS_DECLARE_SINGLETON(PluginRepository);

    public:
        //!
        //! Profile of a function which creates an input plugin.
        //! @param [in] tsp Associated callback to tsp.
        //! @return A new allocated object implementing ts::InputPlugin.
        //!
        typedef InputPlugin* (*InputPluginFactory)(TSP* tsp);

        //!
        //! Profile of a function which creates an output plugin.
        //! @param [in] tsp Associated callback to tsp.
        //! @return A new allocated object implementing ts::OutputPlugin.
        //!
        typedef OutputPlugin* (*OutputPluginFactory)(TSP* tsp);

        //!
        //! Profile of a function which creates a packet processor plugin.
        //! @param [in] tsp Associated callback to tsp.
        //! @return A new allocated object implementing ts::ProcessorPlugin.
        //!
        typedef ProcessorPlugin* (*ProcessorPluginFactory)(TSP* tsp);

        //!
        //! Allow or disallow the loading of plugins from shareable objects.
        //! When disabled, only statically registered plugins are allowed.
        //! Loading is initially enabled by default.
        //! @param [in] allowed When true, dynamically loading plugins is allowed.
        //!
        void setSharedLibraryAllowed(bool allowed) { _sharedLibraryAllowed = allowed; }

        //!
        //! Register an input plugin.
        //! @param [in] name Plugin name.
        //! @param [in] allocator New input plugin allocator function. Ignored when zero.
        //!
        void registerInput(const UString& name, InputPluginFactory allocator);

        //!
        //! Register a packet processor plugin.
        //! @param [in] name Plugin name.
        //! @param [in] allocator New packet processor plugin allocator function. Ignored when zero.
        //!
        void registerProcessor(const UString& name, ProcessorPluginFactory allocator);

        //!
        //! Register an output plugin.
        //! @param [in] name Plugin name.
        //! @param [in] allocator New output plugin allocator function. Ignored when zero.
        //!
        void registerOutput(const UString& name, OutputPluginFactory allocator);

        //!
        //! Get an input plugin by name.
        //! If not found and loading shareable library is allowed, try to load it.
        //! @param [in] name Plugin name.
        //! @param [in,out] report Where to report errors.
        //! @return Plugin allocator function. Zero when not found.
        //!
        InputPluginFactory getInput(const UString& name, Report& report);

        //!
        //! Get a packet processor plugin by name.
        //! If not found and loading shareable library is allowed, try to load it.
        //! @param [in] name Plugin name.
        //! @param [in,out] report Where to report errors.
        //! @return Plugin allocator function. Zero when not found.
        //!
        ProcessorPluginFactory getProcessor(const UString& name, Report& report);

        //!
        //! Get an output plugin by name.
        //! If not found and loading shareable library is allowed, try to load it.
        //! @param [in] name Plugin name.
        //! @param [in,out] report Where to report errors.
        //! @return Plugin allocator function. Zero when not found.
        //!
        OutputPluginFactory getOutput(const UString& name, Report& report);

        //!
        //! Get the number of registered input plugins.
        //! @return The number of registered input plugins.
        //!
        size_t inputCount() const { return _inputPlugins.size(); }

        //!
        //! Get the number of registered processor plugins.
        //! @return The number of registered processor plugins.
        //!
        size_t processorCount() const { return _processorPlugins.size(); }

        //!
        //! Get the number of registered output plugins.
        //! @return The number of registered output plugins.
        //!
        size_t outputCount() const { return _outputPlugins.size(); }

        //!
        //! Get the names of all registered input plugins.
        //! @return The names of all registered input plugins.
        //!
        UStringList inputNames() const;

        //!
        //! Get the names of all registered packet processor plugins.
        //! @return The names of all registered packet processor plugins.
        //!
        UStringList processorNames() const;

        //!
        //! Get the names of all registered output plugins.
        //! @return The names of all registered output plugins.
        //!
        UStringList outputNames() const;

        //!
        //! Load all available tsp processors.
        //! Does nothing when dynamic loading of plugins is disabled.
        //! @param [in,out] report Where to report errors.
        //!
        void loadAllPlugins(Report& report);

        //!
        //! Flags for listPlugins().
        //!
        enum ListFlags {
            LIST_INPUT   = 0x0001,  //!< List input plugins.
            LIST_PACKET  = 0x0002,  //!< List packet processor plugins.
            LIST_OUTPUT  = 0x0004,  //!< List output plugins.
            LIST_COMPACT = 0x0010,  //!< Compact output.
            LIST_NAMES   = 0x0020,  //!< Names only.
            LIST_ALL     = LIST_INPUT | LIST_PACKET | LIST_OUTPUT,  //!< List all plugins.
        };

        //!
        //! Convenient command line options for "list processor" option.
        //!
        static const Enumeration ListProcessorEnum;

        //!
        //! List all tsp processors.
        //! This function is typically used to implement the <code>tsp -\-list-processors</code> option.
        //! @param [in] loadAll When true, all available plugins are loaded first.
        //! Ignored when dynamic loading of plugins is disabled.
        //! @param [in,out] report Where to report errors.
        //! @param [in] flags List options, an or'ed mask of ListFlags values.
        //! @return The text to display.
        //!
        UString listPlugins(bool loadAll, Report& report, int flags = LIST_ALL);

        //!
        //! A class to register plugins.
        //!
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSDUCKDLL Register
        {
            TS_NOBUILD_NOCOPY(Register);
        public:
            //!
            //! The constructor registers an input plugin.
            //! @param [in] name Plugin name.
            //! @param [in] allocator New input plugin allocator function.
            //!
            Register(const UString& name, InputPluginFactory allocator);

            //!
            //! The constructor registers a packet processor plugin.
            //! @param [in] name Plugin name.
            //! @param [in] allocator New packet processor plugin allocator function.
            //!
            Register(const UString& name, ProcessorPluginFactory allocator);

            //!
            //! The constructor registers an output plugin.
            //! @param [in] name Plugin name.
            //! @param [in] allocator New output plugin allocator function.
            //!
            Register(const UString& name, OutputPluginFactory allocator);
        };

    private:
        typedef std::map<UString, InputPluginFactory>     InputMap;
        typedef std::map<UString, ProcessorPluginFactory> ProcessorMap;
        typedef std::map<UString, OutputPluginFactory>    OutputMap;

        bool         _sharedLibraryAllowed;
        InputMap     _inputPlugins;
        ProcessorMap _processorPlugins;
        OutputMap    _outputPlugins;

        template<typename FACTORY>
        FACTORY getFactory(const UString& name, const UString& type, const std::map<UString,FACTORY>&, Report&);

        // List one plugin.
        static void ListOnePlugin(UString& out, const UString& name, Plugin* plugin, size_t name_width, int flags);
    };
}

//
// Implementation note: Take care before modifying the following macros.
// Especially, these macros need to be defined on one single line
// each because of the use of __LINE__ to create unique identifiers.
//
//! @cond nodoxygen
#define _TS_PLUGIN_FACTORY(funcname,classname,suffix) namespace { ts::suffix##Plugin* funcname(ts::TSP* tsp) {return new classname(tsp);} }
#define _TS_REGISTER_PLUGIN(name,classname,suffix) \
    TS_LIBCHECK(); \
    _TS_PLUGIN_FACTORY(TS_UNIQUE_NAME(_F),classname,suffix) static ts::PluginRepository::Register TS_UNIQUE_NAME(_R)(name,&TS_UNIQUE_NAME(_F))
//! @endcond

//!
//! Register an input plugin class in the plugin repository.
//! @param name Plugin name.
//! @param classname Name of a subclass of ts::InputPlugin implementing the plugin.
//! @hideinitializer
//!
#define TS_REGISTER_INPUT_PLUGIN(name,classname) _TS_REGISTER_PLUGIN(name,classname,Input)

//!
//! Register an output plugin class in the plugin repository.
//! @param name Plugin name.
//! @param classname Name of a subclass of ts::OutputPlugin implementing the plugin.
//! @hideinitializer
//!
#define TS_REGISTER_OUTPUT_PLUGIN(name,classname) _TS_REGISTER_PLUGIN(name,classname,Output)

//!
//! Register a packet processing plugin class in the plugin repository.
//! @param name Plugin name.
//! @param classname Name of a subclass of ts::ProcessorPlugin implementing the plugin.
//! @hideinitializer
//!
#define TS_REGISTER_PROCESSOR_PLUGIN(name,classname) _TS_REGISTER_PLUGIN(name,classname,Processor)

//
// Compatibility macros for old plugins.
//
//! @cond nodoxygen
#define TSPLUGIN_DECLARE_VERSION
#define TSPLUGIN_DECLARE_INPUT(name,classname) TS_REGISTER_INPUT_PLUGIN(u ## #name, classname);
#define TSPLUGIN_DECLARE_OUTPUT(name,classname) TS_REGISTER_OUTPUT_PLUGIN(u ## #name, classname);
#define TSPLUGIN_DECLARE_PROCESSOR(name,classname) TS_REGISTER_PROCESSOR_PLUGIN(u ## #name, classname);
//! @endcond
