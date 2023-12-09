//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

namespace ts {
    //!
    //! Representation of a configuration file.
    //! @ingroup app
    //!
    //! Configuration files are based on the old ".INI" syntax on Windows systems.
    //! Lines starting with a '#' are comments and are ignored.
    //! The same entry can be specified several times in the same sections,
    //! all values are kept in a list.
    //!
    class TSDUCKDLL ConfigFile
    {
    public:
        //!
        //! Constructor.
        //! @param [in] filename A file name to read. Don't read a file if empty.
        //! @param [in,out] report Where to report errors.
        //! @param [in] env_disable Optional name of an environment variable. When the corresponding
        //! environment variable is defined to some non-empty value, the file is not loaded.
        //!
        explicit ConfigFile(const fs::path& filename = fs::path(), Report& report = CERR, const UString env_disable = UString());

        //!
        //! Constructor.
        //! @param [in] filename1 A file name to read. Don't read a file if empty.
        //! @param [in] filename2 A file name to read if @a filename1 does not exist. Don't read a file if empty.
        //! @param [in,out] report Where to report errors.
        //! @param [in] env_disable Optional name of an environment variable. When the corresponding
        //! environment variable is defined to some non-empty value, the file is not loaded.
        //!
        ConfigFile(const fs::path& filename1, const fs::path& filename2, Report& report = CERR, const UString env_disable = UString());

        //!
        //! Constructor.
        //! @param [in,out] strm Opened input text stream to read the configuration file.
        //!
        explicit ConfigFile(std::istream& strm);

        //!
        //! System-specific style of default configuration file name.
        //!
        enum FileStyle {
            LOCAL_SYSTEM,   //!< Same as local operating system.
            UNIX_STYLE,     //!< $HOME/.appname
            WINDOWS_STYLE,  //!< appname.ini in same directory as executable
        };

        //!
        //! Get the default configuration file name.
        //! @param [in] style System style for file name.
        //! @param [in] app_name Application name (default: executable base name).
        //! @return The default configuration file name for the application.
        //!
        static fs::path DefaultFileName(FileStyle style = LOCAL_SYSTEM, const UString& app_name = UString());

        //!
        //! Get the latest loaded file.
        //! @return The name of the latest loaded file.
        //!
        fs::path fileName() const { return _filename; }

        //!
        //! Reload the configuration from a file.
        //! @param [in] filename A file name to read.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool load(const fs::path& filename, Report& report = CERR);

        //!
        //! Merge the configuration from a file.
        //! The values which are read from the specified file override previous values.
        //! @param [in] filename A file name to read.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool merge(const fs::path& filename, Report& report = CERR);

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
        bool save(const fs::path& filename = UString(), Report& report = CERR) const;

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
        size_t sectionCount() const { return _sections.size(); }

        //!
        //! Get the names of all sections.
        //! @param [out] names Returned names of all sections.
        //!
        void getSectionNames(UStringVector& names) const;

        //!
        //! Get a reference to a section inside the configuration file instance.
        //! Create the section if it does not exist.
        //! @param [in] name The section name.
        //! @return A reference to the section.
        //!
        ConfigSection& section(const UString& name) { return _sections[name]; }

        //!
        //! Index operator: get a reference to a section inside the configuration file instance.
        //! Create the section if it does not exist.
        //! @param [in] name The section name.
        //! @return A reference to the section.
        //!
        ConfigSection& operator[](const UString& name) { return _sections[name]; }

        //!
        //! Get a reference to a section inside the configuration file instance.
        //! Create the section if it does not exist.
        //! @param [in] name The section name.
        //! @return A read-only reference to the section.
        //!
        const ConfigSection& section(const UString& name) const;

        //!
        //! Index operator: get a reference to a section inside the configuration file instance.
        //! Create the section if it does not exist.
        //! @param [in] name The section name.
        //! @return A read-only reference to the section.
        //!
        const ConfigSection& operator[](const UString& name) const { return section(name); }

        //!
        //! Delete a section.
        //! @param [in] name The section name to delete.
        //!
        void deleteSection(const UString& name) { _sections.erase(name); }

    private:
        // Content of a file.
        // Entries before the first section belong to section named "".
        typedef std::map <UString, ConfigSection> SectionMap;

        // Private members:
        mutable fs::path _filename {};
        SectionMap       _sections {};

        // Used to return a constant reference.
        static const ConfigSection _empty;
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
