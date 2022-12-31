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
//!  State of a modulated broadcast signal.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsVariable.h"
#include "tsStringifyInterface.h"
#include "tsDisplayInterface.h"

namespace ts {
    //!
    //! State of a modulated broadcast signal.
    //! @ingroup hardware
    //!
    //! The type of information on the state of a modulated broadcast signal highly
    //! depends on the operating systems and drivers. The units also differ. The
    //! value can be in decibels or percentage. So, this structure contains many
    //! different fields which can be set or not.
    //!
    class TSDUCKDLL SignalState : public StringifyInterface, public DisplayInterface
    {
    public:
        //!
        //! Unit of a state value.
        //!
        enum class Unit {
            COUNTER,  //!< A raw counter value, unit is 1, whatever it means.
            PERCENT,  //!< A value from 0 to 100.
            MDB,      //!< Milli-decibel, unit of 0.001 dB.
        };

        //!
        //! Implementation of a state value.
        //! Since different operating systems or drivers may return different units,
        //! there is no standard way to represent values. Each value comes with its unit.
        //!
        class TSDUCKDLL Value : public StringifyInterface
        {
        public:
            int64_t value;  //!< The value.
            Unit    unit;   //!< The unit of @a value.

            //!
            //! Constructor.
            //! @param [in] v Initial value.
            //! @param [in] u Unit of @a v.
            //!
            explicit Value(int64_t v = 0, Unit u = Unit::COUNTER) : value(v), unit(u) {}

            // Implementation of interfaces.
            virtual UString toString() const override;
        };

        //!
        //! Set to true when the signal is confirmed to be locked at the input of the demodulator.
        //!
        bool signal_locked;

        //!
        //! Signal strength.
        //!
        Variable<Value> signal_strength;

        //!
        //! Signal to noise ratio (SNR).
        //!
        Variable<Value> signal_noise_ratio;

        //!
        //! Bit error rate (BER).
        //!
        Variable<Value> bit_error_rate;

        //!
        //! Packet error rate (PER).
        //!
        Variable<Value> packet_error_rate;

        //!
        //! Constructor.
        //!
        SignalState();

        //!
        //! Clear content, reset all values, they become "unset"
        //!
        virtual void clear();

        // Implementation of interfaces.
        virtual UString toString() const override;
        virtual std::ostream& display(std::ostream& strm, const UString& margin = UString(), int level = Severity::Info) const override;

        //!
        //! Set a percentage value from a raw driver value.
        //! This method is a utility for system-specific implementations.
        //! @param [out] field Pointer-to-member for the field to update.
        //! @param [in] value Raw value as returned by the driver. The @a field
        //! is set with a value from 0 to 100, based on the range @a min to @a max.
        //! @param [in] min Minimum value corresponding to 0 %.
        //! @param [in] max Maximum value corresponding to 100 %.
        //!
        void setPercent(Variable<Value> SignalState::* field, int64_t value, int64_t min, int64_t max);
    };
}
