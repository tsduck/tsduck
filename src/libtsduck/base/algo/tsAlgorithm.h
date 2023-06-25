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
//!  Miscellaneous C++ algorithms supplementing the standard \<algorithm\>.
//!
//!  This file declares several template functions implementing general-purpose
//!  algorithms. This header may be considered as an extension of the standard
//!  header \<algorithm\>, implementing other algorithms.
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
    bool EnumerateCombinations(const std::set<T>& values,
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

    //!
    //! Remove duplicated elements in a container.
    //!
    //! When duplicates are found, this first occurence is kept, aothers are removed.
    //!
    //! The predefined function @c std::unique() removes duplicated elements when they
    //! are consecutive only. This function removes all duplicates.
    //!
    //! @tparam CONTAINER A container class as defined by the C++ Standard Template Library (STL).
    //! @param [in,out] container A container into which duplicates are removed.
    //!
    template <class CONTAINER>
    void RemoveDuplicates(CONTAINER& container);

    //!
    //! Get the size of the smallest object in a container of objects having a @c size() method.
    //!
    //! @tparam CONTAINER A container class as defined by the C++ Standard Template Library (STL).
    //! @param [in] container A container of objects.
    //! @return The size of the smallest object in @a container.
    //!
    template <class CONTAINER>
    size_t SmallestSize(const CONTAINER& container);

    //!
    //! Get the size of the largest object in a container of objects having a @c size() method.
    //!
    //! @tparam CONTAINER A container class as defined by the C++ Standard Template Library (STL).
    //! @param [in] container A container of objects.
    //! @return The size of the largest object in @a container.
    //!
    template <class CONTAINER>
    size_t LargestSize(const CONTAINER& container);

    //!
    //! Check if a value is present in a vector container.
    //!
    //! @tparam VALUE The type of element in the container.
    //! @param [in] container A container of objects.
    //! @param [in] value The value to search in the container.
    //! @return True if @a value is present in @a container.
    //!
    template <class VALUE>
    bool Contains(const std::vector<VALUE>& container, const VALUE& value)
    {
        return std::find(container.begin(), container.end(),value) != container.end();
    }

    //!
    //! Check if a value is present in a container.
    //!
    //! @tparam CONTAINER A container class with @a find() and @a end() methods.
    //! @tparam VALUE The type of element in the container.
    //! @param [in] container A container of objects.
    //! @param [in] value The value to search in the container.
    //! @return True if @a value is present in @a container.
    //!
    template <class CONTAINER, class VALUE>
    bool Contains(const CONTAINER& container, const VALUE& value)
    {
        return container.find(value) != container.end();
    }

    //!
    //! Get the list of all keys in a map.
    //!
    //! @tparam MAP A map container class as defined by the C++ Standard Template Library (STL).
    //! @param [in] container A container of objects.
    //! @return The list of all keys in @a container.
    //!
    template <class MAP>
    std::list<typename MAP::key_type> MapKeysList(const MAP& container);

    //!
    //! Get the set of all keys in a map.
    //!
    //! @tparam MAP A map container class as defined by the C++ Standard Template Library (STL).
    //! @param [in] container A container of objects.
    //! @return The set of all keys in @a container.
    //!
    template <class MAP>
    std::set<typename MAP::key_type> MapKeysSet(const MAP& container);

    //!
    //! Get the list of all values in a map.
    //!
    //! @tparam MAP A map container class as defined by the C++ Standard Template Library (STL).
    //! @param [in] container A container of objects.
    //! @return The list of all values in @a container.
    //!
    template <class MAP>
    std::list<typename MAP::mapped_type> MapValuesList(const MAP& container);

    //!
    //! Build a vector of integers containing all values in a range.
    //!
    //! @tparam INT An integral type.
    //! @param [in] first First value of the range.
    //! @param [in] last Last value of the range.
    //! @return A vector of @a INT containing all values from @a min to @a max, inclusive.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    std::vector<INT> Range(INT first, INT last);

    //!
    //! I/O manipulator for subclasses of <code>std::basic_ostream</code>.
    //!
    //! The standard C++ library contains support for I/O manipulators on <code>std::basic_ios</code>,
    //! <code>std::basic_istream</code> and <code>std::basic_ostream</code> but not for their subclasses.
    //! This template function is a support routine for I/O manipulators on subclasses.
    //!
    //! Sample usage:
    //! @code
    //! namespace ts {
    //!     // The class:
    //!     class TextFormatter : public std::basic_ostream<char>
    //!     {
    //!     public:
    //!         TextFormatter& margin();
    //!         ....
    //!     };
    //!
    //!     // The I/O manipulator:
    //!     std::ostream& margin(std::ostream& os)
    //!     {
    //!         return IOManipulator(os, &TextFormatter::margin);
    //!     }
    //! }
    //!
    //! // Usage:
    //! ts::TextFormatter out;
    //! out << ts::margin << "<foo>" << std::endl;
    //! @endcode
    //!
    //! @tparam OSTREAM A subclass of <code>std::basic_ostream</code>.
    //! @param [in,out] strm The stream of class @a OSTREAM to manipulate.
    //! @param [in] func A pointer to member function in @a OSTREAM. This method
    //! shall take no parameter and return a reference to the @a OSTREAM object.
    //! @return A reference to the @a strm object.
    //!
    template <class OSTREAM, class TRAITS = std::char_traits<typename OSTREAM::char_type>>
    std::basic_ostream<typename OSTREAM::char_type, TRAITS>& IOManipulator(std::basic_ostream<typename OSTREAM::char_type, TRAITS>& strm, OSTREAM& (OSTREAM::*func)())
    {
        OSTREAM* sub = dynamic_cast<OSTREAM*>(&strm);
        return sub == nullptr ? strm : (sub->*func)();
    }

    //!
    //! I/O manipulator with argument for subclasses of <code>std::basic_ostream</code>.
    //!
    //! This class has a similar role as IOManipulator() for functions using one argument.
    //!
    //! Sample usage:
    //! @code
    //! namespace ts {
    //!     // The class:
    //!     class TextFormatter : public std::basic_ostream<char>
    //!     {
    //!     public:
    //!         TextFormatter& setMarginSize(size_t margin);
    //!         ....
    //!     };
    //!
    //!     // The I/O manipulator:
    //!     IOManipulatorProxy<TextFormatter,size_t> margin(size_t size)
    //!     {
    //!         return IOManipulatorProxy<TextFormatter,size_t>(&TextFormatter::setMarginSize, size);
    //!     }
    //! }
    //!
    //! // Usage:
    //! ts::TextFormatter out;
    //! out << ts::margin(2) << "<foo>" << std::endl;
    //! @endcode
    //!
    //! @tparam OSTREAM A subclass of <code>std::basic_ostream</code>.
    //! @tparam PARAM The type of the parameter for the manipulator.
    //!
    template <class OSTREAM, class PARAM, class TRAITS = std::char_traits<typename OSTREAM::char_type>>
    class IOManipulatorProxy
    {
    private:
        OSTREAM& (OSTREAM::*_func)(PARAM);
        PARAM _param;
    public:
        //!
        //! Constructor.
        //! @param [in] func A pointer to member function in @a OSTREAM. This method shall take
        //! one parameter or type @a PARAM and return a reference to the @a OSTREAM object.
        //! @param [in] param The parameter value to pass to the I/O manipulator.
        //!
        IOManipulatorProxy(OSTREAM& (OSTREAM::*func)(PARAM), PARAM param) : _func(func), _param(param) {}

        //!
        //! Invoke the I/O manipulator, typically invoked by the operator "<<".
        //! @param [in,out] strm The stream of class @a OSTREAM to manipulate.
        //! @return A reference to the @a strm object.
        //!
        std::basic_ostream<typename OSTREAM::char_type, TRAITS>& manipulator(std::basic_ostream<typename OSTREAM::char_type, TRAITS>& strm) const
        {
            OSTREAM* sub = dynamic_cast<OSTREAM*>(&strm);
            return sub == 0 ? strm : (sub->*_func)(_param);
        }
    };
}

//!
//! An overload of operator "<<" on <code>std::basic_ostream</code> for ts::IOManipulatorProxy.
//! @tparam OSTREAM A subclass of <code>std::basic_ostream</code>.
//! @tparam PARAM The type of the parameter for the manipulator.
//! @param [in,out] strm The stream of class @a OSTREAM to manipulate.
//! @param [in] proxy The ts::IOManipulatorProxy object.
//! @return A reference to the @a strm object.
//! @see ts::IOManipulatorProxy
//!
template <class OSTREAM, class PARAM, class TRAITS = std::char_traits<typename OSTREAM::char_type>>
std::basic_ostream<typename OSTREAM::char_type, TRAITS>& operator<<(std::basic_ostream<typename OSTREAM::char_type, TRAITS>& strm, const ts::IOManipulatorProxy<OSTREAM, PARAM, TRAITS>& proxy)
{
    return proxy.manipulator(strm);
}

#include "tsAlgorithmTemplate.h"
