//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A SectionDemux which extracts MPEG tables without external handler.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSectionDemux.h"

namespace ts {
    //!
    //! A SectionDemux which extracts MPEG tables without external handler.
    //! All demuxed tables are accumulated inside the demux, until clear or reset.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL StandaloneTableDemux: public SectionDemux, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(StandaloneTableDemux);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] pid_filter The set of PID's to demux.
        //!
        StandaloneTableDemux(DuckContext& duck, const PIDSet& pid_filter = NoPID());

        //!
        //! Get the number of demuxed tables so far.
        //! @return The number of demuxed tables so far.
        //!
        size_t tableCount() const { return _tables.size(); }

        //!
        //! Get shared pointers to all demuxed tables.
        //! @return A constant reference to a vector of safe pointers to the received tables.
        //!
        const BinaryTablePtrVector& tables() const { return _tables; }

        //!
        //! Clear the received tables without resetting the demux.
        //! This can be useful to forget received tables (and free memory) without resetting the reception.
        //!
        void clearTables() { _tables.clear(); }

        // Inherited methods.
        virtual void reset() override;
        virtual void resetPID(PID pid) override;

    private:
        // Private members
        BinaryTablePtrVector _tables {};

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Make these methods inaccessible
        void setHandler(TableHandlerInterface*) = delete;
        void setHandler(SectionHandlerInterface*) = delete;
    };
}
