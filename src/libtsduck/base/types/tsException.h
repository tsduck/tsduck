//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for all exceptions in TSDuck.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Base class for all exceptions in TSDuck.
    //! @ingroup cpp
    //!
    class TSDUCKDLL Exception : public std::exception
    {
        TS_RULE_OF_FIVE(Exception, noexcept override);
    private:
        UString _what;
        mutable std::string _utf8;
    public:
        //!
        //! Constructor.
        //! @param [in] message Error message for the exception.
        //!
        explicit Exception(const UString& message);

        //!
        //! Constructor.
        //! @param [in] message Error message for the exception.
        //! @param [in] error System error code causing the exception.
        //!
        Exception(const UString& message, int error);

        //!
        //! Get the error message as a C-string.
        //! @return The error message as a C-string (valid as long as this instance exists).
        //!
        virtual const char* what() const noexcept override;
    };
}

//!
//! @hideinitializer
//! This macro declares an exception as a subclass of ts::Exception.
//! @param [in] name Name of the exception class.
//!
// Note: With clang (LLVM), we can track the use of inlined virtual tables using warning -Wweak-vtables.
// But exceptions are ... exceptions. There is no way to declare them easily in one macro in a
// header file without weak virtual tables. So, with clang only, we disable this warning.
//
#define TS_DECLARE_EXCEPTION(name)                            \
    TS_PUSH_WARNING()                                         \
    TS_LLVM_NOWARNING(weak-vtables)                           \
    class name: public ts::Exception                          \
    {                                                         \
    public:                                                   \
        /** Constructor.                                   */ \
        /** @param [in] w Error message for the exception. */ \
        explicit name(const ts::UString& w) :                 \
            ts::Exception(u ## #name u": " + w)               \
        {                                                     \
        }                                                     \
        /** Constructor.                                   */ \
        /** @param [in] w Error message for the exception. */ \
        /** @param [in] code System error code.            */ \
        explicit name(const ts::UString& w, int code) :       \
            ts::Exception(u ## #name u": " + w, code)         \
        {                                                     \
        }                                                     \
        /** Constructor.                                   */ \
        /** @param [in] code System error code.            */ \
        explicit name(int code) :                             \
            ts::Exception(u ## #name, code)                   \
        {                                                     \
        }                                                     \
    };                                                        \
    TS_POP_WARNING()                                          \
    typedef int TS_UNIQUE_NAME(for_trailing_semicolon)

//!
//! Locate the source of the exception in the Exception constructor message string.
//!
#define TS_SRCLOC __FILE__ ":" TS_SLINE ": "

//
// Some "standard" exceptions
//
namespace ts {
    //!
    //! Exception for generic invalid value error.
    //! @ingroup cpp
    //!
    TS_DECLARE_EXCEPTION(InvalidValue);
    //!
    //! Uninitialized variable error.
    //! @ingroup cpp
    //!
    TS_DECLARE_EXCEPTION(UninitializedVariable);
    //!
    //! Unimplemented method error.
    //! @ingroup cpp
    //!
    TS_DECLARE_EXCEPTION(UnimplementedMethod);
    //!
    //! Implementation error.
    //! @ingroup cpp
    //!
    TS_DECLARE_EXCEPTION(ImplementationError);
}
