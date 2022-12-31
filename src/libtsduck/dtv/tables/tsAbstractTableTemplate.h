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
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors - Constructors.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::EntryWithDescriptorsMap(const AbstractTable* table, bool auto_ordering) :
    SuperClass(),
    _table(table),
    _auto_ordering(auto_ordering)
{
}

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::EntryWithDescriptorsMap(const AbstractTable* table, const EntryWithDescriptorsMap& other) :
    SuperClass(),
    _table(table),
    _auto_ordering(other._auto_ordering)
{
    // Copy each entry one by one to ensure that the copied entries actually point to the constructed table.
    for (auto& it : other) {
        (*this)[it.first] = it.second;
    }
}


//----------------------------------------------------------------------------
// Template list of subclasses of EntryWithDescriptors - Assignment.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>& ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::operator=(const EntryWithDescriptorsMap& other)
{
    if (&other != this) {
        // Use same auto ordering as copied map.
        _auto_ordering = other._auto_ordering;
        // Clear and copy each entry one by one to ensure that the copied entries actually point to the target table.
        this->clear();
        for (auto& it : other) {
            (*this)[it.first] = it.second;
        }
    }
    return *this;
}

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ts::AbstractTable::EntryWithDescriptorsMap<KEY, ENTRY, N>& ts::AbstractTable::EntryWithDescriptorsMap<KEY, ENTRY, N>::operator=(EntryWithDescriptorsMap&& other)
{
    if (&other != this) {
        // Use same auto ordering as copied map.
        _auto_ordering = other._auto_ordering;
        // Clear and move each entry one by one to ensure that the copied entries actually point to the target table.
        this->clear();
        for (auto& it : other) {
            (*this)[it.first] = std::move(it.second);
        }
        // The other instance is still a valid map with valid entries.
        // But all entries have unspecified values.
        // Just clear the other it to get a deterministic state.
        other.clear();
    }
    return *this;
}

//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors - Swap.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
void ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::swap(EntryWithDescriptorsMap& other)
{
    if (&other != this) {
        // Unefficient but functionally correct.
        const EntryWithDescriptorsMap tmp(nullptr, other);
        other = *this;
        *this = tmp;
    }
}

//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors - Subscripts.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ENTRY& ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::operator[](const KEY& key)
{
    // The emplace operation ensures that the object is constructed with the supplied arguments (and not copied).
    // This form is more complex but the simplest form of emplace (without the piecewise_construct and tuples)
    // does not compile with LLVM. Not sure if this is a problem with LLVM or if the other compilers are too
    // permissive.
    ENTRY& entry(this->emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(_table)).first->second);

    // When not already specified otherwise, keep the order of entry creation.
    if (_auto_ordering && entry.order_hint == NPOS) {
        entry.order_hint = this->nextOrder();
    }
    return entry;
}

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
const ENTRY& ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::operator[](const KEY& key) const
{
    // Here, we must not create any element (the instance is read-only).
    const auto it = this->find(key);
    if (it == this->end()) {
        // Same exception as std::map::at().
        throw std::out_of_range("unknown key in ts::AbstractTable::EntryWithDescriptorsMap");
    }
    return it->second;
}


//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors.
// Get the insertion order of entrie in the table.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
void ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::getOrder(std::vector<KEY>& order) const
{
    // Build a multimap of keys, indexed by order_hint.
    std::multimap<size_t, KEY> kmap;
    for (const auto& entry : *this) {
        kmap.insert(std::make_pair(entry.second.order_hint, entry.first));
    }

    // Build a vector of keys from this order.
    order.clear();
    order.reserve(kmap.size());
    for (const auto& it : kmap) {
        order.push_back(it.second);
    }
}


//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors.
// Set the insertion order of entrie in the table.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
void ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::setOrder(const std::vector<KEY>& order)
{
    // First pass: get initial ordering.
    std::vector<KEY> input;
    this->getOrder(input);

    // Second pass: Assign ordering hints to explicitly sorted keys.
    size_t count = 0;
    for (const KEY& key : order) {
        const auto it = this->find(key);
        if (it != this->end()) {
            it->second.order_hint = count++;
        }
    }

    // Third pass: reassign increasing ordering numbers for unspecified keys, same order as previously.
    for (const KEY& key : input) {
        if (!Contains(order, key)) {
            const auto it = this->find(key);
            if (it != this->end()) {
                it->second.order_hint = count++;
            }
        }
    }
}

//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors.
// Get the next ordering hint to be used in an entry.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
size_t ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::nextOrder() const
{
    size_t next = 0;
    for (const auto& entry : *this) {
        if (entry.second.order_hint != NPOS) {
            next = std::max(next, entry.second.order_hint + 1);
        }
    }
    return next;
}
