//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsMediaPlayList.h"


//----------------------------------------------------------------------------
// Implementation of StringifyInterface
//----------------------------------------------------------------------------

ts::UString ts::hls::MediaPlayList::toString() const
{
    UString str(MediaElement::toString());

    if (width > 0 || height > 0) {
        str.format(u", %dx%d", {width, height});
    }
    if (bandwidth > 0) {
        str.format(u", %'d b/s", {bandwidth});
    }
    else if (averageBandwidth > 0) {
        str.format(u", %'d b/s", {averageBandwidth});
    }
    if (frameRate % 1000 != 0) {
        str.format(u", @%d.%03d fps", {frameRate / 1000, frameRate % 1000});
    }
    else if (frameRate > 0) {
        str.format(u", @%d fps", {frameRate / 1000});
    }

    return str;
}
