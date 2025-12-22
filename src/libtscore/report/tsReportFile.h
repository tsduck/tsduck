//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A subclass of ts::Report which outputs messages in a text file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsUString.h"

namespace ts {
    //!
    //! A subclass of ts::Report which outputs messages in a text file.
    //! @ingroup libtscore log
    //!
    //! Reentrancy is supported though the template parameter @a SAFETY.
    //!
    //! @tparam SAFETY The required type of thread-safety.
    //!
    template <ThreadSafety SAFETY>
    class ReportFile: public Report
    {
        TS_NOBUILD_NOCOPY(ReportFile);
    public:
        //!
        //! Generic definition of the mutex for this class.
        //!
        using MutexType = typename ThreadSafetyMutex<SAFETY>::type;

        //!
        //! Constructor using a named file.
        //!
        //! @param [in] file_name The name of the file to create. If the file creation fails,
        //! a fatal error is generated with an error message on std::cerr. The file will be
        //! closed when this object is destroyed.
        //! @param [in] append If true, append the messages at the end of the file.
        //! If false (the default), overwrite the file if it already existed.
        //! @param [in] max_severity Maximum debug level to display. None by default.
        //!
        ReportFile(const fs::path& file_name, bool append = false, int max_severity = Severity::Info);

        //!
        //! Constructor using an open file stream.
        //!
        //! @param [in,out] stream A reference to an open output file stream to log the messages.
        //! The corresponding stream object must remain valid as long as this object exists.
        //! Typical valid values are std::cout and std::cerr.
        //! @param [in] max_severity Maximum debug level to display. None by default.
        //!
        ReportFile(std::ostream& stream, int max_severity = 0);

    protected:
        virtual void writeLog(int severity, const UString& message) override;

    private:
        mutable MutexType _mutex {};       // Synchronization.
        fs::path          _file_name {};   // File name in UTF-8.
        std::ofstream     _named_file {};  // Explicitly created file.
        std::ostream&     _file;           // Reference to actual file stream.
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Constructor using a named file.
template <ts::ThreadSafety SAFETY>
ts::ReportFile<SAFETY>::ReportFile(const fs::path& file_name, bool append, int max_severity) :
    Report(max_severity),
    _file_name(file_name),
    _named_file(_file_name, append ? (std::ios::out | std::ios::app) : std::ios::out),
    _file(_named_file)
{
    // Process error when creating the file
    if (!_named_file) {
        std::cerr << "Fatal error creating log file " << file_name << std::endl;
    }
}

// Constructor using an open file stream.
template <ts::ThreadSafety SAFETY>
ts::ReportFile<SAFETY>::ReportFile(std::ostream& stream, int max_severity) :
    Report(max_severity),
    _file(stream)
{
}

// Message processing handler, must be implemented in actual classes.
template <ts::ThreadSafety SAFETY>
void ts::ReportFile<SAFETY>::writeLog(int severity, const UString& message)
{
    std::lock_guard<MutexType> lock(_mutex);
    _file << Severity::AddHeader(severity, message) << std::endl;
}
