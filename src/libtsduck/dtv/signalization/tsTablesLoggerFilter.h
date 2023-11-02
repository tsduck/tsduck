//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TablesLoggerFilter() = default;

        // Implementation of TablesLoggerFilterInterface.
        virtual void defineFilterOptions(Args& args) const override;
        virtual bool loadFilterOptions(DuckContext& duck, Args& args, PIDSet& initial_pids) override;
        virtual bool reset() override;
        virtual bool filterSection(DuckContext& duck, const Section& section, uint16_t cas, PIDSet& more_pids) override;

    private:
        // Command line options:
        bool               _diversified = false;    // Payload must be diversified.
        bool               _negate_tid = false;     // Negate tid filter (exclude selected tids).
        bool               _negate_tidext = false;  // Negate tidext filter (exclude selected tidexts).
        bool               _negate_secnum = false;  // Negate section number filter (exclude selected numbers).
        bool               _psi_si = false;         // Add PSI/SI PID's.
        PIDSet             _pids {};                // PID values to filter.
        std::set<uint8_t>  _tids {};                // TID values to filter.
        std::set<uint16_t> _tidexts {};             // TID-ext values to filter.
        std::set<uint8_t>  _secnums {};             // Section numbers to filter.
        ByteBlock          _content_filter {};      // Section content to filter.
        ByteBlock          _content_mask {};        // Meaningful bits in content filter.

        // Working data:
        PIDSet             _current_pids {};        // Current PID's to filter.
        BinaryTable        _pat {};                 // Last PAT.
    };
}
