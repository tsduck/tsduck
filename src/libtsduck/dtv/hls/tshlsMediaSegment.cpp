//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsMediaSegment.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::hls::MediaSegment::MediaSegment() :
    MediaElement(),
    title(),
    duration(0),
    bitrate(0),
    gap(false)
{
}

ts::hls::MediaSegment::~MediaSegment()
{
}
