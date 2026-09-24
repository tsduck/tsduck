//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Utilities for standard input, output, and error.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsSysUtils.h"

namespace ts {
    //!
    //! Utilities for standard input, output, and error.
    //! This class has no instance, this is only a set of utilities.
    //! @ingroup libtscore system
    //!
    class TSCOREDLL Stdio
    {
        TS_NOBUILD_NOCOPY(Stdio);
    public:
        //!
        //! Identifiers for standard input streams.
        //!
        enum Id {
            STDIN,   //!< Identify standard input.
            STDOUT,  //!< Identify standard output.
            STDERR,  //!< Identify standard error.
        };

        //!
        //! Get the system handle of a standard stream.
        //! @param [in] id Standard stream to identify.
        //! @return The system handle of a standard stream.
        //!
        static SysHandleType Handle(Id id);

        //!
        //! Get the name of a standard stream.
        //! @param [in] id Standard stream to identify.
        //! @return A constant reference to a string such as "standard input".
        //!
        static const UString& Name(Id id);

        //!
        //! Check if a standard stream is a terminal.
        //! @param [in] id Standard stream to check.
        //! @return True if the standard stream is a terminal.
        //!
        static bool IsTerminal(Id id) { return HandleIsTerminal(Handle(id)); }

        //!
        //! Flush all internal buffers of a standard stream.
        //! @param [in] id Standard stream to flush.
        //!
        static void Flush(Id id);

        //!
        //! Put a standard stream stream in binary mode.
        //! The destructor restores the original mode, binary or text, at the time of the constructor.
        //! @ingroup system
        //!
        //! On UNIX systems, text vs. binary mode not make any difference.
        //!
        //! On Windows systems, in a stream which is not open in binary mode, there is automatic
        //! translation between LF and CR-LF. The standard streams are open in text mode (non-binary).
        //! This class can forces one of them into binary mode.
        //!
        class TSCOREDLL BinaryMode: public ReporterBase
        {
            TS_NOBUILD_NOCOPY(BinaryMode);
        public:
            //!
            //! Constructor: save the original mode but doesn't change it.
            //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
            //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
            //! If @a report is a subclass of ts::Args, terminate the application on error.
            //! @param [in] id Standard stream to manage.
            //!
            BinaryMode(Report* report, Id id);

            //!
            //! Constructor: save the original mode but doesn't change it.
            //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
            //! If the resulting report is a subclass of ts::Args, terminate the application on error.
            //! @param [in] id Standard stream to manage.
            //!
            BinaryMode(ReporterBase* delegate, Id id);

            //!
            //! Constructor: save the original mode and change it.
            //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
            //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
            //! If @a report is a subclass of ts::Args, terminate the application on error.
            //! @param [in] id Standard stream to manage.
            //! @param [in] binary If true, set in binary mode. If false, set in text mode.
            //!
            BinaryMode(Report* report, Id id, bool binary);

            //!
            //! Constructor: save the original mode and change it.
            //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
            //! If the resulting report is a subclass of ts::Args, terminate the application on error.
            //! @param [in] id Standard stream to manage.
            //! @param [in] binary If true, set in binary mode. If false, set in text mode.
            //!
            BinaryMode(ReporterBase* delegate, Id id, bool binary);

            //!
            //! Get the stream id.
            //! @return The stream id.
            //!
            Id id() const { return _id; }

            //!
            //! Change the binary vs. text mode.
            //! @param [in] binary If true, set in binary mode. If false, set in text mode.
            //! @return True on success, false on error.
            //!
            bool setBinaryMode(bool binary);

            //!
            //! Restore the original mode.
            //! This is automatically done in the destructor.
            //! @return True on success, false on error.
            //!
            bool restore();

            //!
            //! Destructor: restore the original mode.
            //!
            virtual ~BinaryMode() override;

        private:
            const Id _id;
#if defined(TS_WINDOWS)
            int _fno = -1;
            int _original = -1;
#endif
            // Perform system-specific initialization.
            void init();
        };

        //!
        //! A class to redirect a standard stream.
        //! @ingroup libtscore system
        //!
        //! The constructor redirects a specific input or output stream from or to a given file.
        //! The destructor automatically restores the previous stream.
        //!
        //! If the file name is empty or "-", no redirection occurs, making this mechanism
        //! quite useful for optional redirection based on command line arguments.
        //!
        class TSCOREDLL Redirector: public ReporterBase
        {
            TS_NOBUILD_NOCOPY(Redirector);
        public:
            //!
            //! Constructor, the redirection is automatically started.
            //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
            //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
            //! If @a report is a subclass of ts::Args, terminate the application on error.
            //! @param [in] id Standard stream to manage.
            //! @param [in] name File name from or to which the standard stream is redirected.
            //! If empty or "-", the standard stream is not redirected.
            //! @param [in] mode Mode to use to open the file, @c std::ios::binary by default.
            //!
            Redirector(Report* report, Id id, const fs::path& name, std::ios::openmode mode = std::ios::binary);

            //!
            //! Constructor, the redirection is automatically started.
            //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
            //! If the resulting report is a subclass of ts::Args, terminate the application on error.
            //! @param [in] id Standard stream to manage.
            //! @param [in] name File name from or to which the standard stream is redirected.
            //! If empty or "-", the standard stream is not redirected.
            //! @param [in] mode Mode to use to open the file, @c std::ios::binary by default.
            //!
            Redirector(ReporterBase* delegate, Id id, const fs::path& name, std::ios::openmode mode = std::ios::binary);

            //!
            //! Get the stream id.
            //! @return The stream id.
            //!
            Id id() const { return _binmode.id(); }

            //!
            //! Destructor, the redirection is terminated and restore the previous stream.
            //!
            virtual ~Redirector() override;

        private:
            BinaryMode      _binmode;
            std::ifstream   _ifile {};
            std::ofstream   _ofile {};
            std::streambuf* _previous = nullptr;

            // Perform system-specific initialization.
            void init(const fs::path& name, std::ios::openmode mode);
        };

    private:
        // Internal check using a system handle.
        static bool HandleIsTerminal(SysHandleType h);
    };
}
