//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup cpp
//!  Miscellaneous C++ general-purpose functors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Assign ("=") functor.
    //! @tparam T Data type to assign.
    //!
    template <typename T>
    struct Assign {
        //!
        //! Assign ("=") operator.
        //! @param [out] variable The variable to assign.
        //! @param [in] value The value to assign.
        //! @return A reference to @a variable.
        //!
        T& operator()(T& variable, T value) { return variable = value; }
    };

    //!
    //! Assign with binary and ("&=") functor.
    //! @tparam T Data type to assign.
    //!
    template <typename T>
    struct AssignAnd {
        //!
        //! Assign with binary and ("&=") operator.
        //! @param [in,out] variable The variable to assign.
        //! @param [in] value The value to assign.
        //! @return A reference to @a variable.
        //!
        T& operator()(T& variable, T value) { return variable &= value; }
    };

    //!
    //! Assign with binary or ("|=") functor.
    //! @tparam T Data type to assign.
    //!
    template <typename T>
    struct AssignOr {
        //!
        //! Assign with binary or ("|=") operator.
        //! @param [in,out] variable The variable to assign.
        //! @param [in] value The value to assign.
        //! @return A reference to @a variable.
        //!
        T& operator()(T& variable, T value) { return variable |= value; }
    };

    //!
    //! Assign with binary exclusive or ("^=") functor.
    //! @tparam T Data type to assign.
    //!
    template <typename T>
    struct AssignXor {
        //!
        //! Assign with binary exclusive or ("^=") operator.
        //! @param [in,out] variable The variable to assign.
        //! @param [in] value The value to assign.
        //! @return A reference to @a variable.
        //!
        T& operator()(T& variable, T value) { return variable ^= value; }
    };
}
