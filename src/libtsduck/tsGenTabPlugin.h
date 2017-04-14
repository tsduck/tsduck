//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Definition of the API of a tsgentab plugin.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsAbstractTable.h"
#include "tsReportInterface.h"

namespace ts {

    // Abstract base class of all tsgentab plugins.
    // Plugin is a subclass of Args; each constructor is expected to define
    // the syntax, help and option definitions for the command line.

    class TSDUCKDLL GenTabPlugin: public Args
    {
    public:

        // Constructor
        GenTabPlugin (const std::string& description_ = "",
                      const std::string& syntax_ = "",
                      const std::string& help_ = "") :
            Args (description_, syntax_, help_) {}

        // Virtual destructor
        virtual ~GenTabPlugin () {}

        // The main application invokes generate() to generate the table.
        virtual void generate (AbstractTablePtr&) = 0;

    private:
        GenTabPlugin(const GenTabPlugin&) = delete;
        GenTabPlugin& operator=(const GenTabPlugin&) = delete;
    };

    // All shared libraries providing a tsgentab plugin shall export
    // a global function named "tsgentabNewPlugin" with the following profile.
    // When invoked, it shall allocate a new object implementing
    // ts::GenTabPlugin.

    typedef GenTabPlugin* (*NewGenTabPluginProfile)();
}


//----------------------------------------------------------------------------
//  Helper macros for shared libraries
//----------------------------------------------------------------------------

// The following macros declare the plugin allocation routines.
// Shall be used by shared libraries which provide the plugin.

#define TSGENTAB_DECLARE_PLUGIN(type)             \
    extern "C" {                                  \
        TS_DLL_EXPORT                           \
        ts::GenTabPlugin* tsgentabNewPlugin ()  \
        {                                         \
            return new type ();                   \
        }                                         \
    }
