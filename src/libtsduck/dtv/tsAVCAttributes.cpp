//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//
//  Video attributes for Advanced Video Coding (AVC, ISO 14496-10, ITU H.264)
//
//----------------------------------------------------------------------------

#include "tsAVCAttributes.h"
#include "tsAVCSequenceParameterSet.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::AVCAttributes::AVCAttributes() :
    _hsize(0),
    _vsize(0),
    _profile(0),
    _level(0),
    _chroma(0)
{
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::AVCAttributes::toString() const
{
    if (!_is_valid) {
        return UString();
    }

    UString desc;
    desc.format(u"%dx%d, ", {_hsize, _vsize});
    desc += profileName();
    desc += u", level ";
    desc += levelName();
    desc += u", ";
    desc += chromaFormatName();

    return desc;
}


//----------------------------------------------------------------------------
// Get AVC level name.
//----------------------------------------------------------------------------

ts::UString ts::AVCAttributes::levelName() const
{
    return _is_valid ? UString::Format(u"%d.%d", {_level / 10, _level % 10}) : UString();
}


//----------------------------------------------------------------------------
// Provides an AVC access unit. Return true if the AVCAttributes object
// becomes valid or has new values.
//----------------------------------------------------------------------------

bool ts::AVCAttributes::moreBinaryData (const void* data, size_t size)
{
    // Parse AVC access unit. We are interested in "sequence parameter set" only.
    AVCSequenceParameterSet params (data, size);

    if (!params.valid) {
        return false;
    }

    // Compute final values.
    size_t hsize = params.frameWidth();
    size_t vsize = params.frameHeight();
    uint8_t chroma = params.chroma();

    // Check modification
    bool changed = !_is_valid || _hsize != hsize || _vsize != vsize || _chroma != chroma ||
        _profile != int (params.profile_idc) || _level != int (params.level_idc);

    // Commit final values
    if (changed) {
        _hsize = hsize;
        _vsize = vsize;
        _chroma = chroma;
        _profile = int (params.profile_idc);
        _level = int (params.level_idc);
        _is_valid = true;
    }

    return changed;
}
