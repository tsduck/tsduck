//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard, Matthew Sweet
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Description of a Unicable switch (satellite reception).
//!  Based on a pull request from Matthew Sweet.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsUString.h"
#include "tsReport.h"
#include "tsLNB.h"

namespace ts {
    //!
    //! Description of a Unicable switch (satellite reception).
    //! @ingroup libtsduck hardware
    //!
    //! There are two unicable specifications:
    //!
    //! * EN50494 (Unicable I)
    //! * EN50607 (Unicable II)
    //!
    //! These use the DiSEqC hardware-level interface, while permitting several
    //! receivers to share the same COAXial cable.
    //!
    //! This massively reduces the cabling in multiple-dwelling units such as
    //! hotels, apartments and large offices.
    //!
    //! The standard supports both unidirectional (bus specification 1.x) and
    //! bidirectional (bus specification 2.x) commands.
    //!
    //! In normal use, receivers need only to use unidirectional commands.
    //!
    //! To achieve this, each receiver is attached to the bus via a power-passing
    //! combiner (so that the LNB controllers do not attempt to back-power each other),
    //! and the passive "watching a channel" state is to send a low voltage and no tone.
    //!
    //! To send a command, you:
    //!
    //! 1. assert the high voltage;
    //! 2. wait for a settling time;
    //! 3. send the command;
    //! 4. if it is a bidirectional command await a reply (not supported);
    //! 5. wait for a "clear channel" time;
    //! 6. assert the low voltage;
    //! 7. wait for a settling time;
    //!
    //! It is possible for the commands from two or more receivers to collide,
    //! in which case they will probably both not be actioned.
    //! The specifications suggest a detection and a random-backoff and
    //! retransmit mechanism, which is not implemented here.
    //!
    //! Each receiver is assigned a "user band" and a "user band frequency"
    //! 1. The specification does NOT assign bands to frequencies - switches
    //!    come with a table.
    //! 2. The receiver sends commands for its user-band, and only ever tunes to
    //!    its user-band-frequency.
    //! 3. The switch interprets commands, and frequency-shifts the required
    //!    signal to the receiver's user-band-frequency.
    //! 4. Both versions of the specification have extended versions of the
    //!    "channel change" command which includes a PIN code, the idea being
    //!    that the switch should ignore commands where the PIN is incorrect.
    //!    This is intended to stop the neighbour from hijacking "your"
    //!    user-band. This version of the command is not used here.
    //!
    //! The channel-change command contains the following parameters:
    //! 1. The user-band assigned to the receiver;
    //! 2. The satellite position to be tuned to;
    //! 3. The polarity to be tuned to;
    //! 4. The frequency range to be tuned to;
    //! 5. A "tuning word"
    //!
    //! The specifications combines 2,3 and 4 into a "bank", and somewhat confusingly
    //! (and unnecessarily) tries to compare the bank to DiSEqC uncommitted (1.1), and
    //! committed (1.0) switch positions.
    //!
    //! Unicable I supports up to 8 user-bands on a single piece of COAX, and two
    //! satellite positions. The calculation of the tuning word also uses the
    //! user-band frequency.
    //!
    //! Unicable II supports up to 32 user-bands on a single piece of COAX,
    //! and up to 64 satellite positions. The calculation of the tuning word
    //! does not include the user-band frequency.
    //!
    //! Experience of various switches on the market suggest that it is common
    //! for the switches to support a power-of-two number of satellites, and
    //! ignore unsupported satellite-position bits in commands, therefore for
    //! a switch supporting four satellite positions 0,4,8,...,60 all alias to
    //! the same satellite.
    //!
    class TSDUCKDLL Unicable : public StringifyInterface
    {
    public:
        //!
        //! Unicable version, must be 1 (EN50494) or 2 (EN50607).
        //!
        uint8_t version = 0;
        //!
        //! User band slot, must be in range 1-8 (Unicable I) or 1-32 (Unicable II).
        //!
        uint8_t user_band_slot = 0;
        //!
        //! User band frequency in Hz.
        //! This is the frequency between the tuner and the Unicable switch.
        //! This frequency is statically assigned to the receiver. Each receiver
        //! on the COAX has a specific user band frequency.
        //!
        //! Note: In the string representation of the Unicable parameters, the
        //! user band frequency is in MHz by convention. However, in C++ code,
        //! all frequencies are in Hz for consistency.
        //!
        uint64_t user_band_frequency = 0;

        //!
        //! Default constructor.
        //!
        Unicable() = default;

        //!
        //! Check if the content of this object is valid and consistent.
        //! @return True if the content of this object is valid and consistent.
        //!
        bool isValid() const;

        // Implementation of StringifyInterface
        virtual UString toString() const override;

        //!
        //! Decode a string containing a Unicable representation.
        //! @param [in] str String containing the Unicable representation.
        //! Format: \<version>,\<userband slot>,\<userband frequency in MHz>.
        //! @param [in,out] report Where to log errors.
        //! @return True on success, false on error.
        //!
        bool decode(const UString& str, Report& report);

        //!
        //! Get a string describing the format of Unicable strings.
        //! Typically used in help messages.
        //! @return A constant reference to the description string.
        //!
        static const UString& StringFormat();

        //!
        //! Get the default LNB for Unicable switches.
        //! @param [out] lnb The returned LNB.
        //! @param [in,out] report Where to log errors.
        //! @return True on success, false on error.
        //!
        static bool GetDefaultLNB(LNB& lnb, Report& report);

        //!
        //! Unicable 1 step-size in MHz.
        //!
        static constexpr uint32_t EN50494_STEP_SIZE = 4;
    };
}
