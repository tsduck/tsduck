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
//!  Configuration files management
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsConfigSection.h"
#include "tsCerrReport.h"
#include "tsException.h"

namespace ts {
    //!
    //! Configuration files management
    //!
    class TSDUCKDLL ConfigFile
    {
    public:
        //!
        //! Constructor.
        //! @param [in] filename A file name to read. Don't read a file if empty.
        //! @param [in,out] report Where to report errors.
        //!
        explicit ConfigFile(const std::string& filename = "", ReportInterface& report = CERR);

        //!
        //! Constructor.
        //! @param [in,out] strm Opened input text stream to read the configuration file.
        //!
        explicit ConfigFile(std::istream& strm);

        //!
        //! Default configuration file name (executable file name with ".ini" extension)
        //! @return The default configuration file name for the application.
        //!
        static std::string DefaultFileName();

        //!
        //! Reload the configuration from a file.
        //! @param [in] filename A file name to read.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool load(const std::string& filename, ReportInterface& report = CERR);

        //!
        //! Merge the configuration from a file.
        //! The values which are read from the specified file override previous values.
        //! @param [in] filename A file name to read.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool merge(const std::string& filename, ReportInterface& report = CERR);

        //!
        //! Merge the configuration from a file.
        //! The values which are read from the specified file override previous values.
        //! @param [in,out] strm Opened input text stream to read the configuration file.
        //!
        void merge(std::istream& strm);

        //!
        //! Save a configuration file.
        //! @param [in] filename A file name to write. If no file name is specified, use the
        //! file name from constructor or last load() or merge().
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool save(const std::string& filename = "", ReportInterface& report = CERR) const;

        //!
        //! Save a configuration file in a stream
        //! @param [in,out] strm Opened output text stream to write the configuration file.
        //! @return A reference to @a strm.
        //!
        std::ostream& save(std::ostream& strm) const;

        //!
        //! Reset the content of the configuration.
        //!
        void reset();

        //!
        //! Get the number of sections in the file.
        //! @return The number of sections in the file.
        //!
        size_t sectionCount() const;

        //!
        //! Get the names of all sections.
        //! @param [out] names Returned names of all sections.
        //!
        void getSectionNames(StringVector& names) const;

        //!
        //! Get a reference to a section inside the configuration file instance.
        //! Create the section if it does not exist.
        //! @param [in] name The section name.
        //! @return A reference to the section.
        //!
        ConfigSection& section(const std::string& name)
        {
            return _sections[name];
        }

        //!
        //! Index operator: get a reference to a section inside the configuration file instance.
        //! Create the section if it does not exist.
        //! @param [in] name The section name.
        //! @return A reference to the section.
        //!
        ConfigSection& operator[](const std::string& name)
        {
            return _sections[name];
        }

        //!
        //! Get a reference to a section inside the configuration file instance.
        //! Create the section if it does not exist.
        //! @param [in] name The section name.
        //! @return A read-only reference to the section.
        //!
        const ConfigSection& section(const std::string& name) const;

        //!
        //! Index operator: get a reference to a section inside the configuration file instance.
        //! Create the section if it does not exist.
        //! @param [in] name The section name.
        //! @return A read-only reference to the section.
        //!
        const ConfigSection& operator[](const std::string& name) const
        {
            return section(name);
        }

        //!
        //! Delete a section.
        //! @param [in] name The section name to delete.
        //!
        void deleteSection(const std::string& name)
        {
            _sections.erase(name);
        }

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

//!
//! Output operator for the class @link ts::ConfigFile @endlink on standard text streams.
//! @param [in,out] strm An standard stream in output mode.
//! @param [in] config A @link ts::ConfigFile @endlink object.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::ConfigFile& config)
{
    return config.save(strm);
}
