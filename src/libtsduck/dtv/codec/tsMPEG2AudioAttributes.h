//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        MPEG2AudioAttributes() = default;

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
        uint32_t _header = 0;          // Last audio frame header
        int      _layer = 0;
        BitRate  _bitrate = 0;         // In kb/s
        int      _sampling_freq = 0;   // In Hz
        int      _mode = 0;            // See ISO 11172-3
        int      _mode_extension = 0;  // See ISO 11172-3
    };
}
