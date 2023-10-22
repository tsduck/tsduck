//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAVCAttributes.h"
#include "tsAVCSequenceParameterSet.h"
#include "tsNamesFile.h"


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
// Get AVC names.
//----------------------------------------------------------------------------

ts::UString ts::AVCAttributes::levelName() const
{
    return _is_valid ? UString::Format(u"%d.%d", {_level / 10, _level % 10}) : UString();
}

ts::UString ts::AVCAttributes::profileName() const
{
    return _is_valid ? NameFromDTV(u"avc.profile", _profile) : UString();
}

ts::UString ts::AVCAttributes::chromaFormatName() const
{
    return _is_valid ? NameFromDTV(u"mpeg2.chroma_format", _chroma) : UString();
}


//----------------------------------------------------------------------------
// Provides an AVC access unit. Return true if the AVCAttributes object
// becomes valid or has new values.
//----------------------------------------------------------------------------

bool ts::AVCAttributes::moreBinaryData(const uint8_t* data, size_t size)
{
    // Parse AVC access unit. We are interested in "sequence parameter set" only.
    AVCSequenceParameterSet params(data, size);

    if (!params.valid) {
        return false;
    }

    // Compute final values.
    const size_t hsize = params.frameWidth();
    const size_t vsize = params.frameHeight();
    const uint8_t chroma = params.chroma();

    // Check modification
    bool changed = !_is_valid || _hsize != hsize || _vsize != vsize || _chroma != chroma ||
        _profile != int(params.profile_idc) || _level != int(params.level_idc);

    // Commit final values
    if (changed) {
        _hsize = hsize;
        _vsize = vsize;
        _chroma = chroma;
        _profile = int(params.profile_idc);
        _level = int(params.level_idc);
        _is_valid = true;
    }

    return changed;
}
