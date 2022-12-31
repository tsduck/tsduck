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
        BinaryTablePtrVector _tables;

        // Inherited from TableHandlerInterface
        // This hook is invoked when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Make these methods inaccessible
        void setHandler(TableHandlerInterface*) = delete;
        void setHandler(SectionHandlerInterface*) = delete;
    };
}
