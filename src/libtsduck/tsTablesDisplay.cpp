//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Display PSI/SI tables.
//
//----------------------------------------------------------------------------

#include "tsTablesDisplay.h"
#include "tsFormat.h"
#include "tsDecimal.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsStringUtils.h"
#include "tsIntegerUtils.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsCAT.h"
#include "tsEIT.h"
#include "tsTSDT.h"
#include "tsNIT.h"
#include "tsBAT.h"
#include "tsTOT.h"
#include "tsTDT.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TablesDisplay::TablesDisplay(const TablesDisplayArgs& options, ReportInterface& report) :
    _opt(options),
    _report(report)
{
}


//----------------------------------------------------------------------------
// Display a table on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayTable(std::ostream& strm, const BinaryTable& table, int indent, CASFamily cas)
{
    // Filter invalid tables
    if (!table.isValid()) {
        return strm;
    }

    // Display hexa dump of each section in the table
    if (_opt.raw_dump) {
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            const Section& section(*table.sectionAt(i));
            strm << Hexa(section.content(), section.size(), _opt.raw_flags | hexa::BPL, indent, 16) << std::endl;
        }
        return strm;
    }

    const std::string margin(indent, ' ');

    // Compute total size of table
    size_t total_size = 0;
    for (size_t i = 0; i < table.sectionCount(); ++i) {
        total_size += table.sectionAt(i)->size();
    }

    // Display common header lines.
    strm << margin << "* " << names::TID(table.tableId(), casFamily(cas))
         << ", TID " << int(table.tableId())
         << Format(" (0x%02X)", int(table.tableId()));
    if (table.sourcePID() != PID_NULL) {
        // If PID is the null PID, this means "unknown PID"
        strm << ", PID " << table.sourcePID() << Format(" (0x%04X)", int(table.sourcePID()));
    }
    strm << std::endl
         << margin << "  Version: " << int(table.version())
         << ", sections: " << table.sectionCount()
         << ", total size: " << total_size << " bytes" << std::endl;

    // Loop across all sections.
    for (size_t i = 0; i < table.sectionCount(); ++i) {
        strm << margin << "  - Section " << i << ":" << std::endl;
        displaySection(strm, *table.sectionAt(i), indent + 4, cas, true);
    }

    return strm;
}


//----------------------------------------------------------------------------
// Display a section on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displaySection(std::ostream& strm, const Section& section, int indent, CASFamily cas, bool no_header)
{
    // Filter invalid section
    if (!section.isValid()) {
        return strm;
    }

    // Display hexa dump of the section
    if (_opt.raw_dump) {
        strm << Hexa(section.content(), section.size(), _opt.raw_flags | hexa::BPL, indent, 16) << std::endl;
        return strm;
    }

    const std::string margin(indent, ' ');
    const TID tid = section.tableId();

    // Display common header lines.
    if (!no_header) {
        strm << margin << "* " << names::TID(tid, cas) << ", TID " << int(tid) << ts::Format(" (0x%02X)", tid);
        if (section.sourcePID() != PID_NULL) {
            // If PID is the null PID, this means "unknown PID"
            strm << ", PID " << int(section.sourcePID()) << ts::Format(" (0x%04X)", int(section.sourcePID()));
        }
        strm << std::endl
             << margin << "  Section: " << int(section.sectionNumber())
             << " (last: " << int(section.lastSectionNumber())
             << "), version: " << int(section.version())
             << ", size: " << section.size() << " bytes" << std::endl;
        indent += 2;
    }

    // Display section body
    AbstractTable::DisplaySectionFunction handler = 0;
    if (tid >= TID_EIT_MIN && tid <= TID_EIT_MAX) { // 34 values
        handler = EIT::DisplaySection;
    }
    else {
        switch (tid) {
            case TID_PAT:     handler = PAT::DisplaySection; break;
            case TID_CAT:     handler = CAT::DisplaySection; break;
            case TID_PMT:     handler = PMT::DisplaySection; break;
            case TID_TSDT:    handler = TSDT::DisplaySection; break;
            case TID_NIT_ACT:
            case TID_NIT_OTH: handler = NIT::DisplaySection; break;
            case TID_BAT:     handler = BAT::DisplaySection; break;
            case TID_SDT_ACT:
            case TID_SDT_OTH: handler = SDT::DisplaySection; break;
            case TID_TDT:     handler = TDT::DisplaySection; break;
            case TID_TOT:     handler = TOT::DisplaySection; break;
            default:          handler = 0; break;
        }
    }
    if (handler != 0) {
        handler(strm, section, indent);
    }
    else {
        displayUnkownSection(strm, section, indent);
    }
    return strm;
}


//----------------------------------------------------------------------------
// Ancillary function to display an unknown section
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayUnkownSection(std::ostream& strm, const ts::Section& section, int indent)
{
    const std::string margin(indent, ' ');

    // The table id extension was not yet displayed since it depends on the table id.
    if (section.isLongSection()) {
        strm << margin
             << "TIDext: " << int(section.tableIdExtension())
             << Format(" (0x%04X)", int(section.tableIdExtension()))
             << std::endl;
    }

    // Display section payload.
    const uint8_t* const payload = section.payload();
    const size_t payloadSize = section.payloadSize();
    size_t index = 0;

    for (TLVSyntaxVector::const_iterator it = _opt.tlv_syntax.begin(); it != _opt.tlv_syntax.end() && index < payloadSize; ++it) {

        size_t start = 0;
        size_t size = 0;

        if (it->locateTLV(payload, payloadSize, start, size) && start >= index && size > 0) {

            // Display binary data preceding TLV.
            strm << Hexa(payload + index, start - index, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent, hexa::DEFAULT_LINE_WIDTH, index);
            index = start;

            // Display TLV fields.
            while (index < start + size && index < payloadSize) {
                uint32_t tag = 0;
                size_t length = 0;
                size_t header = it->getTagAndLength(payload + index, payloadSize - index, tag, length);
                if (header == 0 || index + header + length > payloadSize) {
                    break;
                }
                strm << margin
                     << Format("%04X:  Tag: %*u (0x%0*X), length: %*u bytes, value: ",
                               int(index),
                               int(MaxDecimalWidth(it->getTagSize())), int(tag),
                               int(MaxHexaWidth(it->getTagSize())), int(tag),
                               int(MaxDecimalWidth(it->getLengthSize())), int(length));
                if (length <= 8) {
                    // If value is short, display it on the same line.
                    strm << Hexa(payload + index + header, length, hexa::HEXA | hexa::SINGLE_LINE) << std::endl;
                }
                else {
                    strm << std::endl
                         << Hexa(payload + index + header, length, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent, hexa::DEFAULT_LINE_WIDTH, index + header);
                }
                index += header + length;
            }

            // Display a separator after TLV area.
            if (index < payloadSize) {
                strm << margin << Format("%04X:  End of TLV area", int(index)) << std::endl;
            }
        }
    }

    // Display remaining binary data.
    strm << Hexa(payload + index, payloadSize - index, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent, hexa::DEFAULT_LINE_WIDTH, index);
}
