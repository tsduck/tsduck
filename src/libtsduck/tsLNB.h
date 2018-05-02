//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Description of a Low-Noise Block (LNB) converter in a satellite dish.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Description of a Low-Noise Block (LNB) converter in a satellite dish.
    //! @ingroup hardware
    //!
    //! Note: all frequencies are in Hz in parameters.
    //!
    //! Characteristics of a univeral LNB:
    //! - Low frequency: 9.750 GHz
    //! - High frequency: 10.600 GHz
    //! - Switch frequency: 11.700 GHz
    //!
    class TSDUCKDLL LNB
    {
    public:
        static const uint64_t UNIVERSAL_LNB_LOW_FREQUENCY    = TS_UCONST64(9750000000);  //!< Univeral LNB low frequency.
        static const uint64_t UNIVERSAL_LNB_HIGH_FREQUENCY   = TS_UCONST64(10600000000); //!< Univeral LNB high frequency.
        static const uint64_t UNIVERSAL_LNB_SWITCH_FREQUENCY = TS_UCONST64(11700000000); //!< Univeral LNB switch frequency.

        static const LNB Universal; //!< Universal LNB.
        static const LNB Null;      //!< Null LNB: satellite frequency == intermediate frequency.

        //!
        //! Default constructor.
        //! The object is initialized with the characteristics of a univeral LNB.
        //!
        LNB() :
            _low_frequency(UNIVERSAL_LNB_LOW_FREQUENCY),
            _high_frequency(UNIVERSAL_LNB_HIGH_FREQUENCY),
            _switch_frequency(UNIVERSAL_LNB_SWITCH_FREQUENCY)
        {
        }

        //!
        //! Constructor of an LNB without high band.
        //! @param [in] frequency Low frequency.
        //!
        LNB(uint64_t frequency) :
            _low_frequency(frequency),
            _high_frequency(0),
            _switch_frequency(0)
        {
        }

        //!
        //! Constructor of an LNB with low and high band.
        //! @param [in] low_frequency Low frequency.
        //! @param [in] high_frequency High frequency.
        //! @param [in] switch_frequency Switch frequency.
        //!
        LNB(uint64_t low_frequency, uint64_t high_frequency, uint64_t switch_frequency) :
            _low_frequency(low_frequency),
            _high_frequency(high_frequency),
            _switch_frequency(switch_frequency)
        {
        }

        //!
        //! Constructor from a normalized string representation of an LNB.
        //! @param [in] s Normalized string representation of the LNB.
        //! In strings, all values are in MHz. All frequencies are set to zero in case of error.
        //! - "freq" if the LNB has no high band.
        //! - "low,high,switch" if the LNB has a high band.
        //!
        LNB(const UString& s)  :
            _low_frequency(0),
            _high_frequency(0),
            _switch_frequency(0)
        {
            set(s);
        }

        //!
        //! Check if valid (typically after initializing or converting from string).
        //! @return True if valid.
        //!
        bool isValid() const
        {
            return _low_frequency > 0;
        }

        //!
        //! Get the LNB low frequency.
        //! @return The LNB low frequency.
        //!
        uint64_t lowFrequency() const
        {
            return _low_frequency;
        }

        //!
        //! Get the LNB high frequency.
        //! @return The LNB high frequency.
        //!
        uint64_t highFrequency() const
        {
            return _high_frequency;
        }

        //!
        //! Get the LNB switch frequency.
        //! @return The LNB switch frequency.
        //!
        uint64_t switchFrequency() const
        {
            return _switch_frequency;
        }

        //!
        //! Check if the LNB has a high band.
        //! @return True if the LNB has a high ban.
        //!
        bool hasHighBand() const
        {
            return _high_frequency > 0 && _switch_frequency > 0;
        }

        //!
        //! Check if the specified satellite carrier frequency uses the high band of the LNB.
        //! @param [in] satellite_frequency Satellite carrier frequency in Hz.
        //! @return True if @a satellite_frequency is in the high ban of the LBN.
        //!
        bool useHighBand(uint64_t satellite_frequency) const {return hasHighBand() && satellite_frequency >= _switch_frequency;}

        //!
        //! Compute the intermediate frequency from a satellite carrier frequency.
        //!
        //! The satellite carrier frequency is used to carry the signal from the
        //! satellite to the dish. This value is public and is stored in the NIT
        //! for instance. The intermediate frequency is used to carry the signal
        //! from the dish's LNB to the receiver. The way this frequency is
        //! computed depends on the characteristics of the LNB. The intermediate
        //! frequency is the one that is used by the tuner in the satellite
        //! receiver.
        //!
        //! @param [in] satellite_frequency Satellite carrier frequency in Hz.
        //! @return Intermediate frequency between the LNB and the tuner.
        //!
        uint64_t intermediateFrequency(uint64_t satellite_frequency) const;

        //!
        //! Convert the LNB to a string object
        //! @return A normalized representation of the LNB. All values are in MHz.
        //! - "freq" if the LNB has no high band.
        //! - "low,high,switch" if the LNB has a high band.
        //!
        operator UString() const;

        //!
        //! Interpret a string as an LNB value.
        //! @param [in] s Normalized string representation of the LNB.
        //! In strings, all values are in MHz. All frequencies are set to zero in case of error.
        //! - "freq" if the LNB has no high band.
        //! - "low,high,switch" if the LNB has a high band.
        //! @return True on success, false on error.
        //!
        bool set(const UString& s);

        //!
        //! Set values of an LNB without high band.
        //! @param [in] frequency Low frequency.
        //!
        void set(uint64_t frequency)
        {
            _low_frequency    = frequency;
            _high_frequency   = 0;
            _switch_frequency = 0;
        }

        //!
        //! Set values of an LNB with low and high band.
        //! @param [in] low_frequency Low frequency.
        //! @param [in] high_frequency High frequency.
        //! @param [in] switch_frequency Switch frequency.
        //!
        void set(uint64_t low_frequency, uint64_t high_frequency, uint64_t switch_frequency)
        {
            _low_frequency    = low_frequency;
            _high_frequency   = high_frequency;
            _switch_frequency = switch_frequency;
        }

        //!
        //! Set values of a univeral LNB.
        //!
        void setUniversalLNB()
        {
            _low_frequency    = UNIVERSAL_LNB_LOW_FREQUENCY;
            _high_frequency   = UNIVERSAL_LNB_HIGH_FREQUENCY;
            _switch_frequency = UNIVERSAL_LNB_SWITCH_FREQUENCY;
        }

    private:
        // Characteristics of an LNB.
        uint64_t _low_frequency;
        uint64_t _high_frequency;
        uint64_t _switch_frequency;
    };
}

//!
//! Output operator for LNB.
//! @param [in,out] strm Output text stream.
//! @param [in] lnb LNB.
//! @return A reference to @a strm.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::LNB& lnb)
{
    return strm << ts::UString(lnb);
}
