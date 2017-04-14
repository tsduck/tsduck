//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//! @file tsAlgorithm.h
//!
//! Miscellaneous C++ algorithms supplementing the standard \<algorithm\>.
//!
//! This file declares several template functions implementing general-purpose
//! algorithms. This header may be considered as an extension of the standard
//! header \<algorithm\>, implementing other algorithms.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    //!
    //! Enumerate all 'k'-elements combinations of a 'n'-elements set.
    //!
    //! This function enumerates all possible sets of 'k' elements within
    //! a set of 'n' elements. For each combination, a user-supplied predicate
    //! is invoked. The signature of the predicate function should be equivalent
    //! to the following:
    //!
    //! @code
    //! bool p (const std::set<T>& combination);
    //! @endcode
    //!
    //! The signature does not need to have @c const, but the function must not
    //! modify the set which is passed to it. The function returns true when the
    //! search for more combinations shall continue and false when the search
    //! shall prematurely stops.
    //!
    //! @tparam T The type of elements in the sets.
    //!
    //! @tparam UnaryPredicate The type of unary predicate returning true or false
    //! from one set of elements of type @a T.
    //!
    //! @param values The set of all values from which the combinations are built.
    //! The size of @a values is the 'n' in the sentence <i>"enumerate all 'k'-elements
    //! combinations of a 'n'-elements set"</i>.
    //!
    //! @param fixed The set of fixed values which must be present in all combinations.
    //! To enumerate all possible combinations in @a values, use an empty set for @a fixed.
    //! If @a fixed is not empty, the @a predicate will be invoked only with combinations
    //! of @a values which contains all elements in @a fixed.
    //!
    //! @param size The size (number of elements) of the combinations to enumerate.
    //! This is the 'k' in the sentence <i>"enumerate all 'k'-elements combinations of
    //! a 'n'-elements set"</i>.
    //!
    //! @param predicate The unary predicate to invoke for each possible combination
    //! of @a size elements in @a values. If @a predicate returns true, the search for
    //! more combination continues. When @a predicate returns false, the search is
    //! interrupted immediately and no more combination is proposed.
    //! Example: When the application searches for a combination which matches specific
    //! properties, the application's predicate returns false when a matching combination
    //! is found to avoid looking for additional combinations.
    //!
    //! @return True if all combinations were searched and false if the search was
    //! interrupted because @a predicate returned false at some point.
    //!
    template <typename T, class UnaryPredicate>
    bool EnumerateCombinations (const std::set<T>& values,
                                const std::set<T>& fixed,
                                size_t size,
                                UnaryPredicate predicate);

    //!
    //! Append an element into a container if not already present in the container.
    //!
    //! If the element @a e is not already present in the @a container, @a e
    //! is appended at the end of the @a container. Otherwise, the @a container
    //! is left unmodified.
    //!
    //! @tparam ELEMENT Any type.
    //! @tparam CONTAINER A container class of @c ELEMENT as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [in,out] container A container of @c ELEMENT.
    //! @param [in] e An element to conditionally append to @a container.
    //! @return True if @a e was appended in the container, false if it was already present.
    //!
    template <typename ELEMENT, class CONTAINER>
    bool AppendUnique(CONTAINER& container, const ELEMENT& e);
}

#include "tsAlgorithmTemplate.h"
