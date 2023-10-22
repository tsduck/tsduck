//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCAttributes.h"
#include "tsHEVCSequenceParameterSet.h"
#include "tsNamesFile.h"


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
