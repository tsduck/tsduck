//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for commands with plugins.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsPluginOptions.h"

namespace ts {
    //!
    //! Command line arguments for commands with plugins.
    //!
    //! The command line is analyzed, keeping command-specific options and
    //! plugin descriptions apart.
    //!
    //! The option -\-list-plugins is automatically added and processed.
    //!
    //! @ingroup plugin
    //!
    class TSDUCKDLL ArgsWithPlugins: public Args
    {
        TS_NOCOPY(ArgsWithPlugins);
    public:
        //!
        //! Constructor.
        //! @param [in] min_inputs Minimum number of input plugins.
        //! @param [in] max_inputs Maximum number of input plugins.
        //! @param [in] min_plugins Minimum number of packet processor plugins.
        //! @param [in] max_plugins Maximum number of packet processor plugins.
        //! @param [in] min_outputs Minimum number of output plugins.
        //! @param [in] max_outputs Maximum number of output plugins.
        //! @param [in] description A short one-line description.
        //! @param [in] syntax A short one-line syntax summary.
        //! @param [in] flags An or'ed mask of Flags values.
        //!
        ArgsWithPlugins(size_t min_inputs = 0,
                        size_t max_inputs = UNLIMITED_COUNT,
                        size_t min_plugins = 0,
                        size_t max_plugins = UNLIMITED_COUNT,
                        size_t min_outputs = 0,
                        size_t max_outputs = UNLIMITED_COUNT,
                        const UString& description = UString(),
                        const UString& syntax = UString(),
                        int flags = 0);

        // Inherited methods.
        virtual bool analyze(const UString& command, bool processRedirections = true) override;
        virtual bool analyze(int argc, char* argv[], bool processRedirections = true) override;
        virtual bool analyze(const UString& app_name, const UStringVector& arguments, bool processRedirections = true) override;
        virtual UString getHelpText(HelpFormat format, size_t line_width = DEFAULT_LINE_WIDTH) const override;
        virtual void setSyntax(const UString& syntax) override;

        //!
        //! Get the number of plugins of a given type, after command line analysis.
        //! @param [in] type Type of plugin to find.
        //! @return The number of plugins of that type.
        //!
        size_t pluginCount(PluginType type) const;

        //!
        //! Get a plugin of a given type, after command line analysis.
        //! @param [out] plugin Returned name and arguments of the plugin.
        //! @param [in] type Type of plugin to find.
        //! @param [in] def_value The plugin name to set in @a plugin if the plugin is not present.
        //! @param [in] index Index of the plugin to find.
        //!
        void getPlugin(PluginOptions& plugin, PluginType type, const UChar* def_value = u"", size_t index = 0) const;

        //!
        //! Get all plugins of a given type, after command line analysis.
        //! @param [out] plugins Returned name and arguments of the plugins.
        //! @param [in] type Type of plugin to find.
        //!
        void getPlugins(PluginOptionsVector& plugins, PluginType type) const;

    private:
        const size_t _min_inputs;
        const size_t _max_inputs;
        const size_t _min_plugins;
        const size_t _max_plugins;
        const size_t _min_outputs;
        const size_t _max_outputs;
        std::map<PluginType,PluginOptionsVector> _plugins {};

        // Non-virtual version of setSyntax(), can be called in constructor.
        void setDirectSyntax(const UString& syntax);

        // Process --list-plugins.
        void processListPlugins();

        // Search next plugin option.
        size_t nextProcOpt(const UStringVector& args, size_t index, PluginType& type);

        // Load default list of plugins by type.
        void loadDefaultPlugins(PluginType type, const UString& entry);
    };
}
