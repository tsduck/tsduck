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
//!  The default section filter for TablesLogger.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTablesLoggerFilterInterface.h"
#include "tsBinaryTable.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! The default section filter for TablesLogger.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TablesLoggerFilter: public TablesLoggerFilterInterface
    {
        TS_NOCOPY(TablesLoggerFilter);
    public:
        //!
        //! Default constructor.
        //!
        TablesLoggerFilter();

        // Implementation of TablesLoggerFilterInterface.
        virtual void defineFilterOptions(Args& args) const override;
        virtual bool loadFilterOptions(DuckContext& duck, Args& args, PIDSet& initial_pids) override;
        virtual bool filterSection(DuckContext& duck, const Section& section, uint16_t cas, PIDSet& more_pids) override;

    private:
        bool               _diversified;    // Payload must be diversified.
        bool               _negate_tid;     // Negate tid filter (exclude selected tids).
        bool               _negate_tidext;  // Negate tidext filter (exclude selected tidexts).
        bool               _negate_secnum;  // Negate section number filter (exclude selected numbers).
        bool               _psi_si;         // Add PSI/SI PID's.
        PIDSet             _pids;           // PID values to filter.
        std::set<uint8_t>  _tids;           // TID values to filter.
        std::set<uint16_t> _tidexts;        // TID-ext values to filter.
        std::set<uint8_t>  _secnums;        // Section numbers to filter.
        ByteBlock          _content_filter; // Section content to filter.
        ByteBlock          _content_mask;   // Meaningful bits in content filter.
        BinaryTable        _pat;            // Last PAT.
    };
}
