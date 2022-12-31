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
    public:
        //!
        //! Virtual destructor.
        //!
        virtual ~CommandLineHandler();
    };

    //!
    //! Profile of a CommandLineHandler method which is invoked by a CommandLine instance for one command.
    //! @param [in] command Name of the command, unabbreviated, as defined in CommandLine::command().
    //! @param [in,out] args Command line arguments.
    //! @return Status of the execution of the command.
    //!
    typedef CommandStatus (CommandLineHandler::*CommandLineMethod)(const UString& command, Args& args);
}
