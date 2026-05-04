//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  An identifier for sources of events in a Reactor (I/O, timers, etc).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! An identifier for sources of events in a Reactor (I/O, timers, etc).
    //! This is a small object which can be passed by value at zero cost.
    //!
    class TSCOREDLL EventId
    {
    public:
        //!
        //! Default constructor.
        //!
        EventId() : _ptr(nullptr) {}

        //!
        //! Check if the source id is valid.
        //! @return True if the source id is valid.
        //!
        bool isValid() const { return _ptr != nullptr; }

        //!
        //! Invalidate the content of this object.
        //!
        void invalidate() { _ptr = nullptr; }

        //!
        //! Equality operator.
        //! @param [in] other Other instance to compare.
        //! @return True if this object is equal to @a other.
        //!
        bool operator==(const EventId& other) const { return _ptr == other._ptr; }

        //!
        //! Less than operator (for use in hash tables).
        //! @param [in] other Other instance to compare.
        //! @return True if this object is less to @a other.
        //!
        bool operator<(const EventId& other) const { return _ptr < other._ptr; }

        //!
        //! Get a string representation, for debug purpose.
        //! @return A string representation of this object (an integer value).
        //!
        UString toString() const { return UString::Hexa(reinterpret_cast<uintptr_t>(_ptr)); }

    private:
        void* _ptr;

        // The internal value is directly managed by the implementation of the reactor.
        friend class Reactor;
        EventId(void* res) : _ptr(res) {}
    };
}
