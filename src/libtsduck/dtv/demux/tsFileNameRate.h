//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Decode file names / repetition rates command line arguments
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTime.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Representation of a file name and an associated repetition rate.
    //! This is typically used to handle section files to inject into transport streams.
    //! @ingroup cmd
    //!
    class TSDUCKDLL FileNameRate
    {
    public:
        UString     file_name {};        //!< File name.
        UString     display_name {};     //!< File name in display form.
        bool        inline_xml = false;  //!< File name contains inline XML text (not a real file name).
        Time        file_date {};        //!< Last modification date of file.
        MilliSecond repetition = 0;      //!< Repetition rate in milliseconds.
        size_t      retry_count {1};     //!< Number of allowed retry in case of error when using the file.

        //!
        //! Default constructor.
        //! @param [in] name File name.
        //! @param [in] rep  Repetition rate in milliseconds.
        //!
        explicit FileNameRate(const UString& name = UString(), MilliSecond rep = 0);

        //!
        //! Comparison operator.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is equal to @a other.
        //!
        bool operator==(const FileNameRate& other) const;
        TS_UNEQUAL_OPERATOR(FileNameRate)

        //!
        //! Comparison "less than" operator.
        //! It does not really makes sense. Only defined to allow usage in containers.
        //! @param [in] other Other instance to compare.
        //! @return True if this instance is less than to @a other.
        //!
        bool operator<(const FileNameRate& other) const;

        //!
        //! Scan the file for update.
        //! Update the modification date of the file in @a file_date.
        //! @param [in] retry Number of allowed retry in case of error when using the file.
        //! @param [in] report Where to report a verbose message when a file changed.
        //! @return True if the file has changed or is scanned for the first time
        //! or has been deleted.
        //!
        bool scanFile(size_t retry = 1, Report& report = NULLREP);
    };
}
