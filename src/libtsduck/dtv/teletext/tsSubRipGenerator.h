//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generate subtitles in SubRip format (aka SRT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Generate subtitles in SubRip format (aka SRT).
    //! @ingroup mpeg
    //!
    class TSDUCKDLL SubRipGenerator
    {
        TS_NOCOPY(SubRipGenerator);
    public:
        //!
        //! Constructor.
        //! @param [in] fileName Optional output file name. If non-empty, the file is created and open.
        //! Use isOpen() to check if the file was correctly created.
        //! @param [in,out] report Where to report errors.
        //!
        explicit SubRipGenerator(const fs::path& fileName = UString(), Report& report = NULLREP);

        //!
        //! Constructor.
        //! @param [in] stream Output text stream. The generator is considered "open" if @a stream is not zero.
        //!
        explicit SubRipGenerator(std::ostream* stream);

        //!
        //! Destructor.
        //!
        ~SubRipGenerator();

        //!
        //! Open or re-open the generator to a new file.
        //! The previous file is closed.
        //! @param [in] fileName Output file name.
        //! @param [in,out] report Where to report errors.
        //! @return True if the file was correctly created and open, false otherwise.
        //!
        bool open(const fs::path& fileName, Report& report = NULLREP);

        //!
        //! Open or re-open the generator to a new text stream.
        //! The previous file is closed.
        //! @param [in] stream Output text stream.
        //! @return True if @a stream is not zero, false otherwise.
        //!
        bool setStream(std::ostream* stream);

        //!
        //! Close the generator.
        //!
        void close();

        //!
        //! Check if the generator is open and ready to output frames.
        //! @return True if the generator is open and ready.
        //!
        bool isOpen() { return _stream != nullptr; }

        //!
        //! Add a multi-lines subtitle frame.
        //! @param [in] showTimestamp Show frame at this timestamp (in ms from start of stream)
        //! @param [in] hideTimestamp Hide frame at this timestamp (in ms from start of stream)
        //! @param [in] lines Text lines.
        //!
        void addFrame(MilliSecond showTimestamp, MilliSecond hideTimestamp, const UStringList& lines);

        //!
        //! Add a one-line subtitle frame.
        //! @param [in] showTimestamp Show frame at this timestamp (in ms from start of stream)
        //! @param [in] hideTimestamp Hide frame at this timestamp (in ms from start of stream)
        //! @param [in] line Text line.
        //!
        void addFrame(MilliSecond showTimestamp, MilliSecond hideTimestamp, const UString& line);

        //!
        //! Get the number of generated frames fo far.
        //! @return The number of generated frames fo far.
        //!
        int frameCount() const { return _frameCount; }

        //!
        //! Format a timestamp as SRT time.
        //! @param [in] timestamp Timestamp in milliseconds.
        //! @return SRT formatted time.
        //!
        static UString FormatTime(MilliSecond timestamp);

        //!
        //! Format a duration as SRT header.
        //! @param [in] showTimestamp Show frame at this timestamp (in ms from start of stream)
        //! @param [in] hideTimestamp Hide frame at this timestamp (in ms from start of stream)
        //! @return SRT formatted duration.
        //!
        static UString FormatDuration(MilliSecond showTimestamp, MilliSecond hideTimestamp);

    private:
        std::ofstream _outputStream {};   // Text stream for output file.
        std::ostream* _stream = nullptr;  // Output text stream.
        int           _frameCount = 0;    // Number of output frames.
    };
}
