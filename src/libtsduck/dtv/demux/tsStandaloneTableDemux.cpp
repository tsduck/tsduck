//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStandaloneTableDemux.h"
#include "tsBinaryTable.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::StandaloneTableDemux::StandaloneTableDemux(DuckContext& duck, const PIDSet& pid_filter) :
    SectionDemux(duck, this, nullptr, pid_filter)
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
