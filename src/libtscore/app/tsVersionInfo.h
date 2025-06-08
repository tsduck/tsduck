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
            ALL     = -1,  //!< Multi-line output with full details.
            SHORT   = -2,  //!< Short format X.Y-R.
            LONG    = -3,  //!< Full explanatory format.
            INTEGER = -4,  //!< Integer format XXYYRRRRR.
            DATE    = -5,  //!< Build date.
        };

        //!
        //! Enumeration description of class Format.
        //! Typically used to implement the -\-version command line option.
        //! @return A constant reference to the enumeration description.
        //!
        static const Names& FormatEnum();

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

    private:
        Report& _report;
        Report& _debug;
        bool    _started = false;

        // Inherited from Thread.
        virtual void main() override;

        // Convert a version string into a vector of integers.
        static void VersionToInts(std::vector<int>& ints, const ts::UString& version);
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
