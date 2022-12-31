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
//!  Video attributes for MPEG-1 and MPEG-2.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAudioVideoAttributes.h"

namespace ts {
    //!
    //! Video attributes for MPEG-1 and MPEG-2.
    //! @ingroup mpeg
    //!
    //! A VideoAttributes object is built by transmitting video units (starting
    //! with a 00 00 01 xx start code). The state of the object may change
    //! after adding a "sequence header" unit and its following unit.
    //! When the later is a "sequence extension" unit, this is MPEG-2 video.
    //! Initially, a VideoAttributes is invalid.
    //!
    class TSDUCKDLL MPEG2VideoAttributes: public AbstractAudioVideoAttributes
    {
    public:
        //!
        //! Default constructor.
        //!
        MPEG2VideoAttributes();

        // Implementation of abstract methods.
        // The "binary data" is a video unit, starting with a 00 00 01 xx start code.
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
        //! Get display aspect ratio.
        //! @return Display aspect ratio, code values are AR_* from tsMPEG.h.
        //!
        uint8_t aspectRatioCode() const {return _is_valid ? _ar_code : 0;}

        //!
        //! Get display aspect ratio name.
        //! @return Display aspect ratio as a string.
        //!
        UString aspectRatioName() const;

        //!
        //! Check if refresh mode is progressive.
        //! @return True if refresh mode is progressive. Note that progressive() and
        //! interlaced() can return false both if the refresh mode is unspecifed.
        //!
        bool progressive() const {return _is_valid ? _progressive : false;}

        //!
        //! Check if refresh mode is interlaced.
        //! @return True if refresh mode is interlaced. Note that progressive() and
        //! interlaced() can return false both if the refresh mode is unspecifed.
        //!
        bool interlaced() const {return _is_valid ? _interlaced : false;}

        //!
        //! Get the refresh mode name.
        //! @return The refresh mode as a string.
        //!
        UString refreshModeName() const;

        //!
        //! Get chroma format.
        //! @return Chroma format, code values are CHROMA_* from tsMPEG.h, 0 if unknown.
        //!
        uint8_t chromaFormat() const {return _is_valid ? _cf_code : 0;}

        //!
        //! Get chroma format name.
        //! @return Chroma format as a string.
        //!
        UString chromaFormatName() const;

        //!
        //! Get frame rate: approximate value per second.
        //! @return Approximate frame rate per second.
        //! Example: return 30 for NTSC (actual NTSC rate is 30/1.001 = 29.97).
        //!
        size_t frameRate() const {return (frameRate100() + 99) / 100;}

        //!
        //! Get frame rate per 100 seconds.
        //! @return Frame rate per 100 second.
        //! Example: return 2997 for NTSC (actual NTSC rate is 30/1.001 = 29.97).
        //!
        size_t frameRate100() const {return _is_valid && _fr_div != 0 ? (100 * _fr_num) / _fr_div : 0;}

        //!
        //! Get frame rate numerator.
        //! @return Frame rate numerator.
        //! Example: return 30000 for NTSC (actual NTSC rate is 30/1.001 = 29.97).
        //!
        size_t frameRateNumerator() const {return _is_valid ? _fr_num : 0;}

        //!
        //! Get frame rate divider.
        //! @return Frame rate divider.
        //! Example: return 1001 for NTSC (actual NTSC rate is 30/1.001 = 29.97).
        //!
        size_t frameRateDivider() const {return _is_valid ? _fr_div : 1;}

        //!
        //! Get frame rate name.
        //! @return Frame rate as a string.
        //! Example: return "@29.97 Hz" for NTSC (actual NTSC rate is 30/1.001 = 29.97).
        //!
        UString frameRateName() const;

        //!
        //! Maximum bitrate.
        //! @return Maximum bitrate in bits/second.
        //!
        uint32_t maximumBitRate() const {return _is_valid ? _bitrate * 400: 0;}

        //!
        //! Video Buffering Verifier size in bits.
        //! @return Video Buffering Verifier size in bits.
        //!
        size_t vbvSize() const {return _is_valid ? _vbv_size * 16 * 1024: 0;}

    private:
        // Actual values, when _is_valid == true
        size_t   _hsize;       // Horizontal size in pixel
        size_t   _vsize;       // Vertical size in pixel
        uint8_t  _ar_code;     // Aspect ratio code (AR_* from tsMPEG.h)
        bool     _progressive;
        bool     _interlaced;
        uint8_t  _cf_code;     // Chroma format code (CHROMA_* from tsMPEG.h)
        size_t   _fr_num;      // Frame rate numerator
        size_t   _fr_div;      // Frame rate divider
        uint32_t _bitrate;     // Maximum bit rate
        size_t   _vbv_size;    // Video Buffering Verifier size in bits

        // Temporary values from a "sequence header" unit
        bool     _waiting;     // Previous unit was a "sequence header"
        size_t   _sh_hsize;    // Horizontal size in pixel
        size_t   _sh_vsize;    // Vertical size in pixel
        uint8_t  _sh_ar_code;  // Aspect ratio code (AR_* from tsMPEG.h)
        size_t   _sh_fr_code;  // Frame rate code
        uint32_t _sh_bitrate;  // Maximum bit rate
        size_t   _sh_vbv_size; // Video Buffering Verifier size in bits

        // Extract frame rate fields from frame rate code
        static size_t FRNum(uint8_t);
        static size_t FRDiv(uint8_t);
    };
}
