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

#include "tshlsMediaPlayList.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::hls::MediaPlayList::MediaPlayList() :
    MediaElement(),
    bandwidth(0),
    averageBandwidth(0),
    width(0),
    height(0),
    frameRate(0),
    codecs(),
    hdcp(),
    videoRange(),
    video(),
    audio(),
    subtitles(),
    closedCaptions()
{
}


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
