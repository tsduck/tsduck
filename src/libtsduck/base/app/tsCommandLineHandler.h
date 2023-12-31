//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  An interface which handles a command from a CommandLine instance.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class Args;
    class UString;

    //!
    //! Status of a command which is executed by a CommandLine object.
    //! @ingroup cmd
    //!
    enum class CommandStatus {
        SUCCESS,  //!< Command successful.
        EXIT,     //!< Exit command interpreter with success status.
        ERROR,    //!< Command terminated on error.
        FATAL     //!< Fatal error, exit command interpreter with error status.
    };

    //!
    //! An interface which handles a command from a CommandLine instance.
    //! @ingroup cmd
    //!
    class TSDUCKDLL CommandLineHandler
    {
        TS_INTERFACE(CommandLineHandler);
    };

    //!
    //! Profile of a CommandLineHandler method which is invoked by a CommandLine instance for one command.
    //! @param [in] command Name of the command, unabbreviated, as defined in CommandLine::command().
    //! @param [in,out] args Command line arguments.
    //! @return Status of the execution of the command.
    //!
    using CommandLineMethod = CommandStatus (CommandLineHandler::*)(const UString& command, Args& args);
}
