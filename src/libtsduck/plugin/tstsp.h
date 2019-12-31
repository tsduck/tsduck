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
//!  Common definitions for the tsp tool.
//!  @ingroup plugin
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"

namespace ts {
    //!
    //! Namespace for TSP classes.
    //!
    namespace tsp {
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
        //! Enumeration description of ts::tsp::ControlCommand.
        //!
        TSDUCKDLL extern const Enumeration ControlCommandEnum;
    }
}
