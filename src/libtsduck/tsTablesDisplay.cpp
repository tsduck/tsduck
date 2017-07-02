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
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TablesDisplay::TablesDisplay(const TablesDisplayArgs& options, ReportInterface& report) :
    _opt(options),
    _report(report),
    _outfile(),
    _use_outfile(false)
{
}


//----------------------------------------------------------------------------
// Get the current output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::out()
{
    return _use_outfile ? _outfile : std::cout;
}


//----------------------------------------------------------------------------
// Flush the text output.
//----------------------------------------------------------------------------

void ts::TablesDisplay::flush()
{
    // Flush the output.
    out().flush();

    // On Windows, we must force the lower-level standard output.
#if !defined(__windows)
    if (!_use_outfile) {
        ::fflush(stdout);
        ::fsync(STDOUT_FILENO);
    }
#endif
}


//----------------------------------------------------------------------------
// Redirect the output stream to a file.
//----------------------------------------------------------------------------

bool ts::TablesDisplay::redirect(const std::string& file_name)
{
    // Close previous file, if any.
    if (_use_outfile) {
        _outfile.close();
        _use_outfile = false;
    }

    // Open new file if any.
    if (!file_name.empty()) {
        _report.verbose("creating " + file_name);
        _outfile.open(file_name.c_str(), std::ios::out);
        if (!_outfile) {
            _report.error("cannot create " + file_name);
            return false;
        }
        _use_outfile = true;
    }

    return true;
}


//----------------------------------------------------------------------------
// Display a table on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayTable(const BinaryTable& table, int indent, CASFamily cas)
{
    std::ostream& strm(out());

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
        displaySection(*table.sectionAt(i), indent + 4, cas, true);
    }

    return strm;
}


//----------------------------------------------------------------------------
// Display a section on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displaySection(const Section& section, int indent, CASFamily cas, bool no_header)
{
    std::ostream& strm(out());

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
            case TID_PAT:
                handler = PAT::DisplaySection;
                break;
            case TID_CAT:
                handler = CAT::DisplaySection;
                break;
            case TID_PMT:
                handler = PMT::DisplaySection;
                break;
            case TID_TSDT:
                handler = TSDT::DisplaySection;
                break;
            case TID_NIT_ACT:
            case TID_NIT_OTH:
                handler = NIT::DisplaySection;
                break;
            case TID_BAT:
                handler = BAT::DisplaySection;
                break;
            case TID_SDT_ACT:
            case TID_SDT_OTH:
                handler = SDT::DisplaySection;
                break;
            case TID_TDT:
                handler = TDT::DisplaySection;
                break;
            case TID_TOT:
                handler = TOT::DisplaySection;
                break;
            default:
                handler = 0;
                break;
        }
    }
    if (handler != 0) {
        handler(strm, section, indent);
    }
    else {
        displayUnkownSection(section, indent);
    }
    return strm;
}


//----------------------------------------------------------------------------
// Ancillary function to display an unknown section
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayUnkownSection(const ts::Section& section, int indent)
{
    std::ostream& strm(out());
    const std::string margin(indent, ' ');

    // The table id extension was not yet displayed since it depends on the table id.
    if (section.isLongSection()) {
        strm << margin
             << "TIDext: " << int(section.tableIdExtension())
             << Format(" (0x%04X)", int(section.tableIdExtension()))
             << std::endl;
    }

    // Section payload.
    const uint8_t* const payload = section.payload();
    const size_t payloadSize = section.payloadSize();

    // Current index to display in payload.
    size_t index = 0;

    // Loop on all possible TLV syntaxen.
    for (TLVSyntaxVector::const_iterator it = _opt.tlv_syntax.begin(); it != _opt.tlv_syntax.end() && index < payloadSize; ++it) {

        // Can we locate a TLV area after current index?
        size_t tlvStart = 0;
        size_t tlvSize = 0;
        if (it->locateTLV(payload, payloadSize, tlvStart, tlvSize) && tlvStart >= index && tlvSize > 0) {

            // Display TLV fields, from index to end of TLV area.
            const size_t endIndex = index + tlvStart + tlvSize;
            displayTLV(payload + index,    // start of area to display
                       tlvStart - index,   // offset of TLV records in area to display
                       tlvSize,            // total size of TLV records
                       index,              // offset to display for start of area
                       indent,             // left margin
                       0,                  // inner margin
                       *it);               // TLV syntax
            index = endIndex;

            // Display a separator after TLV area.
            if (index < payloadSize) {
                strm << Format("%*s%04X:  End of TLV area", indent, "", int(index)) << std::endl;
            }
        }
    }

    // Display remaining binary data.
    strm << Hexa(payload + index, payloadSize - index, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent, hexa::DEFAULT_LINE_WIDTH, index);
}


