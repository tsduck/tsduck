//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTeletextFrame.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TeletextFrame::TeletextFrame(PID pid,
                                 int page,
                                 int frameCount,
                                 MilliSecond showTimestamp,
                                 MilliSecond hideTimestamp,
                                 const UStringList& lines) :
    _pid(pid),
    _page(page),
    _frameCount(frameCount),
    _showTimestamp(showTimestamp),
    _hideTimestamp(hideTimestamp),
    _lines(lines)
{
}
