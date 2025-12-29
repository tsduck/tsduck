//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Reading macOS XML PList files.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! This class implements a macOS XML PList file.
    //! @ingroup libtscore unix
    //!
    class TSCOREDLL MacPList: public std::map<UString, UString>
    {
    public:
        //!
        //! Explicit reference to the SuperClass
        //!
        using SuperClass = std::map<UString, UString>;

        //!
        //! Constructor from an optional macOS XML PList file.
        //! @param [in] fileName macOS XML PList file name.
        //! @param [in,out] report Where to report errors.
        //!
        explicit MacPList(const UString& fileName = UString(), Report& report = NULLREP);

        //!
        //! Reload from a macOS XML PList file.
        //! @param [in] fileName macOS XML PList file name.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool load(const UString& fileName, Report& report = NULLREP);
    };
}
