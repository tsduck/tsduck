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
//  Configuration files management
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsConfigSection.h"
#include "tsCerrReport.h"
#include "tsException.h"

namespace ts {

    class TSDUCKDLL ConfigFile
    {
    public:
        // Constructor & destructor
        explicit ConfigFile(const std::string& filename = "", ReportInterface& report = CERR);
        explicit ConfigFile(std::istream& strm);

        // Default configuration file name (executable file name with ".ini" extension)
        static std::string DefaultFileName();

        // Reload configuration from a file.
        // Return true on success, false on error.
        bool load(const std::string& filename, ReportInterface& report = CERR);

        // Merge configuration from a file.
        // Values from the specified file override previous values.
        // Return true on success, false on error.
        bool merge(const std::string& filename, ReportInterface& report = CERR);
        void merge(std::istream& strm);

        // Save a configuration file.
        // If no file name is specified, use name from constructor or last load() or merge().
        // Return true on success, false on error.
        bool save(const std::string& filename = "", ReportInterface& report = CERR) const;

        // Save a configuration file in a stream
        std::ostream& save(std::ostream& strm) const;

        // Reset content of the configuration
        void reset();

        // Get the number of sections in the file.
        size_t sectionCount() const;

        // Get the names of all sections
        void getSectionNames(StringVector& names) const;

        // Get a reference to a section.
        // Create it if does not exist.
        ConfigSection& section(const std::string& name) {return _sections[name];}
        ConfigSection& operator[](const std::string& name) {return _sections[name];}

        // Get a reference to a section.
        // Return a reference to an empty section if does not exist.
        const ConfigSection& section(const std::string& name) const;
        const ConfigSection& operator[](const std::string& name) const {return section(name);}

        // Delete a section
        void deleteSection(const std::string& name) {_sections.erase(name);}

    private:
        // Content of a file.
        // Entries before the first section belong to section named "".
        typedef std::map <std::string, ConfigSection> SectionMap;

        // Private members:
        mutable std::string _filename;
        SectionMap          _sections;
        const ConfigSection _empty;
    };
}

// Output operator
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::ConfigFile& config)
{
    return config.save(strm);
}
