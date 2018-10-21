//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsTablesFactory.h"
#include "tsNames.h"
#include "tsIntegerUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TablesDisplay::TablesDisplay(const TablesDisplayArgs& options, Report& report) :
    _opt(options),
    _report(report),
    _outfile(),
    _use_outfile(false)
{
}


//----------------------------------------------------------------------------
// The actual CAS family to use.
//----------------------------------------------------------------------------

ts::CASFamily ts::TablesDisplay::casFamily(CASFamily cas) const
{
    // Default implementation keeps the proposed CAS.
    // A subclass may change this behavior.
    return cas;
}


//----------------------------------------------------------------------------
// The actual private data specifier to use.
//----------------------------------------------------------------------------

ts::PDS ts::TablesDisplay::actualPDS(PDS pds) const
{
    return pds == 0 ? _opt.default_pds : pds;
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
#if !defined(TS_WINDOWS)
    if (!_use_outfile) {
        ::fflush(stdout);
        ::fsync(STDOUT_FILENO);
    }
#endif
}


//----------------------------------------------------------------------------
// Redirect the output stream to a file.
//----------------------------------------------------------------------------

bool ts::TablesDisplay::redirect(const UString& file_name)
{
    // Close previous file, if any.
    if (_use_outfile) {
        _outfile.close();
        _use_outfile = false;
    }

    // Open new file if any.
    if (!file_name.empty()) {
        _report.verbose(u"creating " + file_name);
        const std::string nameUTF8(file_name.toUTF8());
        _outfile.open(nameUTF8.c_str(), std::ios::out);
        if (!_outfile) {
            _report.error(u"cannot create " + file_name);
            return false;
        }
        _use_outfile = true;
    }

    return true;
}


//----------------------------------------------------------------------------
// A utility method to dump extraneous bytes after expected data.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayExtraData(const void* data, size_t size, int indent)
{
    std::ostream& strm(out());
    if (size > 0) {
        strm << std::string(indent, ' ') << "Extraneous " << size << " bytes:" << std::endl
             << UString::Dump(data, size, UString::HEXA | UString::ASCII | UString::OFFSET, indent);
    }
    return strm;
}


//----------------------------------------------------------------------------
// A utility method to display data if it can be interpreted as an ASCII string.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayIfASCII(const void *data, size_t size, const UString& prefix, const UString& suffix)
{
    std::ostream& strm(out());
    const std::string ascii(ToASCII(data, size));
    if (!ascii.empty()) {
        strm << prefix << ascii << suffix;
    }
    return strm;
}


//----------------------------------------------------------------------------
// A utility method to interpret data as an ASCII string.
//----------------------------------------------------------------------------

std::string ts::TablesDisplay::ToASCII(const void *data, size_t size)
{
    const char* str = reinterpret_cast<const char*>(data);
    size_t strSize = 0;

    for (size_t i = 0; i < size; ++i) {
        if (str[i] >= 0x20 && str[i] <= 0x7E) {
            // This is an ASCII character.
            if (i == strSize) {
                strSize++;
            }
            else {
                // But come after trailing zero.
                return std::string();
            }
        }
        else if (str[i] != 0) {
            // Not ASCII, not trailing zero, unusable string.
            return std::string();
        }
    }

    // Found an ASCII string.
    return std::string(str, strSize);
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
            strm << UString::Dump(section.content(), section.size(), _opt.raw_flags | UString::BPL, indent, 16) << std::endl;
        }
        return strm;
    }

    const std::string margin(indent, ' ');
    const TID tid = table.tableId();
    cas = casFamily(cas);

    // Compute total size of table
    size_t total_size = 0;
    for (size_t i = 0; i < table.sectionCount(); ++i) {
        total_size += table.sectionAt(i)->size();
    }

    // Display common header lines.
    strm << margin << UString::Format(u"* %s, TID %d (0x%X)", {names::TID(tid, cas), table.tableId(), table.tableId()});
    if (table.sourcePID() != PID_NULL) {
        // If PID is the null PID, this means "unknown PID"
        strm << UString::Format(u", PID %d (0x%X)", {table.sourcePID(), table.sourcePID()});
    }
    strm << std::endl;
    if (table.sectionCount() == 1 && table.sectionAt(0)->isShortSection()) {
        strm << margin << "  Short section";
    }
    else {
        strm << margin << "  Version: " << int(table.version()) << ", sections: " << table.sectionCount();
    }
    strm << ", total size: " << total_size << " bytes" << std::endl;

    // Loop across all sections.
    for (size_t i = 0; i < table.sectionCount(); ++i) {
        const SectionPtr section(table.sectionAt(i));
        strm << margin << "  - Section " << i;
        if (section->isNext()) {
            strm << ", next (not yet applicable)";
        }
        strm << ":" << std::endl;
        displaySection(*section, indent + 4, cas, true);
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
        strm << UString::Dump(section.content(), section.size(), _opt.raw_flags | UString::BPL, indent, 16) << std::endl;
        return strm;
    }

    const std::string margin(indent, ' ');
    const TID tid = section.tableId();
    cas = casFamily(cas);

    // Display common header lines.
    if (!no_header) {
        strm << margin << UString::Format(u"* %s, TID %d (0x%X)", {names::TID(tid, cas), tid, tid});
        if (section.sourcePID() != PID_NULL) {
            // If PID is the null PID, this means "unknown PID"
            strm << UString::Format(u", PID %d (0x%X)", {section.sourcePID(), section.sourcePID()});
        }
        strm << std::endl;
        if (section.isShortSection()) {
            strm << margin << "  Short section";
        }
        else {
            strm << margin << "  Section: " << int(section.sectionNumber())
                << " (last: " << int(section.lastSectionNumber())
                << "), version: " << int(section.version());
            if (section.isNext()) {
                strm << ", next (not yet applicable)";
            }
        }
        strm << ", size: " << section.size() << " bytes" << std::endl;
        indent += 2;
    }

    // Display section body
    return displaySectionData(section, indent, cas);
}


