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
//!  Options for the class PSILogger.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsMPEG.h"

namespace ts {

    class TSDUCKDLL PSILoggerOptions: public Args
    {
    public:
        // Constructor.
        PSILoggerOptions (const std::string& description = "",
                          const std::string& syntax = "",
                          const std::string& help = "",
                          int flags = 0);

        // Public fields, by options.
        bool        all_versions;   // Display all versions of PSI tables
        bool        clear;          // Clear stream, do not wait for a CAT
        bool        cat_only;       // Only CAT, ignore other PSI
        bool        dump;           // Dump all sections
        std::string output;         // Destination name file

        // Overriden methods.
        void setHelp (const std::string& help);
        virtual bool analyze (int argc, char* argv[]);
        virtual bool analyze (const std::string& app_name, const StringVector& arguments);

        // Get option values (the public fields) after analysis of another
        // ts::Args object defining the same options.
        void getOptions (Args&);

    private:
        // Inaccessible operations
        virtual bool analyze(const char* app_name, const char* arg1, ...);
    };
}
