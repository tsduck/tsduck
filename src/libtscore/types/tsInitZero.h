//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Encapsulate a plain old C data structure with zero initialization.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMemory.h"

namespace ts {
    //!
    //! Encapsulate a plain old C data structure with automatic zero initialization.
    //! @ingroup libtscore cpp
    //! @tparam STRUCT The plain old C data structure type to encapsulate.
    //!
    template <typename STRUCT>
    struct InitZero {
        //!
        //! Encapsulated plain old C data structure.
        //! 
        STRUCT data;

        //!
        //! Size in bytes of the encapsulated plain old C data structure.
        //!
        static constexpr size_t SIZE = sizeof(data);

        //!
        //! Default constructor, zeroe the C structure.
        //!
        InitZero() :
            data()
        {
            TS_ZERO(data);
        }

        //!
        //! Comparator for containers, no real semantic.
        //! @param other Other data structure to compare.
        //! @return True if the binary content of this data structure is locally less than the content of @a other.
        //! 
        bool operator<(const InitZero<STRUCT>& other) const
        {
            return MemCompare(&data, &other.data, SIZE) < 0;
        }
    };
}
