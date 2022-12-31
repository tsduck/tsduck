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
//!
//!  @file
//!  Video attributes for HEVC / H.265.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAudioVideoAttributes.h"

namespace ts {
    //!
    //! Video attributes for HEVC / H.265.
    //! @ingroup mpeg
    //!
    //! An HEVCAttributes object is built by transmitting HEVC access units
    //! (aka "NALunits"). The state of the object may change after adding a
    //! "sequence parameter set" NALunit. Initially, an HEVCAttributes object
    //! is invalid.
    //!
    class TSDUCKDLL HEVCAttributes: public AbstractAudioVideoAttributes
    {
    public:
        //!
        //! Default constructor.
        //!
        HEVCAttributes();

        // Implementation of abstract methods.
        // The "binary data" is an HEVC access unit.
        virtual bool moreBinaryData(const uint8_t*, size_t) override;
        virtual UString toString() const override;

        //!
        //! Get video horizontal size in pixels.
        //! @return Video horizontal size in pixels.
        //!
        size_t horizontalSize() const {return _is_valid ? _hsize : 0;}

        //!
        //! Get video vertical size in pixels.
        //! @return Video vertical size in pixels.
        //!
        size_t verticalSize() const {return _is_valid ? _vsize : 0;}

        //!
        //! Get HEVC profile.
        //! @return HEVC profile, 0 if unknown.
        //!
        int profile() const {return _is_valid ? _profile : 0;}

        //!
        //! Get HEVC profile name.
        //! @return HEVC profile as a string.
        //!
        UString profileName() const;

        //!
        //! Get HEVC level.
        //! @return HEVC level, 0 if unknown.
        //!
        int level() const {return _is_valid ? _level : 0;}

        //!
        //! Get HEVC level name.
        //! @return HEVC level as a string.
        //!
        UString levelName() const;

        //!
        //! Get chroma format.
        //! @return Chroma format, code values are CHROMA_* from tsMPEG.h, 0 if unknown.
        //!
        uint8_t chromaFormat() const {return _is_valid ? _chroma : 0;}

        //!
        //! Get chroma format name.
        //! @return Chroma format as a string.
        //!
        UString chromaFormatName() const;

    private:
        size_t  _hsize;    // Horizontal size in pixel
        size_t  _vsize;    // Vertical size in pixel
        int     _profile;  // HEVC profile
        int     _level;    // HEVC level
        uint8_t _chroma;   // Chroma format code (CHROMA_* from tsMPEG.h)
    };
}
