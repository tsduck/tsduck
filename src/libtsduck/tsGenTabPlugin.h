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
//!
//!  @file
//!  Definition of the API of a tsgentab plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsAbstractTable.h"
#include "tsReportInterface.h"

namespace ts {
    //!
    //! Abstract base class of all tsgentab plugins.
    //!
    //! GenTabPlugin is a subclass of Args; each constructor is expected to define
    //! the syntax, help and option definitions for the command line.
    //!
    class TSDUCKDLL GenTabPlugin: public Args
    {
    public:
        //!
        //! Constructor.
        //!
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] help A multi-line string describing the usage of options and parameters.
        //!
        GenTabPlugin(const std::string& description = "",
                     const std::string& syntax = "",
                     const std::string& help = "") :
            Args(description, syntax, help)
        {
        }

        //!
        //! Virtual destructor
        //!
        virtual ~GenTabPlugin() {}

        //!
        //! The main application invokes generate() to generate the table.
        //! Must be implemented by subclasses.
        //! @param [out] table Safe pointer to the generated table.
        //!
        virtual void generate(AbstractTablePtr& table) = 0;

    private:
        GenTabPlugin(const GenTabPlugin&) = delete;
        GenTabPlugin& operator=(const GenTabPlugin&) = delete;
    };

    //!
    //! Tsgentab plugin interface profile.
    //!
    //! All shared libraries providing a tsgentab plugin shall export
    //! a global function named @c tsgentabNewPlugin with the following profile.
    //!
    //! @return A new allocated object implementing ts::GenTabPlugin.
    //!
    typedef GenTabPlugin* (*NewGenTabPluginProfile)();
}

//!
//! Export tsgentab plugin interface out of the shared library.
//! This macro declares the plugin allocation routine.
//! Shall be used by shared libraries which provide a tsgentab plugin.
//! @param type Name of a subclass of ts::GenTabPlugin implementing the plugin.
//! @hideinitializer
//!
#define TSGENTAB_DECLARE_PLUGIN(type)         \
    extern "C" {                              \
        /** @cond nodoxygen */                \
        TS_DLL_EXPORT                         \
        ts::GenTabPlugin* tsgentabNewPlugin() \
        {                                     \
            return new type();                \
        }                                     \
        /** @endcond */                       \
    }
