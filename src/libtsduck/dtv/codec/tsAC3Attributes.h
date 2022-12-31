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
//!  AC-3 (DD) and Enhanced-AC-3 (DD+) audio attributes
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractAudioVideoAttributes.h"

namespace ts {
    //!
    //! AC-3 (DD) and Enhanced-AC-3 (DD+) audio attributes.
    //!
    //! See ETSI TS 102 366 for the specification and encoding of AC-3 and Enhanced-AC-3.
    //!
    //! An AC3Attributes object is built by transmitting audio frames from
    //! PES payloads. Initially, an AC3Attributes object is invalid.
    //!
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AC3Attributes: public AbstractAudioVideoAttributes
    {
    public:
        //!
        //! Default constructor.
        //!
        AC3Attributes();

        // Implementation of abstract methods.
        // The "binary data" is an audio frame or PES payload.
        virtual bool moreBinaryData(const uint8_t*, size_t) override;
        virtual UString toString() const override;

        //!
        //! Check if this is Enhanced-AC-3.
        //! @return True for Enhanced-AC-3, false for AC-3.
        //!
        bool isEnhancedAC3() const {return _is_valid && _eac3;}

        //!
        //! Bitstream identification ("bsid").
        //! @return The bitstream identification ("bsid"), see ETSI TS 102 366.
        //!
        int bitstreamId() const {return _is_valid ? _bsid : 0;}

        //!
        //! Bitstream mode ("bsmod", metadata info).
        //! @return The bitstream mode ("bsmod", metadata info), see ETSI TS 102 366.
        //!
        int bitstreamMode() const {return _is_valid ? _bsmod : 0;}

        //!
        //! String representation of bitstream mode.
        //! @return The string representation of bitstream mode.
        //! @see bitstreamMode()
        //!
        UString bitstreamModeDescription() const;

        //!
        //! Audio coding mode ("acmod").
        //! @return The audio coding mode ("acmod"), see ETSI TS 102 366.
        //!
        int audioCodingMode() const {return _is_valid ? _acmod : 0;}

        //!
        //! String representation of audio coding mode.
        //! @return The string representation of audio coding mode.
        //! @see audioCodingMode()
        //!
        UString audioCodingDescription() const;

        //!
        //! Sampling frequency in Hz.
        //! @return The sampling frequency in Hz.
        //!
        int samplingFrequency() const {return _is_valid ? _sampling_freq : 0;}

        //!
        //! Check if this is Dolby Surround.
        //! @return True if this is Dolby Surround.
        //!
        bool dolbySurround() const {return _is_valid && _surround;}

        //!
        //! Rebuild a component_type for AC-3 descriptors.
        //! @return The component_type value for an AC-3 descriptor. See ETSI 300 468 V1.9.1, annex D.1.
        //!
        uint8_t componentType() const;

    private:
        bool _eac3;            // Enhanced-AC-3, not AC-3
        bool _surround;        // Dolby Surround
        int  _bsid;            // See ETSI TS 102 366
        int  _bsmod;           // See ETSI TS 102 366
        int  _acmod;           // See ETSI TS 102 366
        int  _sampling_freq;   // In Hz

        // Extract 'bsmod' from an Enhanced-AC-3 frame. Return 0 if not found.
        int extractEAC3bsmod (const uint8_t*, size_t);
    };
}
