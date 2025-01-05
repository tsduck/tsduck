//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsMediaElement.h"


//----------------------------------------------------------------------------
// Implementation of StringifyInterface
//----------------------------------------------------------------------------

ts::UString ts::hls::MediaElement::toString() const
{
    return relative_uri.empty() ? u"unknown URI" : relative_uri;
}


//----------------------------------------------------------------------------
// Get the URL string to use.
//----------------------------------------------------------------------------

ts::UString ts::hls::MediaElement::urlString() const
{
    return url.isValid() ? url.toString() : file_path;
}
