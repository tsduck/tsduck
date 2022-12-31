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
        //! Constructor.
        //! @param [in] pid PID number.
        //! @param [in] page Teletext page number.
        //! @param [in] frameCount Frame counter in this page, starting at 1.
        //! @param [in] showTimestamp Show frame at this timestamp (in ms from start of stream)
        //! @param [in] hideTimestamp Hide frame at this timestamp (in ms from start of stream)
        //! @param [in] lines Text lines.
        //!
        TeletextFrame(PID                pid           = 0,
                      int                page          = 0,
                      int                frameCount    = 0,
                      MilliSecond        showTimestamp = 0,
                      MilliSecond        hideTimestamp = 0,
                      const UStringList& lines         = UStringList());

        //!
        //! Get the text lines. May contain embedded HTML tags.
        //! @return The text lines.
        //!
        UStringList lines() const
        {
            return _lines;
        }

        //!
        //! Add a line of text to the frame.
        //! @param [in] line Text line to add.
        //!
        void addLine(const UString& line)
        {
            _lines.push_back(line);
        }

        //!
        //! Get the PID from which the frame originates.
        //! @return The PID from which the frame originates.
        //!
        PID pid() const
        {
            return _pid;
        }

        //!
        //! Get the Teletext page number.
        //! @return The Teletext page number.
        //!
        int page() const
        {
            return _page;
        }

        //!
        //! Get the frame number in this page, starting at 1.
        //! @return The frame number in this page, starting at 1.
        //!
        int frameCount() const
        {
            return _frameCount;
        }

        //!
        //! Get the "show" timestamp in ms from start of stream.
        //! @return The "show" timestamp in ms from start of stream.
        //!
        MilliSecond showTimestamp() const
        {
            return _showTimestamp;
        }

        //!
        //! Get the "hide" timestamp in ms from start of stream.
        //! @return The "hide" timestamp in ms from start of stream.
        //!
        MilliSecond hideTimestamp() const
        {
            return _hideTimestamp;
        }

private:
        PID         _pid;            //!< PID number.
        int         _page;           //!< Teletext page number.
        int         _frameCount;     //!< Frame counter in this page, starting at 1.
        MilliSecond _showTimestamp;  //!< Show frame at this timestamp (in ms from start of stream)
        MilliSecond _hideTimestamp;  //!< Hide frame at this timestamp (in ms from start of stream)
        UStringList _lines;          //!< Text lines. May contain embedded HTML tags.
    };
}
