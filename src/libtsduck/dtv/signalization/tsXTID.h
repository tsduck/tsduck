
//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  "Extended Table Id", a synthetic value for identifying tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTID.h"

namespace ts {
    //!
    //! Extended MPEG table id.
    //! @ingroup mpeg
    //!
    //! For convenience, it is sometimes useful to identify tables using an
    //! "extended TID", a combination of TID and TIDext. On one PID, two tables
    //! with the same TID but with different TIDext are considered as distinct
    //! tables. By convention, the TIDext is always zero with short sections.
    //!
    class TSDUCKDLL XTID
    {
    private:
        uint32_t _xtid; // 7-bit: unused, 1-bit: long table, 8-bit: tid, 16-bit: tid-ext
    public:
        //!
        //! Constructor from a short table id.
        //! Short tables have no TIDext.
        //! @param [in] tid Table id.
        //!
        explicit XTID(TID tid = 0xFF) : _xtid((uint32_t(tid) & 0xFF) << 16) {}

        //!
        //! Constructor from a long table id and tid-ext.
        //! @param [in] tid Table id.
        //! @param [in] tid_ext Table id extension.
        //!
        XTID(TID tid, uint16_t tid_ext): _xtid(0x01000000 | ((uint32_t(tid) & 0xFF) << 16) | (uint32_t(tid_ext) & 0xFFFF)) {}

        //! @cond nodoxygen
        auto operator<=>(const XTID&) const = default;
        //! @endcond

        //!
        //! Check if the table is a long one.
        //! @return True if the table is a long one.
        //!
        bool isLongSection() const { return (_xtid & 0x01000000) != 0; }

        //!
        //! Check if the table is a short one.
        //! @return True if the table is a short one.
        //!
        bool isShortSection() const { return (_xtid & 0x01000000) == 0; }

        //!
        //! Get the table id.
        //! @return The table id.
        //!
        TID tid() const { return TID((_xtid >> 16) & 0xFF); }

        //!
        //! Get the table id extension.
        //! @return The table id extension.
        //!
        uint16_t tidExt() const { return uint16_t(_xtid & 0xFFFF); }

        //!
        //! Convert to a string object.
        //! Note: The XTID class does not implement StringifyInterface because we don't want to
        //! make it virtual and keep the instance size small, without vtable pointer.
        //! @return This object, converted as a string.
        //!
        UString toString() const;
    };
}
