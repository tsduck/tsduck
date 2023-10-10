//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  "Extended Table Id", a synthetic value for identifying tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPSI.h"

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
    class TSDUCKDLL ETID
    {
    private:
        uint32_t _etid; // 7-bit: unused, 1-bit: long table, 8-bit: tid, 16-bit: tid-ext
    public:
        //!
        //! Constructor from a short table id.
        //! Short tables have no TIDext.
        //! @param [in] tid Table id.
        //!
        explicit ETID(TID tid = 0xFF) : _etid((uint32_t(tid) & 0xFF) << 16) {}

        //!
        //! Constructor from a long table id and tid-ext.
        //! @param [in] tid Table id.
        //! @param [in] tid_ext Table id extension.
        //!
        ETID(TID tid, uint16_t tid_ext): _etid(0x01000000 | ((uint32_t(tid) & 0xFF) << 16) | (uint32_t(tid_ext) & 0xFFFF)) {}

        //!
        //! Check if the table is a long one.
        //! @return True if the table is a long one.
        //!
        bool isLongSection() const { return (_etid & 0x01000000) != 0; }

        //!
        //! Check if the table is a short one.
        //! @return True if the table is a short one.
        //!
        bool isShortSection() const { return (_etid & 0x01000000) == 0; }

        //!
        //! Get the table id.
        //! @return The table id.
        //!
        TID tid() const { return TID((_etid >> 16) & 0xFF); }

        //!
        //! Get the table id extension.
        //! @return The table id extension.
        //!
        uint16_t tidExt() const { return uint16_t(_etid & 0xFFFF); }

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object == @a e.
        //!
        bool operator==(const ETID& e) const { return _etid == e._etid; }
        TS_UNEQUAL_OPERATOR(ETID)

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object < @a e.
        //!
        bool operator<(const ETID& e) const { return _etid <  e._etid; }

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object <= @a e.
        //!
        bool operator<=(const ETID& e) const { return _etid <= e._etid; }

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object > @a e.
        //!
        bool operator>(const ETID& e) const { return _etid >  e._etid; }

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object >= @a e.
        //!
        bool operator>=(const ETID& e) const { return _etid >= e._etid; }
    };
}
