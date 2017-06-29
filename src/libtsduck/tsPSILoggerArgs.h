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
//!  Command line arguments for the class PSILogger.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTablesDisplayArgs.h"

namespace ts {
    //!
    //! Command line arguments for the class PSILogger.
    //!
    class TSDUCKDLL PSILoggerArgs: public TablesDisplayArgs
    {
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef TablesDisplayArgs SuperClass;

        //!
        //! Constructor.
        //!
        PSILoggerArgs();

        //!
        //! Virtual destructor.
        //!
        ~PSILoggerArgs() {}

        // Public fields, by options.
        bool        all_versions;   //!< Display all versions of PSI tables.
        bool        clear;          //!< Clear stream, do not wait for a CAT.
        bool        cat_only;       //!< Only CAT, ignore other PSI.
        bool        dump;           //!< Dump all sections.
        std::string output;         //!< Destination name file.

        //!
        //! Define command line options in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        virtual void defineOptions(Args& args) const;

        //!
        //! Add help about command line options in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        virtual void addHelp(Args& args) const;

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //!
        virtual void load(Args& args);
    };
}
