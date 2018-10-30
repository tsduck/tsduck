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
    //! The command line is analyzed, keeping command-specific options and
    //! plugin descriptions apart.
    //! @ingroup plugin
    //!
    class TSDUCKDLL ArgsWithPlugins: public Args
    {
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
        virtual bool analyze(int argc, char* argv[], bool processRedirections = true) override;
        virtual bool analyze(const UString& app_name, const UStringVector& arguments, bool processRedirections = true) override;

        // Option values
        PluginOptionsVector inputs;    //!< Ordered list of input plugins.
        PluginOptionsVector plugins;   //!< Ordered list of packet processor plugins.
        PluginOptionsVector outputs;   //!< Ordered list of output plugins.

    private:
        const size_t _min_inputs;
        const size_t _max_inputs;
        const size_t _min_plugins;
        const size_t _max_plugins;
        const size_t _min_outputs;
        const size_t _max_outputs;

        // Search next plugin option.
        size_t nextProcOpt(const UStringVector& args, size_t index, PluginType& type, PluginOptionsVector*& opts);

        // Load default list of plugins by type.
        void loadDefaultPlugins(PluginType type, const UString& entry, PluginOptionsVector& options);

        // Inaccessible operations.
        ArgsWithPlugins(const ArgsWithPlugins&) = delete;
        ArgsWithPlugins& operator=(const ArgsWithPlugins&) = delete;
    };
}
