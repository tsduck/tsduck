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
//!
//!  @file
//!  Definition of TSP control commands syntax.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsCerrReport.h"

namespace ts {
    //!
    //! Definition of TSP control commands syntax.
    //! These commands are used with the @a tspcontrol utility to
    //! inspect or modify a running @a tsp command.
    //! @ingroup plugin
    //!
    class TSDUCKDLL TSPControlCommand
    {
        TS_NOCOPY(TSPControlCommand);
    public:
        //!
        //! Constructor.
        //!
        TSPControlCommand();

        //!
        //! Definition of TSP control command.
        //!
        enum ControlCommand {
            CMD_NONE,     //!< No command specified, do nothing.
            CMD_EXIT,     //!< Exit tsp.
            CMD_SETLOG,   //!< Change log level.
            CMD_LIST,     //!< List all plugins.
            CMD_SUSPEND,  //!< Suspend a plugin.
            CMD_RESUME,   //!< Resume a suspended plugin.
            CMD_RESTART,  //!< Restart a plugin with different parameters.
        };

        //!
        //! Enumeration description of ControlCommand.
        //!
        static const Enumeration ControlCommandEnum;

        //!
        //! Analyze a command line.
        //! @param [in] line Command line to analyze.
        //! @param [out] cmd Returned command line index.
        //! @param [out] args Return pointer to analyzed args.
        //! Points to an Args object inside this instance of ControlCommandLine.
        //! @param [in,out] report Where to report errors.
        //! @return True for a valid command, false on invalid command.
        //!
        bool analyze(const UString& line, ControlCommand& cmd, const Args*& args, Report& report);

        //!
        //! Get a formatted help text for all commands.
        //! @param [in] format Requested format of the help text.
        //! @param [in] line_width Maximum width of text lines.
        //! @return The formatted help text.
        //!
        UString getAllHelpText(Args::HelpFormat format, size_t line_width = 79) const;

    private:
        std::map<ControlCommand,Args> _commands;

        // Add a new command.
        Args* newCommand(ControlCommand cmd, const UString& description, const UString& syntax, int flags = 0);
    };
}
