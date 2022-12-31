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

#include "tsHEVCAttributes.h"
#include "tsHEVCSequenceParameterSet.h"
#include "tsNamesFile.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::HEVCAttributes::HEVCAttributes() :
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

ts::UString ts::HEVCAttributes::toString() const
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
// Get HEVC names.
//----------------------------------------------------------------------------

ts::UString ts::HEVCAttributes::levelName() const
{
    return _is_valid ? UString::Format(u"%d.%d", {_level / 30, (_level / 3) % 10}) : UString();
}

ts::UString ts::HEVCAttributes::profileName() const
{
    return _is_valid? NameFromDTV(u"hevc.profile", _profile) : UString();
}

ts::UString ts::HEVCAttributes::chromaFormatName() const
{
    return _is_valid ? NameFromDTV(u"mpeg2.chroma_format", _chroma) : UString();
}


//----------------------------------------------------------------------------
// Provides an HEVC access unit. Return true if the HEVCAttributes object
// becomes valid or has new values.
//----------------------------------------------------------------------------

bool ts::HEVCAttributes::moreBinaryData(const uint8_t* data, size_t size)
{
    // Parse HEVC access unit. We are interested in "sequence parameter set" only.
    HEVCSequenceParameterSet params(data, size);

    if (!params.valid) {
        return false;
    }

    // Compute final values.
    const size_t hsize = params.frameWidth();
    const size_t vsize = params.frameHeight();
    const int profile = int(params.profile_tier_level.profile());
    const uint8_t chroma = params.chroma();

    // Check modification
    const bool changed = !_is_valid || _hsize != hsize || _vsize != vsize || _chroma != chroma || _profile != profile ||
        _level != int(params.profile_tier_level.general_level_idc);

    // Commit final values
    if (changed) {
        _hsize = hsize;
        _vsize = vsize;
        _chroma = chroma;
        _profile = profile;
        _level = int(params.profile_tier_level.general_level_idc);
        _is_valid = true;
    }

    return changed;
}
