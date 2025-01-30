//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    using BoolPredicate = bool (*)(bool, bool);

    //!
    //! Declaration of a boolean predicate with one argument.
    //! @ingroup cpp
    //!
    using MonoBoolPredicate = bool (*)(bool);

    //!
    //! Declaration of a boolean predicate with a variable number of arguments.
    //! @ingroup cpp
    //!
    using MultiBoolPredicate = bool (*)(std::initializer_list<bool>);

    //!
    //! MonoBoolPredicate identity.
    //! @ingroup cpp
    //! @param [in] arg A boolean argument.
    //! @return The same as @a arg.
    //!
    TSCOREDLL inline bool Identity(bool arg) { return arg; }

    //!
    //! MonoBoolPredicate "not".
    //! @ingroup cpp
    //! @param [in] arg A boolean argument.
    //! @return Not @a arg.
    //!
    TSCOREDLL inline bool Not(bool arg) { return !arg; }

    //!
    //! BoolPredicate "and".
    //! Note this predicate does not allow the traditional "&&" short circuit.
    //! The two arguments are always evaluated.
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return @a arg1 and @a arg2.
    //!
    TSCOREDLL inline bool And(bool arg1, bool arg2) { return arg1 && arg2; }

    //!
    //! BoolPredicate "or".
    //! Note this predicate does not allow the traditional "||" short circuit.
    //! The two arguments are always evaluated.
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return @a arg1 or @a arg2.
    //!
    TSCOREDLL inline bool Or(bool arg1, bool arg2) { return arg1 || arg2; }

    //!
    //! BoolPredicate "nand".
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return Not @a arg1 and @a arg2.
    //!
    TSCOREDLL inline bool Nand(bool arg1, bool arg2) { return !(arg1 && arg2); }

    //!
    //! BoolPredicate "nor".
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return Not@a arg1 or @a arg2.
    //!
    TSCOREDLL inline bool Nor(bool arg1, bool arg2) { return !(arg1 || arg2); }

    //!
    //! BoolPredicate "exclusive or".
    //! @ingroup cpp
    //! @param [in] arg1 A boolean argument.
    //! @param [in] arg2 A boolean argument.
    //! @return @a arg1 xor @a arg2.
    //!
    TSCOREDLL inline bool Xor(bool arg1, bool arg2) { return bool(int(arg1) ^ int(arg2)); }

    //!
    //! MultiBoolPredicate "and".
    //! @ingroup cpp
    //! @param [in] args A variable list of boolean arguments.
    //! @return True if all boolean values in @a args are true, false otherwise.
    //!
    TSCOREDLL bool MultiAnd(std::initializer_list<bool> args);

    //!
    //! MultiBoolPredicate "or".
    //! @ingroup cpp
    //! @param [in] args A variable list of boolean arguments.
    //! @return True if any boolean values in @a args is true, false otherwise.
    //!
    TSCOREDLL bool MultiOr(std::initializer_list<bool> args);

    //!
    //! MultiBoolPredicate "nand".
    //! @ingroup cpp
    //! @param [in] args A variable list of boolean arguments.
    //! @return False if all boolean values in @a args are true, true otherwise.
    //!
    TSCOREDLL inline bool MultiNand(std::initializer_list<bool> args) { return !MultiAnd(args); }

    //!
    //! MultiBoolPredicate "nor".
    //! @ingroup cpp
    //! @param [in] args A variable list of boolean arguments.
    //! @return False if any boolean values in @a args is true, true otherwise.
    //!
    TSCOREDLL inline bool MultiNor(std::initializer_list<bool> args) { return !MultiOr(args); }
}
