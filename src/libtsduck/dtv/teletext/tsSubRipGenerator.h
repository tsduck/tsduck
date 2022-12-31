//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
        explicit SubRipGenerator(const UString& fileName = UString(), Report& report = NULLREP);

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
        bool open(const UString& fileName, Report& report = NULLREP);

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
        std::ofstream _outputStream;  //!< Text stream for output file.
        std::ostream* _stream;        //!< Output text stream.
        int           _frameCount;    //!< Number of output frames.
    };
}
