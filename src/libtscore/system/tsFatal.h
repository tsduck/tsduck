//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Handle some fatal situations.
//!
//!  *Important*: By default, without specific "nothrow" form of allocator,
//!  the operator "new" and std::make_shared() never return a null pointer.
//!  They throw std::bad_alloc in case of allocation failure. Therefore, it
//!  is useless to test such new pointers for a null value and trigger a
//!  fatal error.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

//!
//! Build a string literal for fatal error messages.
//! @ingroup app
//! @param literal 8-bit string literal
//!
#define TS_FATALMSG(literal) "\n\n*** " literal ", aborting...\n\n"

//!
//! @hideinitializer
//! Abort the application with a fatal error message.
//! @ingroup app
//! @param literal 8-bit string literal error message.
//! @see ts::FatalError
//!
#define TS_FATAL(literal) ts::FatalError(TS_FATALMSG(literal), sizeof(TS_FATALMSG(literal)) - 1)

namespace ts {
    //!
    //! Handle a fatal error.
    //! An emergency message is output and the application is terminated.
    //! @ingroup app
    //! @param [in] message Address of an emergency error message to output.
    //! @param [in] length Length of @a message. The caller must specify @a length
    //! in a static way. In that kind of fatal error, we can't even dare to call strlen().
    //!
    [[noreturn]] TSCOREDLL void FatalError(const char* message, size_t length);
}
