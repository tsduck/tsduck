//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
    //! @ingroup mpeg
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
        StandaloneTableDemux(DuckContext& duck, const PIDSet& pid_filter = NoPID);

        //!
        //! Get the number of demuxed tables so far.
        //! @return The number of demuxed tables so far.
        //!
        size_t tableCount() const { return _tables.size(); }

        //!
        //! Get a pointer to a demuxed table.
        //! @param [in] index Index of a demuxed table, from 0 to tableCount()-1.
        //! @return A safe pointer to the corresponding table.
        //!
        const BinaryTablePtr& tableAt(size_t index) const;

        //! @copydoc SectionDemux::reset()
        virtual void reset() override;

        //! @copydoc SectionDemux::resetPID()
        virtual void resetPID(PID pid) override;

    private:
        // Private members
        BinaryTablePtrVector _tables {};

        // Inherited from TableHandlerInterface
        // This hook is invoked when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Make these methods inaccessible
        void setHandler(TableHandlerInterface*) = delete;
        void setHandler(SectionHandlerInterface*) = delete;
    };
}
