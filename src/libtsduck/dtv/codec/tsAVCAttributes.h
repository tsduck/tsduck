//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Video attributes for Advanced Video Coding.
//!  AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAudioVideoAttributes.h"

namespace ts {
    //!
    //! Video attributes for Advanced Video Coding.
    //! @ingroup mpeg
    //!
    //! AVC is Advanced Video Coding, ISO 14496-10, ITU H.264.
    //!
    //! An AVCAttributes object is built by transmitting AVC access units
    //! (aka "NALunits"). The state of the object may change after adding a
    //! "sequence parameter set" NALunit. Initially, an AVCAttributes object
    //! is invalid.
    //!
    class TSDUCKDLL AVCAttributes: public AbstractAudioVideoAttributes
    {
    public:
        //!
        //! Default constructor.
        //!
        AVCAttributes() = default;

        // Implementation of abstract methods.
        // The "binary data" is an AVC access unit.
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
        //! Get AVC profile.
        //! @return AVC profile, 0 if unknown.
        //!
        int profile() const {return _is_valid ? _profile : 0;}

        //!
        //! Get AVC profile name.
        //! @return AVC profile as a string.
        //!
        UString profileName() const;

        //!
        //! Get AVC level.
        //! @return AVC level, 0 if unknown.
        //!
        int level() const {return _is_valid ? _level : 0;}

        //!
        //! Get AVC level name.
        //! @return AVC level as a string.
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
        size_t  _hsize = 0;    // Horizontal size in pixel
        size_t  _vsize = 0;    // Vertical size in pixel
        int     _profile = 0;  // AVC profile
        int     _level = 0;    // AVC level
        uint8_t _chroma = 0;   // Chroma format code (CHROMA_* from tsMPEG.h)
    };
}
