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
//
//  "Extended Table Id", a synthetic value for identifying tables.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"

namespace ts {

    // For convenience, it is sometimes useful to identify tables using an
    // "extended TID", a combination of TID and TIDext. On one PID, two tables
    // with the same TID but with different TIDext are considered as distinct
    // tables. By convention, the TIDext is always zero with short sections.

    class TSDUCKDLL ETID
    {
    private:
        uint32_t _etid; // 7-bit: unused, 1-bit: long table, 8-bit: tid, 16-bit: tid-ext
    public:
        // Constructor from a short table id
        ETID (TID tid = 0xFF): _etid ((uint32_t (tid) & 0xFF) << 16) {}

        // Constructor from a long table id and tid-ext
        ETID (TID tid, uint16_t tid_ext): _etid (0x01000000 | ((uint32_t (tid) & 0xFF) << 16) | (uint32_t (tid_ext) & 0xFFFF)) {}

        // Copy constructor
        ETID (const ETID& e): _etid (e._etid) {}

        // Extract values
        bool isLongSection() const {return (_etid & 0x01000000) != 0;}
        bool isShortSection() const {return (_etid & 0x01000000) == 0;}
        TID tid() const {return TID ((_etid >> 16) & 0xFF);}
        uint16_t tidExt() const {return uint16_t (_etid & 0xFFFF);}

        // Comparisons
        bool operator == (const ETID& e) const {return _etid == e._etid;}
        bool operator != (const ETID& e) const {return _etid != e._etid;}
        bool operator <  (const ETID& e) const {return _etid <  e._etid;}
        bool operator <= (const ETID& e) const {return _etid <= e._etid;}
        bool operator >  (const ETID& e) const {return _etid >  e._etid;}
        bool operator >= (const ETID& e) const {return _etid >= e._etid;}

        // Assignment
        ETID& operator = (const ETID& e) {_etid = e._etid; return *this;}
    };
}
