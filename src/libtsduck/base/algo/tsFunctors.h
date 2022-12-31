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
