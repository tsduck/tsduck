//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Information about version identification of TSDuck.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"
#include "tsReport.h"
#include "tsThread.h"
#include "tsLibTSCoreVersion.h"

//!
//! TSDuck namespace, containing all TSDuck classes and functions.
//!
namespace ts {
    //!
    //! Information about version identification of TSDuck.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL VersionInfo : private Thread
    {
        TS_NOBUILD_NOCOPY(VersionInfo);
    public:
        //!
        //! Default constructor.
        //! @param [in,out] report Reference to the report object through which new versions will be reported.
        //!
        VersionInfo(Report& report);

        //!
        //! Destructor.
        //!
        virtual ~VersionInfo() override;

        //!
        //! Start a thread which checks the availability of a new TSDuck version.
        //!
        //! If a new version is found, it is reported through the Report object that was specified in the constructor.
        //! This can be done only once, further calls are ignored.
        //! The destructor of this object waits for the completion of the thread.
        //!
        //! If the environment variable TSDUCK_NO_VERSION_CHECK is not empty, do not start the new version check.
        //! To debug new version checking, define the environment variable TS_DEBUG_NEW_VERSION to any non-empty
        //! value; debug and errors messages will be reported through the same Report object.
        //!
        void startNewVersionDetection();

        //!
        //! Types of version formatting, for predefined option -\-version.
        //!
        enum class Format {
            ALL,          //!< Multi-line output with full details.
            SHORT,        //!< Short format X.Y-R.
            LONG,         //!< Full explanatory format.
            INTEGER,      //!< Integer format XXYYRRRRR.
            DATE,         //!< Build date.
        };

        //!
        //! Enumeration description of class Format.
        //! Typically used to implement the -\-version command line option.
        //! @return A constant reference to the enumeration description.
        //!
        static const Names& FormatEnum() { return FormatEnumNames(); }

        //!
        //! Enumeration of supported features.
        //! Typically used to implement the -\-support command line option.
        //! For each name, the value is 1 if the feature is supported and 0 if it is not.
        //! @return A constant reference to the enumeration description.
        //!
        static const Names& SupportEnum() { return SupportEnumNames(); }

        //!
        //! Get the TSDuck formatted version number.
        //! @param [in] format Type of output, short by default.
        //! @param [in] applicationName Name of the application to prepend to the long format.
        //! @return The formatted version string.
        //!
        static UString GetVersion(Format format = Format::SHORT, const UString& applicationName = UString());

        //!
        //! Compare two version strings.
        //! @param [in] v1 First version string.
        //! @param [in] v2 First version string.
        //! @return One of -1, 0, 1 when @a v1 < @a v2, @a v1 == @a v2, @a v1 > @a v2.
        //!
        static int CompareVersions(const UString& v1, const UString& v2);

        //!
        //! A class to register a feature of the application.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSCOREDLL RegisterFeature
        {
            TS_NOBUILD_NOCOPY(RegisterFeature);
        public:
            //!
            //! Describe the level of support for a feature.
            //!
            enum Support {
                ALWAYS,      //!< Feature is always supported, may ask version but no need to ask if supported.
                SUPPORTED,   //!< Optional feature, currently supported.
                UNSUPPORTED  //!< Optional feature, not supported.
            };

            //!
            //! Profile of a function return a version string.
            //!
            using GetVersionFunc = UString (*)();

            //!
            //! Register a feature.
            //! @param [in] option Feature name as used in command line options.
            //! @param [in] name Feature name as used on display.
            //! @param [in] support Level of support.
            //! @param [in] get_version Function returning the version of the feature. Can be null (no identified version).
            //!
            RegisterFeature(const UString& option, const UString& name, Support support, GetVersionFunc get_version);
        };

    private:
        Report& _report;
        Report& _debug;
        bool    _started = false;

        // Inherited from Thread.
        virtual void main() override;

        // Convert a version string into a vector of integers.
        static void VersionToInts(std::vector<int>& ints, const ts::UString& version);

        // Non-constant versions are for internal use only.
        static Names& FormatEnumNames();
        static Names& SupportEnumNames();

        // A map of options with versions.
        using VersionOptionMap = std::map<Names::int_t, std::pair<UString, RegisterFeature::GetVersionFunc>>;
        static VersionOptionMap& VersionOptions();
    };
}

#if defined(DOXYGEN)
//!
//! Macro to disable remote version checking on GitHub.
//!
//! When this macro is defined on the compilation command line, no version
//! check is performed on GitHub. The utility @e tsversion does not call
//! GitHub, does not check, download or upgrade new versions.
//!
//! This macro is typically used by packagers of Linux distros who have the
//! exclusive distribution of software packages. In that case, the packages
//! for TSDuck shall be exclusively upgraded from the distro repositories,
//! not using binaries from GitHub.
//!
#define TS_NO_GITHUB 1
#endif

//!
//! Registration of a feature for which commands may check support level and version.
//! @param option Feature name as used in command line options.
//! @param name Feature name as used on display.
//! @param support Level of support. Use only the enum name, without prefix.
//! @param get_version Function returning the version of the feature. Can be null (no identified version).
//! @hideinitializer
//!
#define TS_REGISTER_FEATURE(option, name, support, get_version) \
    TS_LIBTSCORE_CHECK(); \
    static ts::VersionInfo::RegisterFeature TS_UNIQUE_NAME(_Registrar)((option), (name), ts::VersionInfo::RegisterFeature::support, (get_version))
