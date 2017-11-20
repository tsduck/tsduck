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
//  This class maps PID's with CA system ids.
//
//----------------------------------------------------------------------------

#include "tsCASMapper.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsCAT.h"
#include "tsNames.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::CASMapper::CASMapper(Report& report) :
    TableHandlerInterface(),
    _report(report),
    _demux(this),
    _pids()
{
    // Specify the PID filters
    _demux.addPID(PID_PAT);
    _demux.addPID(PID_CAT);
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void ts::CASMapper::handleTable(SectionDemux&, const BinaryTable& table)
{
    assert(table.sectionCount() > 0);

    switch (table.tableId()) {

        case TID_PAT: {
            const PAT pat(table);
            if (pat.isValid()) {
                // Add a filter on each referenced PID to get all PMT's.
                for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                    _demux.addPID(it->second);
                }
            }
            break;
        }

        case TID_CAT: {
            const CAT cat(table);
            if (cat.isValid()) {
                // Identify all EMM PID's.
                analyzeCADescriptors(cat.descs, false);
            }
            break;
        }

        case TID_PMT: {
            const PMT pmt(table);
            if (pmt.isValid()) {
                // Identify all ECM PID's at program level.
                analyzeCADescriptors(pmt.descs, false);
                // Identify all ECM PID's at stream level.
                for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
                    analyzeCADescriptors(it->second.descs, true);
                }
            }
            break;
        }

        default: {
            if (_report.verbose()) {
                _report.verbose(u"Got unexpected TID %d (0x%X) on PID %d (0x%X)", {table.tableId(), table.tableId(), table.sourcePID(), table.sourcePID()});
            }
        }
    }
}

//----------------------------------------------------------------------------
// Explore a descriptor list and record EMM and ECM PID's.
//----------------------------------------------------------------------------

void ts::CASMapper::analyzeCADescriptors(const DescriptorList& descs, bool is_ecm)
{
    for (size_t i = 0; i < descs.count(); ++i) {
        const DescriptorPtr& desc(descs[i]);
        if (!desc.isNull() && desc->tag() == DID_CA) {
            const CADescriptorPtr cadesc(new CADescriptor(*desc));
            if (!cadesc.isNull() && cadesc->isValid()) {
                const std::string cas_name(names::CASId(cadesc->cas_id).toUTF8());
                _pids[cadesc->ca_pid] = PIDDescription(cadesc->cas_id, is_ecm, cadesc);
                _report.verbose(u"Found %s PID %d (0x%X) for CAS id 0x%X (%s)",
                                {is_ecm ? u"ECM" : u"EMM", cadesc->ca_pid, cadesc->ca_pid, cadesc->cas_id, cas_name});
            }
        }
    }
}


//----------------------------------------------------------------------------
// Get the characteristics of CA PID's.
//----------------------------------------------------------------------------

uint16_t ts::CASMapper::casId(PID pid) const
{
    const PIDDescriptionMap::const_iterator it(_pids.find(pid));
    return it == _pids.end() ? 0 : it->second.cas_id;
}

ts::CASFamily ts::CASMapper::casFamily(PID pid) const
{
    return CASFamilyOf(casId(pid));
}

bool ts::CASMapper::isECM(PID pid) const
{
    const PIDDescriptionMap::const_iterator it(_pids.find(pid));
    return it != _pids.end() && it->second.is_ecm;
}

bool ts::CASMapper::isEMM(PID pid) const
{
    const PIDDescriptionMap::const_iterator it(_pids.find(pid));
    return it != _pids.end() && !it->second.is_ecm;
}

bool ts::CASMapper::getCADescriptor(PID pid, CADescriptorPtr& desc) const
{
    const PIDDescriptionMap::const_iterator it(_pids.find(pid));
    if (it == _pids.end()) {
        desc.clear();
    }
    else {
        desc = it->second.ca_desc;
    }
    return !desc.isNull();
}