//----------------------------------------------------------------------------
// Display a section on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displaySectionData(const Section& section, int indent, CASFamily cas)
{
    DisplaySectionFunction handler = TablesFactory::Instance()->getSectionDisplay(section.tableId());

    if (handler != nullptr) {
        handler(*this, section, indent);
    }
    else {
        displayUnkownSectionData(section, indent);
    }
    return out();
}


//----------------------------------------------------------------------------
// Display the payload of a section on the output stream as a one-line "log" message.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::logSectionData(const Section& section, const UString& header, size_t max_bytes, CASFamily cas)
{
    std::ostream& strm(out());

    // Number of bytes to log.
    size_t log_size = section.payloadSize();
    if (max_bytes > 0 && max_bytes < log_size) {
        log_size = max_bytes;
    }

    // Output exactly one line.
    strm << header << UString::Dump(section.payload(), log_size, UString::SINGLE_LINE);
    if (section.payloadSize() > log_size) {
        strm << " ...";
    }
    strm << std::endl;

    return strm;
}


//----------------------------------------------------------------------------
// Display the content of an unknown descriptor.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayUnkownDescriptor(DID did, const uint8_t * payload, size_t size, int indent, TID tid, PDS pds)
{
    out() << UString::Dump(payload, size, UString::HEXA | UString::ASCII | UString::OFFSET, indent);
}


//----------------------------------------------------------------------------
// Display an unknown section
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayUnkownSectionData(const ts::Section& section, int indent)
{
    std::ostream& strm(out());
    const std::string margin(indent, ' ');

    // The table id extension was not yet displayed since it depends on the table id.
    if (section.isLongSection()) {
        strm << margin << UString::Format(u"TIDext: %d (0x%X)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;
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
                strm << UString::Format(u"%*s%04X:  End of TLV area", {indent, "", index}) << std::endl;
            }
        }
    }

    // Display remaining binary data.
    strm << UString::Dump(payload + index, payloadSize - index, UString::HEXA | UString::ASCII | UString::OFFSET, indent, UString::DEFAULT_HEXA_LINE_WIDTH, index);
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
    strm << UString::Dump(data, tlvStart, UString::HEXA | UString::ASCII | UString::OFFSET, indent, UString::DEFAULT_HEXA_LINE_WIDTH, dataOffset, innerIndent);

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
        strm << UString::Format(u"%*s%04X:  %*sTag: %*d (0x%0*X), length: %*d bytes, value: ",
                                {indent, u"",
                                 dataOffset + index,
                                 innerIndent, u"",
                                 MaxDecimalWidth(tlv.getTagSize()), tag,
                                 MaxHexaWidth(tlv.getTagSize()), tag,
                                 MaxDecimalWidth(tlv.getLengthSize()), valueSize});

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
            strm << UString::Dump(value, valueSize, UString::HEXA | UString::SINGLE_LINE) << std::endl;
        }
        else {
            strm << std::endl
                 << UString::Dump(value, valueSize, UString::HEXA | UString::ASCII | UString::OFFSET, indent, UString::DEFAULT_HEXA_LINE_WIDTH, valueOffset, innerIndent + 2);
        }

        // Point after current TLV record.
        index += headerSize + valueSize;
    }

    // Display a separator after TLV area.
    if (index > tlvStart && index < endIndex) {
        strm << UString::Format(u"%*s%04X:  %*sEnd of TLV area", {indent, u"", index, innerIndent, u""}) << std::endl;
    }

    // Display remaining binary data.
    strm << UString::Dump(data + index, endIndex - index, UString::HEXA | UString::ASCII | UString::OFFSET, indent, UString::DEFAULT_HEXA_LINE_WIDTH, dataOffset + index, innerIndent);
}


