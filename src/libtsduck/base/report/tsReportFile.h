//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
    //! @ingroup log
    //!
    //! Reentrancy is supported though the template parameter @a MUTEX.
    //!
    //! @tparam MUTEX A mutex class to synchronize access to the object.
    //!
    template <class MUTEX = ts::null_mutex>
    class ReportFile: public Report
    {
    public:
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
        ReportFile(const UString& file_name, bool append = false, int max_severity = Severity::Info);

        //!
        //! Constructor using an open file stream.
        //!
        //! @param [in,out] stream A reference to an open output file stream to log the messages.
        //! The corresponding stream object must remain valid as long as this object exists.
        //! Typical valid values are std::cout and std::cerr.
        //! @param [in] max_severity Maximum debug level to display. None by default.
        //!
        ReportFile(std::ostream& stream, int max_severity = 0);

        //!
        //! Destructor
        //!
        virtual ~ReportFile() override;

    protected:
        virtual void writeLog(int severity, const UString& message) override;

    private:
        mutable MUTEX _mutex {};       // Synchronization.
        std::string   _file_name {};   // File name in UTF-8.
        std::ofstream _named_file {};  // Explicitly created file.
        std::ostream& _file;           // Reference to actual file stream.
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Constructor using a named file.
template <class MUTEX>
ts::ReportFile<MUTEX>::ReportFile(const UString& file_name, bool append, int max_severity) :
    Report(max_severity),
    _file_name(file_name.toUTF8()),
    _named_file(_file_name.c_str(), append ? (std::ios::out | std::ios::app) : std::ios::out),
    _file(_named_file)
{
    // Process error when creating the file
    if (!_named_file) {
        std::cerr << "Fatal error creating log file " << file_name << std::endl;
    }
}

// Constructor using an open file stream.
template <class MUTEX>
ts::ReportFile<MUTEX>::ReportFile(std::ostream& stream, int max_severity) :
    Report(max_severity),
    _file(stream)
{
}

// Destructor
TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <class MUTEX>
ts::ReportFile<MUTEX>::~ReportFile()
{
    std::lock_guard<MUTEX> lock(_mutex);
    // Close the file if it was explicitly open by constructor
    if (_named_file.is_open()) {
        _named_file.close();
    }
}
TS_POP_WARNING()

// Message processing handler, must be implemented in actual classes.
template <class MUTEX>
void ts::ReportFile<MUTEX>::writeLog(int severity, const UString& message)
{
    std::lock_guard<MUTEX> lock(_mutex);
    _file << Severity::Header(severity) << message << std::endl;
}
