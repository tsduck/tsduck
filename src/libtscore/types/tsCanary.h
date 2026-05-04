//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Canary class to detect memory corruption in instable environments.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"

namespace ts {
    //!
    //! Canary class to detect memory corruption in instable environments.
    //! @ingroup libtscore cpp
    //!
    //! Some libraries or operating systems associate a "user-pointer" to an event or some equivalent
    //! concept. This user-pointer usually is a user-allocated data structure. It is passed back to the
    //! application by the library or operating system at some point in the future.
    //!
    //! The application must ensure that the user-allocated structure remains valid as long as the
    //! user-pointer is stored in the library or operating system. Otherwise, the application may
    //! receive the address of a structure which is no longer valid.
    //!
    //! In some cases, it is complicated to understand how long this user pointer is stored. To track
    //! potential errors in our interpretation of the various libraries or operating systems logics,
    //! we use some internal consistency checks using canaries, values which are altered when the
    //! structure is no longer valid.
    //!
    //! User-defined data structures which are used as user-pointer for external libraries or operating
    //! systems should use a Canary field as first field.
    //!
    class TSCOREDLL Canary
    {
        TS_DEFAULT_COPY_MOVE(Canary);
    private:
        volatile uint32_t _canary = GOOD;             // First field in memory.
        static constexpr uint32_t GOOD = 0x474F4F44;  // "GOOD"
        static constexpr uint32_t BAD = 0x42414453;   // "BADS"

    public:
        //!
        //! Constructor with a "good" default value.
        //!
        Canary() = default;

        //!
        //! The destructor sets a "bad" canary to detect use-after-free in case of incorrect order of usage.
        //!
        ~Canary() { _canary = BAD; }

        //!
        //! Check that a Canary pointer is valid and get an error message.
        //! @param [in] c The address of something that is expected to be a valid Canary.
        //! @return A null pointer if @a c seems a valid pointer, the address of a static error message otherwise.
        //!
        static const UChar* Error(const Canary* c);

        //!
        //! Check that a Canary pointer is valid, get and log an error message on standard error.
        //! @param [in] c The address of something that is expected to be a valid Canary.
        //! @return A null pointer if @a c seems a valid pointer, the address of a static error message otherwise.
        //! In case of invalid pointer, the error message is also displayed on standard error.
        //!
        static const UChar* LogError(const Canary* c);

        //!
        //! Check that a Canary pointer is valid.
        //! @param [in] c The address of something that is expected to be a valid Canary.
        //! @return True if @a c seems a valid pointer, false otherwise.
        //!
        static bool IsValid(const Canary* c) { return Error(c) == nullptr; }
    };
}
