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
//  This module contains the display routines for class ts::BinaryTable
//
//----------------------------------------------------------------------------

#include "tsBinaryTable.h"
#include "tsFormat.h"
#include "tsNames.h"



//----------------------------------------------------------------------------
// Display the table on an output stream
//----------------------------------------------------------------------------

std::ostream& ts::BinaryTable::display (std::ostream& strm, int indent, CASFamily cas) const
{
    const std::string margin (indent, ' ');

    // Filter invalid tables

    if (!_is_valid) {
        return strm;
    }

    // Compute total size of table

    size_t total_size (0);
    for (size_t i = 0; i < _sections.size(); ++i) {
        total_size += _sections[i]->size();
    }

    // Display common header lines.
    // If PID is the null PID, this means "unknown PID"

    strm << margin << "* " << names::TID (_tid, cas)
         << ", TID " << int (_tid)
         << Format (" (0x%02X)", int (_tid));
    if (_source_pid != PID_NULL) {
        strm << ", PID " << _source_pid << Format (" (0x%04X)", int (_source_pid));
    }
    strm << std::endl
         << margin << "  Version: " << int (_version)
         << ", sections: " << _sections.size()
         << ", total size: " << total_size << " bytes" << std::endl;

    // Loop across all sections

    for (size_t i = 0; i < _sections.size(); ++i) {
        strm << margin << "  - Section " << i << ":" << std::endl;
        _sections[i]->display (strm, indent + 4, cas, true);
    }

    return strm;
}
