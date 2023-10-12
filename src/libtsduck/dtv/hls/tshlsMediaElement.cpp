//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsMediaElement.h"


//----------------------------------------------------------------------------
// Implementation of StringifyInterface
//----------------------------------------------------------------------------

ts::UString ts::hls::MediaElement::toString() const
{
    return relativeURI.empty() ? u"unknown URI" : relativeURI;
}


//----------------------------------------------------------------------------
// Get the URL string to use.
//----------------------------------------------------------------------------

ts::UString ts::hls::MediaElement::urlString() const
{
    return url.isValid() ? url.toString() : filePath;
}