//----------------------------------------------------------------------------
// Display a memory area containing a list of TLV records.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayTLV(const uint8_t* data,
                                   size_t tlvStart,
                                   size_t tlvSize,
                                   size_t dataOffset,
                                   int indent,
                                   int innerIndent,
                                   const TLVSyntax& tlv)
{
    std::ostream& strm(out());

    // We use the same syntax for the optional embedded TLV, except that it is automatically located.
    TLVSyntax tlvInner(tlv);
    tlvInner.setAutoLocation();

    // Display binary data preceding TLV, from data to data + tlvStart.
    strm << Hexa(data, tlvStart, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent, hexa::DEFAULT_LINE_WIDTH, dataOffset, innerIndent);

    // Display TLV fields, from data + tlvStart to data + tlvStart + tlvSize.
    size_t index = tlvStart;
    const size_t endIndex = tlvStart + tlvSize;
    while (index < endIndex) {

        // Get TLV header (tag, length)
        uint32_t tag = 0;
        size_t valueSize = 0;
        const size_t headerSize = tlv.getTagAndLength(data + index, endIndex - index, tag, valueSize);
        if (headerSize == 0 || index + headerSize + valueSize > endIndex) {
            break; // no more TLV record
        }

        // Location of value area.
        const uint8_t* const value = data + index + headerSize;
        const size_t valueOffset = dataOffset + index + headerSize;

        // Description of the TLV record.
        strm << Format("%*s%04X:  %*sTag: %*u (0x%0*X), length: %*u bytes, value: ",
                       indent, "",
                       int(dataOffset + index),
                       innerIndent, "",
                       int(MaxDecimalWidth(tlv.getTagSize())), int(tag),
                       int(MaxHexaWidth(tlv.getTagSize())), int(tag),
                       int(MaxDecimalWidth(tlv.getLengthSize())), int(valueSize));

        // Display the value field.
        size_t tlvInnerStart = 0;
        size_t tlvInnerSize = 0;
        if (_opt.min_nested_tlv > 0 && valueSize >= _opt.min_nested_tlv && tlvInner.locateTLV(value, valueSize, tlvInnerStart, tlvInnerSize)) {
            // Found a nested TLV area.
            strm << std::endl;
            displayTLV(value, tlvInnerStart, tlvInnerSize, valueOffset, indent, innerIndent + 2, tlvInner);
        }
        else if (valueSize <= 8) {
            // If value is short, display it on the same line.
            strm << Hexa(value, valueSize, hexa::HEXA | hexa::SINGLE_LINE) << std::endl;
        }
        else {
            strm << std::endl
                 << Hexa(value, valueSize, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent, hexa::DEFAULT_LINE_WIDTH, valueOffset, innerIndent + 2);
        }

        // Point after current TLV record.
        index += headerSize + valueSize;
    }

    // Display a separator after TLV area.
    if (index > tlvStart && index < endIndex) {
        strm << Format("%*s%04X:  %*sEnd of TLV area", indent, "", int(index), innerIndent, "") << std::endl;
    }

    // Display remaining binary data.
    strm << Hexa(data + index, endIndex - index, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent, hexa::DEFAULT_LINE_WIDTH, dataOffset + index, innerIndent);
}
