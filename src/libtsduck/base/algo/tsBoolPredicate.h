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
//!  General-purpose boolean predicates.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Declaration of a boolean predicate with two arguments.
    //! @ingroup cpp
    //!
    typedef bool (*BoolPredicate)(bool, bool);

    //!
    //! Declaration of a boolean predicate with one argument.
    //! @ingroup cpp
    //!
    typedef bool (*MonoBoolPredicate)(bool);

    //!
    //! Declaration of a boolean predicate with a variable number of arguments.
    //! @ingroup cpp
    //!
    typedef bool (*MultiBoolPredicate)(std::initializer_list<bool>);

    //!
    //! MonoBoolPredicate identity.
    //! @ingroup cpp
    //! @param [in] arg A boolean argument.
    //! @return The same as @a arg.
    //!
    TSDUCKDLL inline bool Identity(bool arg) { return arg; }

    //!
    //! MonoBoolPredicate "not".
    //! @ingroup cpp
    //! @param [in] arg A boolean argument.
    //! @return Not @a arg.
    //!
    TSDUCKDLL inline bool Not(bool arg) { return !arg; }

    //!
    //! BoolPredicate "and".
    //! Note this predicate does not allow the traditional "&&" short circuit.
    //! The two arguments are always evaluated.
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return @a arg1 and @a arg2.
    //!
    TSDUCKDLL inline bool And(bool arg1, bool arg2) { return arg1 && arg2; }

    //!
    //! BoolPredicate "or".
    //! Note this predicate does not allow the traditional "||" short circuit.
    //! The two arguments are always evaluated.
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return @a arg1 or @a arg2.
    //!
    TSDUCKDLL inline bool Or(bool arg1, bool arg2) { return arg1 || arg2; }

    //!
    //! BoolPredicate "nand".
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return Not @a arg1 and @a arg2.
    //!
    TSDUCKDLL inline bool Nand(bool arg1, bool arg2) { return !(arg1 && arg2); }

    //!
    //! BoolPredicate "nor".
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return Not@a arg1 or @a arg2.
    //!
    TSDUCKDLL inline bool Nor(bool arg1, bool arg2) { return !(arg1 || arg2); }

    //!
    //! BoolPredicate "exclusive or".
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return @a arg1 xor @a arg2.
    //!
    TSDUCKDLL inline bool Xor(bool arg1, bool arg2) { return bool(int(arg1) ^ int(arg2)); }

    //!
    //! MultiBoolPredicate "and".
    //! @ingroup cpp
    //! @param [in] args A variable list of boolean arguments.
    //! @return True if all boolean values in @a args are true, false otherwise.
    //!
    TSDUCKDLL bool MultiAnd(std::initializer_list<bool> args);

    //!
    //! MultiBoolPredicate "or".
    //! @ingroup cpp
    //! @param [in] args A variable list of boolean arguments.
    //! @return True if any boolean values in @a args is true, false otherwise.
    //!
    TSDUCKDLL bool MultiOr(std::initializer_list<bool> args);

    //!
    //! MultiBoolPredicate "nand".
    //! @ingroup cpp
    //! @param [in] args A variable list of boolean arguments.
    //! @return False if all boolean values in @a args are true, true otherwise.
    //!
    TSDUCKDLL inline bool MultiNand(std::initializer_list<bool> args) { return !MultiAnd(args); }

    //!
    //! MultiBoolPredicate "nor".
    //! @ingroup cpp
    //! @param [in] args A variable list of boolean arguments.
    //! @return False if any boolean values in @a args is true, true otherwise.
    //!
    TSDUCKDLL inline bool MultiNor(std::initializer_list<bool> args) { return !MultiOr(args); }
}