//----------------------------------------------------------------------------
// Display a descriptor on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayDescriptor(const Descriptor& desc, int indent, TID tid, PDS pds, CASFamily cas)
{
    if (desc.isValid()) {
        return displayDescriptorData(desc.tag(), desc.payload(), desc.payloadSize(), indent, tid, actualPDS(pds), cas);
    }
    else {
        return out();
    }
}


//----------------------------------------------------------------------------
// Display a list of descriptors from a memory area
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayDescriptorList(const void* data, size_t size, int indent, TID tid, PDS pds, CASFamily cas)
{
    std::ostream& strm(out());
    const std::string margin(indent, ' ');
    const uint8_t* desc_start = reinterpret_cast<const uint8_t*>(data);
    size_t desc_index = 0;

    // Loop across all descriptors
    while (size >= 2) {  // descriptor header size

        // Get descriptor header
        uint8_t desc_tag = *desc_start++;
        size_t desc_length = *desc_start++;
        size -= 2;

        if (desc_length > size) {
            strm << margin << "- Invalid descriptor length: " << desc_length
                 << " (" << size << " bytes allocated)" << std::endl;
            break;
        }

        // Display descriptor header
        strm << margin << "- Descriptor " << desc_index++ << ": "
             << names::DID(desc_tag, actualPDS(pds), tid, names::VALUE | names::BOTH) << ", "
             << desc_length << " bytes" << std::endl;

        // If the descriptor contains a private_data_specifier, keep it
        // to establish a private context.
        if (desc_tag == DID_PRIV_DATA_SPECIF && desc_length >= 4) {
            pds = GetUInt32(desc_start);
        }

        // Display descriptor.
        displayDescriptorData(desc_tag, desc_start, desc_length, indent + 2, tid, actualPDS(pds), cas);

        // Move to next descriptor for next iteration
        desc_start += desc_length;
        size -= desc_length;
    }

    // Report extraneous bytes
    displayExtraData(desc_start, size, indent);
    return strm;
}


//----------------------------------------------------------------------------
// Display a list of descriptors.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayDescriptorList(const DescriptorList& list, int indent, TID tid, PDS pds, CASFamily cas)
{
    std::ostream& strm(out());
    const std::string margin(indent, ' ');

    for (size_t i = 0; i < list.count(); ++i) {
        const DescriptorPtr& desc(list[i]);
        if (!desc.isNull()) {
            pds = list.privateDataSpecifier(i);
            strm << margin << "- Descriptor " << i << ": "
                 << names::DID(desc->tag(), actualPDS(pds), tid, names::VALUE | names::BOTH) << ", "
                 << desc->size() << " bytes" << std::endl;
            displayDescriptor(*desc, indent + 2, tid, actualPDS(pds), cas);
        }
    }

    return strm;
}


//----------------------------------------------------------------------------
// Display a descriptor on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayDescriptorData(DID did, const uint8_t* payload, size_t size, int indent, TID tid, ts::PDS pds, CASFamily cas)
{
    std::ostream& strm(out());

    // Compute extended descriptor id.
    EDID edid;
    if (did >= 0x80) {
        // Private descriptor.
        edid = EDID::Private(did, actualPDS(pds));
    }
    else if (did == DID_MPEG_EXTENSION && size >= 1) {
        // MPEG extension descriptor, the extension id is in the first byte of the payload.
        const uint8_t ext = *payload++;
        edid = EDID::ExtensionMPEG(ext);
        size--;
        // Display extended descriptor header
        strm << std::string(indent, ' ') << "MPEG extended descriptor: " << DVBNameFromSection(u"MPEGExtendedDescriptorId", ext, names::VALUE | names::BOTH) << std::endl;
    }
    else if (did == DID_DVB_EXTENSION && size >= 1) {
        // Extension descriptor, the extension id is in the first byte of the payload.
        const uint8_t ext = *payload++;
        edid = EDID::ExtensionDVB(ext);
        size--;
        // Display extended descriptor header
        strm << std::string(indent, ' ') << "Extended descriptor: " << names::EDID(ext, names::VALUE | names::BOTH) << std::endl;
    }
    else {
        // Simple descriptor.
        edid = EDID::Standard(did);
    }

    // Locate the display handler for this descriptor payload.
    DisplayDescriptorFunction handler = TablesFactory::Instance()->getDescriptorDisplay(edid, tid);

    if (handler != nullptr) {
        handler(*this, did, payload, size, indent, tid, actualPDS(pds));
    }
    else {
        displayUnkownDescriptor(did, payload, size, indent, tid, actualPDS(pds));
    }

    return strm;
}
