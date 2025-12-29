//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
        str.format(u", %dx%d", width, height);
    }
    if (bandwidth > 0) {
        str.format(u", %'d b/s", bandwidth);
    }
    else if (average_bandwidth > 0) {
        str.format(u", %'d b/s", average_bandwidth);
    }
    if (frame_rate % 1000 != 0) {
        str.format(u", @%d.%03d fps", frame_rate / 1000, frame_rate % 1000);
    }
    else if (frame_rate > 0) {
        str.format(u", @%d fps", frame_rate / 1000);
    }

    return str;
}
