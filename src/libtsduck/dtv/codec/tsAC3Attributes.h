//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        AC3Attributes() = default;

        // Implementation of abstract methods.
        // The "binary data" is an audio frame or PES payload.
        virtual bool moreBinaryData(const uint8_t*, size_t) override;
        virtual UString toString() const override;

        //!
        //! Check if this is Enhanced-AC-3.
        //! @return True for Enhanced-AC-3, false for AC-3.
        //!
        bool isEnhancedAC3() const { return _is_valid && _eac3; }

        //!
        //! Bitstream identification ("bsid").
        //! @return The bitstream identification ("bsid"), see ETSI TS 102 366.
        //!
        int bitstreamId() const { return _is_valid ? _bsid : 0; }

        //!
        //! Bitstream mode ("bsmod", metadata info).
        //! @return The bitstream mode ("bsmod", metadata info), see ETSI TS 102 366.
        //!
        int bitstreamMode() const { return _is_valid ? _bsmod : 0; }

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
        int audioCodingMode() const { return _is_valid ? _acmod : 0; }

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
        int samplingFrequency() const { return _is_valid ? _sampling_freq : 0; }

        //!
        //! Check if this is Dolby Surround.
        //! @return True if this is Dolby Surround.
        //!
        bool dolbySurround() const { return _is_valid && _surround; }

        //!
        //! Rebuild a component_type for AC-3 descriptors.
        //! @return The component_type value for an AC-3 descriptor. See ETSI 300 468 V1.9.1, annex D.1.
        //!
        uint8_t componentType() const;

    private:
        bool _eac3 = false;        // Enhanced-AC-3, not AC-3
        bool _surround = false;    // Dolby Surround
        int  _bsid = 0;            // See ETSI TS 102 366
        int  _bsmod = 0;           // See ETSI TS 102 366
        int  _acmod = 0;           // See ETSI TS 102 366
        int  _sampling_freq = 0;   // In Hz

        // Extract 'bsmod' from an Enhanced-AC-3 frame. Return 0 if not found.
        int extractEAC3bsmod (const uint8_t*, size_t);
    };
}
