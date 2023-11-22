//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A proxy class to automatically report std::error_code errors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsUString.h"

namespace ts {
    //!
    //! A proxy class to automatically report std::error_code errors.
    //! @ingroup log
    //!
    //! This class derives from std::error_code and can be used in any C++
    //! standard library call which takes an output parameter of type std::error_code.
    //!
    //! The magic is in the destructor of the class. Whenever an instance of this class
    //! is destructed, if the object contains an error, the corresponding error message
    //! is logged in the associated Report object. Therefore, the standard usage pattern
    //! is the following:
    //!
    //! @code
    //! if (!std::filesystem::create_directory(dir, &ErrCodeReport(report, "error creating directory", dir))) {
    //!     // error processing, error message already logged to @c report
    //!     return false;
    //! }
    //! @endcode
    //!
    //! The first important point is the usage of a constructor, directly as a parameter
    //! of the system call, here @c std::filesystem::create_directory. According to C++ rules,
    //! a temporary instance of ErrCodeReport is constructed just before calling @c create_directory.
    //! This instance is then destructed immediately after returning from @c create_directory.
    //! Therefore, in case of error, the error message is automatically logged upon returning
    //! from @c create_directory, before the next instructions.
    //!
    //! The second important point is the usage of the "&" operator in front of the constructor.
    //! This is a trick to allow the direct usage of a constructor in the function argument list.
    //! All system functions which return an error code in a parameter declare that parameter as
    //! a non-constant reference to an instance of std::error_code. Because the reference is not
    //! constant, the parameter must be an @e lvalue. And a constructor call is an @e rvalue, not
    //! an @e lvalue. The operator "&" is redefined in the class and return a non-constant reference
    //! to the object. Thus, the expression becomes an @e lvalue. Note that the choice of "&" is
    //! arbitrary and any other unitary operator could have been used instead.
    //!
    //! Also note that many apparently redundant constructors are defined. This is a solution to
    //! an issue with Microsoft C++. With gcc and clang, calling the first ErrCodeReport constructor
    //! with expressions which are convertible to UString is accepted. However, Microsoft C++ generates
    //! an obscure error. To avoid that error, a constructor must be found with the exact same paramater
    //! types as the values in the constructor call. This is why additional constructors are defined with
    //! various common combinations of parameter types. More constructors may need to be defined with
    //! new code.
    //!
    class TSDUCKDLL ErrCodeReport : public std::error_code
    {
        TS_NOBUILD_NOCOPY(ErrCodeReport);
    public:
        //!
        //! Main constructor.
        //! @param [in,out] report The report to which all error messages are logged in the destructor.
        //! @param [in] message Option message to display before the system error message.
        //! @param [in] object Optional "object" name, to be displayed after @a message.
        //!
        ErrCodeReport(Report& report, const UString& message = UString(), const UString& object = UString()) :
            _report(report), _message(message), _object(object) {}

        //! @cond nodoxygen
        //  Alternative constructors to avoid a Microsoft C++ issue, see comments in the declaration of the class.
        ErrCodeReport(Report& report, const UChar* message, const UString& object = UString()) :
            _report(report), _message(message), _object(object) {}
        ErrCodeReport(Report& report, const UChar* message, const UChar* object) :
            _report(report), _message(message), _object(object) {}
        ErrCodeReport(Report& report, const UChar* message, const fs::path& object) :
            _report(report), _message(message), _object(object.string()) {}
        ErrCodeReport(Report& report, const UString& message, const fs::path& object) :
            _report(report), _message(message), _object(object.string()) {}
        //! @endcond

        //!
        //! Turn a constructor expression into an @e lvalue to be used in system function calls.
        //! See the description of ErrCodeReport for usage patterns.
        //! @return A non-constant reference to this object.
        //! @see ErrCodeReport
        //!
        ErrCodeReport& operator &() { return *this; }

        //!
        //! Destructor.
        //! If this object contains a non-success code, an error message is logged on the report.
        //!
        ~ErrCodeReport();

    private:
        Report& _report;
        UString _message {};
        UString _object {};
    };
}
