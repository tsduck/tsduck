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
//!  Audio attributes for MPEG-1 / MPEG-2
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAudioVideoAttributes.h"
#include "tsBitRate.h"

namespace ts {
    //!
    //! Audio attributes for MPEG-1 / MPEG-2 audio.
    //! @ingroup mpeg
    //!
    //! An MPEG2AudioAttributes object is built by transmitting audio frames from
    //! PES payloads. Initially, an AudioAttributes object is invalid.
    //!
    class TSDUCKDLL MPEG2AudioAttributes: public AbstractAudioVideoAttributes
    {
    public:
        //!
        //! Default constructor.
        //!
        MPEG2AudioAttributes();

        // Implementation of abstract methods.
        // The "binary data" is an audio frame or PES payload.
        virtual bool moreBinaryData(const uint8_t*, size_t) override;
        virtual UString toString() const override;

        //!
        //! MPEG audio layer.
        //! @return The MPEG audio layer (1-3, 0 if unknown).
        //!
        int layer() const {return _is_valid ? _layer : 0;}

        //!
        //! MPEG audio layer name.
        //! @return A string describing the MPEG audio layer.
        //!
        UString layerName() const;

        //!
        //! Bitrate in bits/second.
        //! @return The bitrate in bits/second (0 if variable bitrate).
        //!
        BitRate bitrate() const {return _is_valid ? 1024 * _bitrate : 0;}

        //!
        //! Sampling frequency in Hz.
        //! @return The sampling frequency in Hz.
        //!
        int samplingFrequency() const {return _is_valid ? _sampling_freq : 0;}

        //!
        //! Get the mono/stereo mode.
        //! @return The mono/stereo mode (see ISO 11172-3).
        //!
        int stereoMode() const {return _is_valid ? _mode : 0;}

        //!
        //! Get the mono/stereo mode extension.
        //! @return The mono/stereo mode extension (see ISO 11172-3).
        //!
        int stereoModeExtension() const {return _is_valid ? _mode_extension : 0;}

        //!
        //! Mono/stereo mode name.
        //! @return A string describing the mono/stereo mode .
        //!
        UString stereoDescription() const;

    private:
        uint32_t _header;          // Last audio frame header
        int      _layer;
        BitRate  _bitrate;         // In kb/s
        int      _sampling_freq;   // In Hz
        int      _mode;            // See ISO 11172-3
        int      _mode_extension;  // See ISO 11172-3
    };
}
