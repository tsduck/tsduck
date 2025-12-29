//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
    //! @ingroup libtsduck mpeg
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
        //! @param [in] showTimestamp Show frame at this timestamp (from start of stream)
        //! @param [in] hideTimestamp Hide frame at this timestamp (from start of stream)
        //! @param [in] lines Text lines.
        //!
        template <class Rep1, class Period1, class Rep2, class Period2>
        void addFrame(const cn::duration<Rep1,Period1>& showTimestamp, const cn::duration<Rep2,Period2>& hideTimestamp, const UStringList& lines);

        //!
        //! Add a one-line subtitle frame.
        //! @param [in] showTimestamp Show frame at this timestamp (from start of stream)
        //! @param [in] hideTimestamp Hide frame at this timestamp (from start of stream)
        //! @param [in] line Text line.
        //!
        template <class Rep1, class Period1, class Rep2, class Period2>
        void addFrame(const cn::duration<Rep1,Period1>& showTimestamp, const cn::duration<Rep2,Period2>& hideTimestamp, const UString& line)
        {
            addFrame(showTimestamp, hideTimestamp, UStringList {line});
        }

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
        template <class Rep, class Period>
        static UString FormatTime(const cn::duration<Rep,Period>& timestamp);

        //!
        //! Format a duration as SRT header.
        //! @param [in] showTimestamp Show frame at this timestamp (in ms from start of stream)
        //! @param [in] hideTimestamp Hide frame at this timestamp (in ms from start of stream)
        //! @return SRT formatted duration.
        //!
        template <class Rep1, class Period1, class Rep2, class Period2>
        static UString FormatDuration(const cn::duration<Rep1,Period1>& showTimestamp, const cn::duration<Rep2,Period2>& hideTimestamp)
        {
            return FormatTime(showTimestamp) + u" --> " + FormatTime(hideTimestamp);
        }

    private:
        std::ofstream _outputStream {};   // Text stream for output file.
        std::ostream* _stream = nullptr;  // Output text stream.
        int           _frameCount = 0;    // Number of output frames.
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Format a duration as SRT header.
template <class Rep, class Period>
ts::UString ts::SubRipGenerator::FormatTime(const cn::duration<Rep,Period>& timestamp)
{
    // Time stamp is in milliseconds.
    const cn::milliseconds::rep ms = cn::duration_cast<cn::milliseconds>(timestamp).count();
    const int h = int(ms / 3600000);
    const int m = int(ms / 60000 - 60 * h);
    const int s = int(ms / 1000 - 3600 * h - 60 * m);
    const int u = int(ms - 3600000 * h - 60000 * m - 1000 * s);
    return UString::Format(u"%02d:%02d:%02d,%03d", h, m, s, u);
}

// Add a multi-lines subtitle frame.
template <class Rep1, class Period1, class Rep2, class Period2>
void ts::SubRipGenerator::addFrame(const cn::duration<Rep1,Period1>& showTimestamp, const cn::duration<Rep2,Period2>& hideTimestamp, const UStringList& lines)
{
    // Empty lines are illegal in SRT. Make sure we have at least one non-empty line.
    bool notEmpty = false;
    for (const auto& it : lines) {
        if (!it.empty()) {
            notEmpty = true;
            break;
        }
    }

    // Generate the frame only when it is possible to do so.
    if (notEmpty && _stream != nullptr) {
        // First line: Frame count, starting at 1.
        // Second line: Start and end timestamps.
        *_stream << ++_frameCount << std::endl
                 << FormatDuration(showTimestamp, hideTimestamp) << std::endl;

        // Subsequent lines: Subtitle text.
        for (const auto& it : lines) {
            // Empty lines are illegal in SRT, skip them.
            if (!it.empty()) {
                *_stream << it << std::endl;
            }
        }
        // Trailing empty line to mark the end of frame.
        *_stream << std::endl;
    }
}

#endif // DOXYGEN
