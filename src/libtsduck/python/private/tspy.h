//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup python
//!  Base definitions for the TSDuck Python bindings (C++ implementation).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

//!
//! @hideinitializer
//! Attribute to export a function to Python.
//!
#define TSDUCKPY \
    TS_GCC_NOWARNING(missing-prototypes) \
    TS_LLVM_NOWARNING(missing-prototypes) \
    extern "C" TSDUCKDLL

namespace ts {
    //!
    //! Namespace for internal utilities to support Python bindings.
    //!
    namespace py {
        //!
        //! Convert a UTF-16 buffer in a UString.
        //! @param [in] buffer Address of a buffer with UTF-16 content.
        //! @param [in] size Size in bytes of the buffer.
        //! @return The converted string.
        //!
        UString ToString(const uint8_t* buffer, size_t size);

        //!
        //! Convert a UTF-16 buffer in a list of UString.
        //! The various strings in the buffer are separated with 0xFFFF code points (invalid UTF-16 value)
        //! @param [in] buffer Address of a buffer with UTF-16 content.
        //! @param [in] size Size in bytes of the buffer.
        //! @return The converted strings.
        //!
        UStringList ToStringList(const uint8_t* buffer, size_t size);

        //!
        //! Convert a string into a UTF-16 buffer.
        //! @param [in] str The initial string.
        //! @param [out] buffer Address of a buffer where the string is returned in UTF-16 format.
        //! @param [in,out] size Initial/maximum size in bytes of the buffer. Upon return, contains the written size in bytes.
        //!
        void FromString(const UString& str, uint8_t* buffer, size_t* size);
    }
}
