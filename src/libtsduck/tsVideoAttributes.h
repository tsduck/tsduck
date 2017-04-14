//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Video attributes for MPEG-1 and MPEG-2.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAudioVideoAttributes.h"
#include "tsMPEG.h"
#include "tsNames.h"

namespace ts {

    // A VideoAttributes object is built by transmitting video units (starting
    // with a 00 00 01 xx start code). The state of the object may change
    // after adding a "sequence header" unit and its following unit.
    // When the later is a "sequence extension" unit, this is MPEG-2 video.
    // Initially, a VideoAttributes is invalid.

    class TSDUCKDLL VideoAttributes: public AbstractAudioVideoAttributes
    {
    public:
        // Default constructor
        VideoAttributes(): AbstractAudioVideoAttributes(), _waiting (false) {}

        // Implementation of abstract methods.
        // The "binary data" is a video unit, starting with a 00 00 01 xx start code.
        virtual bool moreBinaryData (const void*, size_t);
        virtual operator std::string () const;

        // Get size
        size_t horizontalSize() const {return _is_valid ? _hsize : 0;}
        size_t verticalSize() const {return _is_valid ? _vsize : 0;}

        // Get aspect ratio (code values are AR_* from tsMPEG.h)
        uint8_t aspectRatioCode() const {return _is_valid ? _ar_code : 0;}
        std::string aspectRatioName() const {return _is_valid ? names::AspectRatio (_ar_code) : "";}

        // Refresh mode (both can be false if unspecifed)
        bool progressive() const {return _is_valid ? _progressive : false;}
        bool interlaced() const {return _is_valid ? _interlaced : false;}
        std::string refreshModeName() const;

        // Get chroma format (code values are CHROMA_* from tsMPEG.h, 0 if unknown)
        uint8_t chromaFormat() const {return _is_valid ? _cf_code : 0;}
        std::string chromaFormatName() const {return _is_valid ? names::ChromaFormat (_cf_code) : "";}
        
        // Get frame rate: approximate value per second, per 100 seconds,
        // using numerator and divider.
        size_t frameRate() const {return (frameRate100() + 99) / 100;}
        size_t frameRate100() const {return _is_valid && _fr_div != 0 ? (100 * _fr_num) / _fr_div : 0;}
        size_t frameRateNumerator() const {return _is_valid ? _fr_num : 0;}
        size_t frameRateDivider() const {return _is_valid ? _fr_div : 1;}
        std::string frameRateName() const;

        // Maximum bitrate
        BitRate maximumBitRate() const {return _is_valid ? _bitrate * 400: 0;}

        // Video Buffering Verifier size in bits
        size_t vbvSize() const {return _is_valid ? _vbv_size * 16 * 1024: 0;}

    private:
        // Actual values, when _is_valid == true
        size_t  _hsize;    // Horizontal size in pixel
        size_t  _vsize;    // Vertical size in pixel
        uint8_t   _ar_code;  // Aspect ratio code (AR_* from tsMPEG.h)
        bool    _progressive;
        bool    _interlaced;
        uint8_t   _cf_code;  // Chroma format code (CHROMA_* from tsMPEG.h)
        size_t  _fr_num;   // Frame rate numerator
        size_t  _fr_div;   // Frame rate divider
        BitRate _bitrate;  // Maximum bit rate
        size_t  _vbv_size; // Video Buffering Verifier size in bits

        // Temporary values from a "sequence header" unit
        bool    _waiting;     // Previous unit was a "sequence header"
        size_t  _sh_hsize;    // Horizontal size in pixel
        size_t  _sh_vsize;    // Vertical size in pixel
        uint8_t   _sh_ar_code;  // Aspect ratio code (AR_* from tsMPEG.h)
        size_t  _sh_fr_code;  // Frame rate code
        BitRate _sh_bitrate;  // Maximum bit rate
        size_t  _sh_vbv_size; // Video Buffering Verifier size in bits

        // Extract frame rate fields from frame rate code
        static size_t FRNum (uint8_t);
        static size_t FRDiv (uint8_t);
    };
}
