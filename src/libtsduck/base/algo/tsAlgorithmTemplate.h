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

#pragma once
#include "tsPlatform.h"


//----------------------------------------------------------------------------
// Internal recursive implementation of EnumerateCombinations.
//----------------------------------------------------------------------------

namespace ts {
    //
    // This function use a 'current' set as starting point. It builds all combinations
    // all elements in the range ['begin'..'end') and adds these combinations to 'current'.
    // It does this recursively to add more elements into 'current'. When 'current' has
    // reached the requested size (ie. when 'level' is zero), the user's predicate is
    // invoked.
    //
    template <typename T, class UnaryPredicate>
    bool _EnumerateCombinationsImpl(typename std::set<T>::const_iterator begin,
                                    typename std::set<T>::const_iterator end,
                                    typename std::set<T>& current,
                                    UnaryPredicate predicate,
                                    size_t level)
    {
        if (level == 0) {
            // We have built a complete combination, invoke user's predicate.
            return predicate (current);
        }
        else  {
            // Building a combination in current
            bool more = true;
            auto it = begin;
            while (more && it != end) {
                const T& x (*it);
                current.insert (x);
                more = _EnumerateCombinationsImpl(++it, end, current, predicate, level - 1);
                current.erase (x);
            }
            return more;
        }
    }
}


//----------------------------------------------------------------------------
// Enumerate all 'k'-elements combinations of a 'n'-elements set.
//----------------------------------------------------------------------------

template <typename T, class UnaryPredicate>
bool ts::EnumerateCombinations(const std::set<T>& values,
                               const std::set<T>& fixed,
                               size_t size,
                               UnaryPredicate predicate)
{
    // There is no possible combination in the following cases:
    // - The requested combination size is larger that the set of all values.
    // - The set of fixed values is larger than the requested combination size.
    // - The set of fixed values is not included into the set of all values.
    // In any of these cases, there is no need to search anything.
    if (size > values.size() || fixed.size() > size || !std::includes(values.begin(), values.end(), fixed.begin(), fixed.end())) {
        // Return true since the user's predicate did not force a premature ending.
        return true;
    }

    // The set which is used to build the various combinations is 'current'.
    // The user's predicate will be invoked using this set.
    // Its initial content (and constant subset) is made of the set of fixed values.
    std::set<T> current(fixed);

    // Invoke the recursive combination function.
    if (current.empty()) {
        // No fixed values, the combinations are build directly on values.
        return _EnumerateCombinationsImpl(values.begin(), values.end(), current, predicate, size);
    }
    else {
        // There are some fixed values. The combinations are build on the subset
        // of values which excludes all fixed values.
        std::set<T> sub;
        std::set_difference(values.begin(), values.end(), current.begin(), current.end(), std::inserter(sub, sub.begin()));
        return _EnumerateCombinationsImpl(sub.begin(), sub.end(), current, predicate, size - current.size());
    }
}


//----------------------------------------------------------------------------
// Append an element into a container if not already present in the container.
// Return true is appended, false if already present.
//----------------------------------------------------------------------------

template <typename ELEMENT, class CONTAINER>
bool ts::AppendUnique(CONTAINER& container, const ELEMENT& e)
{
    for (const auto& it : container) {
        if (e == it) {
            return false;  // already present
        }
    }
    container.push_back(e);
    return true; // new object inserted
}


//----------------------------------------------------------------------------
// Remove duplicated elements in a container.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::RemoveDuplicates(CONTAINER& container)
{
    for (auto it1 = container.begin(); it1 != container.end(); ++it1) {
        auto it2 = it1;
        for (++it2; it2 != container.end(); ) {
            if (*it2 == *it1) {
                it2 = container.erase(it2);
            }
            else {
                ++it2;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Get the size of the smallest/largest object in a container.
//----------------------------------------------------------------------------

template <class CONTAINER>
size_t ts::SmallestSize(const CONTAINER& container)
{
    if (container.empty()) {
        return 0;
    }
    else {
        size_t smallest = std::numeric_limits<size_t>::max();
        for (auto it = container.begin(); smallest > 0 && it != container.end(); ++it) {
            smallest = std::min(smallest, it->size());
        }
        return smallest;
    }
}

template <class CONTAINER>
size_t ts::LargestSize(const CONTAINER& container)
{
    size_t largest = 0;
    for (auto& it : container) {
        largest = std::max(largest, it.size());
    }
    return largest;
}


//----------------------------------------------------------------------------
// Get the list of all keys / values in a map.
//----------------------------------------------------------------------------

template <class MAP>
std::list<typename MAP::key_type> ts::MapKeysList(const MAP& container)
{
    std::list<typename MAP::key_type> keys;
    for (const auto& it : container) {
        keys.push_back(it.first);
    }
    return keys;
}

template <class MAP>
std::set<typename MAP::key_type> ts::MapKeysSet(const MAP& container)
{
    std::set<typename MAP::key_type> keys;
    for (const auto& it : container) {
        keys.insert(it.first);
    }
    return keys;
}

template <class MAP>
std::list<typename MAP::mapped_type> ts::MapValuesList(const MAP& container)
{
    std::list<typename MAP::mapped_type> values;
    for (const auto& it : container) {
        values.push_back(it.second);
    }
    return values;
}

//----------------------------------------------------------------------------
// Build a vector of integers containing all values in a range.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
std::vector<INT> ts::Range(INT first, INT last)
{
    std::vector<INT> vec;
    while (first <= last) {
        vec.push_back(first);
        if (first == std::numeric_limits<INT>::max()) {
            break;
        }
        else {
            first++;
        }
    }
    return vec;
}
