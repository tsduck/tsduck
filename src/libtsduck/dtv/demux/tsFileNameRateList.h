//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Specialized list of file names and an associated repetition rates.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsFileNameRate.h"

namespace ts {
    //!
    //! Specialized list of file names and an associated repetition rates.
    //! This is typically used to handle section files to inject into transport streams.
    //! @ingroup cmd
    //!
    class TSDUCKDLL FileNameRateList : public std::list<FileNameRate>
    {
    public:
        //!
        //! Reference to the superclass.
        //!
        typedef std::list<FileNameRate> SuperClass;

        //!
        //! Default constructor.
        //!
        FileNameRateList() = default;

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        FileNameRateList(const FileNameRateList& other) : SuperClass(other) {}

        //!
        //! Decode a list of parameters containing a list of file names with
        //! optional repetition rates in milliseconds.
        //!
        //! @param [in,out] args Instance of ts::Args containing the command line parameters.
        //! @param [in] option_name The long name of an option. All values of this option
        //! are fetched. Each value must be a string "name[=value]" where @e value is an
        //! optional repetition rate in milliseconds.
        //! @param [in] default_rate Default repetition rate for files without repetition rate.
        //! @return True on success. On error, set error state in @a args and return false.
        //!
        bool getArgs(Args& args, const UChar* option_name = nullptr, MilliSecond default_rate = 0);

        //!
        //! Scan the files for update.
        //! Update the modification dates of the files.
        //! @param [in] retry Number of allowed retry in case of error when using the file.
        //! @param [in] report Where to report a verbose message when a file changed.
        //! @return Number of files which changed.
        //!
        size_t scanFiles(size_t retry = 1, Report& report = NULLREP);
    };
}
