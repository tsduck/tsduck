//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Description of one Teletext frame.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Description of one Teletext frame.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TeletextFrame
    {
    public:
        //!
        //! Default constructor.
        TeletextFrame() = default;

        //!
        //! Full constructor.
        //! @param [in] pid PID number.
        //! @param [in] page Teletext page number.
        //! @param [in] frameCount Frame counter in this page, starting at 1.
        //! @param [in] showTimestamp Show frame at this timestamp (in ms from start of stream)
        //! @param [in] hideTimestamp Hide frame at this timestamp (in ms from start of stream)
        //! @param [in] lines Text lines.
        //!
        template <class Rep1, class Period1, class Rep2, class Period2>
        TeletextFrame(PID pid,
                      int page,
                      int frameCount,
                      const cn::duration<Rep1,Period1>& showTimestamp,
                      const cn::duration<Rep2,Period2>& hideTimestamp,
                      const UStringList& lines = UStringList());

        //!
        //! Get the text lines. May contain embedded HTML tags.
        //! @return A constant reference to the text lines.
        //!
        const UStringList& lines() const { return _lines; }

        //!
        //! Add a line of text to the frame.
        //! @param [in] line Text line to add.
        //!
        void addLine(const UString& line) { _lines.push_back(line); }

        //!
        //! Get the PID from which the frame originates.
        //! @return The PID from which the frame originates.
        //!
        PID pid() const { return _pid; }

        //!
        //! Get the Teletext page number.
        //! @return The Teletext page number.
        //!
        int page() const { return _page; }

        //!
        //! Get the frame number in this page, starting at 1.
        //! @return The frame number in this page, starting at 1.
        //!
        int frameCount() const { return _frameCount; }

        //!
        //! Get the "show" timestamp in ms from start of stream.
        //! @return The "show" timestamp in ms from start of stream.
        //!
        cn::milliseconds showTimestamp() const { return _showTimestamp; }

        //!
        //! Get the "hide" timestamp in ms from start of stream.
        //! @return The "hide" timestamp in ms from start of stream.
        //!
        cn::milliseconds hideTimestamp() const { return _hideTimestamp; }

private:
        PID              _pid = PID_NULL;     //!< PID number.
        int              _page = 0;           //!< Teletext page number.
        int              _frameCount = 0;     //!< Frame counter in this page, starting at 1.
        cn::milliseconds _showTimestamp {0};  //!< Show frame at this timestamp (in ms from start of stream)
        cn::milliseconds _hideTimestamp {0};  //!< Hide frame at this timestamp (in ms from start of stream)
        UStringList      _lines {};           //!< Text lines. May contain embedded HTML tags.
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

template <class Rep1, class Period1, class Rep2, class Period2>
ts::TeletextFrame::TeletextFrame(PID pid, int page, int frameCount,
                                 const cn::duration<Rep1,Period1>& showTimestamp,
                                 const cn::duration<Rep2,Period2>& hideTimestamp,
                                 const UStringList& lines) :
    _pid(pid),
    _page(page),
    _frameCount(frameCount),
    _showTimestamp(showTimestamp),
    _hideTimestamp(hideTimestamp),
    _lines(lines)
{
}

#endif // DOXYGEN
