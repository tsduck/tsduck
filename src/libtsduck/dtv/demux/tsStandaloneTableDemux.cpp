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

#include "tsStandaloneTableDemux.h"
#include "tsBinaryTable.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::StandaloneTableDemux::StandaloneTableDemux(DuckContext& duck, const PIDSet& pid_filter) :
    SectionDemux(duck, this, nullptr, pid_filter),
    _tables()
{
}


//----------------------------------------------------------------------------
// Get a pointer to a demuxed table.
//----------------------------------------------------------------------------

const ts::BinaryTablePtr& ts::StandaloneTableDemux::tableAt(size_t index) const
{
    assert(index < _tables.size());
    return _tables[index];
}


//----------------------------------------------------------------------------
// Reset the analysis context (partially built sections and tables).
// Inherited from SectionDemux
//----------------------------------------------------------------------------

void ts::StandaloneTableDemux::reset()
{
    // Reset the demux
    SectionDemux::reset();

    // Reset the demuxed tables
    _tables.clear();
}


//----------------------------------------------------------------------------
// Reset the analysis context for one single PID.
// Inherited from SectionDemux
//----------------------------------------------------------------------------

void ts::StandaloneTableDemux::resetPID(PID pid)
{
    // Reset the demux for the PID
    SectionDemux::resetPID(pid);

    // Removed demuxed tables for this PID
    size_t index = 0;
    for (size_t i = 0; i < _tables.size(); ++i) {
        if (_tables[i]->sourcePID() != pid) {
            _tables[index++] = _tables[i];
        }
    }
    _tables.resize (index);
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
// Inherited from TableHandlerInterface
//----------------------------------------------------------------------------

void ts::StandaloneTableDemux::handleTable(SectionDemux&, const BinaryTable& table)
{
    _tables.push_back(new BinaryTable(table, ShareMode::SHARE));
}
