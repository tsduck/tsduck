//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsBinaryTable.h"
#include "tsPSIBuffer.h"
#include "tsSection.h"
#include "tsDescriptor.h"
#include "tsDescriptorList.h"
#include "tsNames.h"
#include "tsIntegerUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructors.
//----------------------------------------------------------------------------

ts::TablesDisplay::TablesDisplay(DuckContext& d) :
    _duck(d),
    _raw_dump(false),
    _raw_flags(UString::HEXA),
    _tlv_syntax(),
    _min_nested_tlv(0)
{
}

ts::TablesDisplay::~TablesDisplay()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesDisplay::defineArgs(Args& args) const
{
    args.option(u"c-style", 'c');
    args.help(u"c-style",
              u"Same as --raw-dump (no interpretation of section) but dump the "
              u"bytes in C-language style.");

    args.option(u"nested-tlv", 0, Args::POSITIVE, 0, 1, 0, 0, true);
    args.help(u"nested-tlv", u"min-size",
              u"With option --tlv, try to interpret the value field of each TLV record as "
              u"another TLV area. If the min-size value is specified, the nested TLV "
              u"interpretation is performed only on value fields larger than this size. "
              u"The syntax of the nested TLV is the same as the enclosing TLV.");

    args.option(u"raw-dump", 'r');
    args.help(u"raw-dump", u"Raw dump of section, no interpretation.");

    args.option(u"tlv", 0, Args::STRING, 0, Args::UNLIMITED_COUNT);
    args.help(u"tlv", u"For sections of unknown types, this option specifies how to interpret "
              u"some parts of the section payload as TLV records. Several --tlv options "
              u"are allowed, each one describes a part of the section payload.\n\n"
              u"Each syntax string has the form \"start,size,tagSize,lengthSize,order\". "
              u"The start and size fields define the offset and size of the TLV area "
              u"in the section payload. If the size field is \"auto\", the TLV extends up "
              u"to the end of the section. If the start field is \"auto\", the longest "
              u"TLV area in the section payload will be used. The fields tagSize and "
              u"lengthSize indicate the size in bytes of the Tag and Length fields in "
              u"the TLV structure. The field order must be either \"msb\" or \"lsb\" and "
              u"indicates the byte order of the Tag and Length fields.\n\n"
              u"All fields are optional. The default values are \"auto,auto,1,1,msb\".");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TablesDisplay::loadArgs(DuckContext& duck, Args &args)
{
    _raw_dump = args.present(u"raw-dump");
    _raw_flags = UString::HEXA;
    if (args.present(u"c-style")) {
        _raw_dump = true;
        _raw_flags |= UString::C_STYLE;
    }

    // The --nested-tlv has an optional value.
    // If present without value, use 1, meaning all non-empty TLV records.
    // If not present, we use 0, which means no nested TLV.
    _min_nested_tlv = args.present(u"nested-tlv") ? args.intValue<size_t>(u"nested-tlv", 1) : 0;

    // Get all TLV syntax specifications.
    _tlv_syntax.clear();
    const size_t count = args.count(u"tlv");
    for (size_t i = 0; i < count; ++i) {
        TLVSyntax tlv;
        tlv.fromString(args.value(u"tlv", u"", i), args);
        _tlv_syntax.push_back(tlv);
    }
    std::sort(_tlv_syntax.begin(), _tlv_syntax.end());
    return true;
}


//----------------------------------------------------------------------------
// A utility method to dump extraneous bytes after expected data.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayExtraData(PSIBuffer& buf, int indent)
{
    displayExtraData(buf.currentReadAddress(), buf.remainingReadBytes(), indent);
    buf.skipBytes(buf.remainingReadBytes());
    return _duck.out();
}

std::ostream& ts::TablesDisplay::displayExtraData(const void* data, size_t size, int indent)
{
    std::ostream& strm(_duck.out());
    if (size > 0) {
        strm << std::string(indent, ' ') << "Extraneous " << size << " bytes:" << std::endl
             << UString::Dump(data, size, UString::HEXA | UString::ASCII | UString::OFFSET, indent);
    }
    return strm;
}


//----------------------------------------------------------------------------
// A utility method to dump private binary data in a descriptor or section.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayPrivateData(const UString& title, const void* data, size_t size, int indent, size_t single_line_max)
{
    std::ostream& strm(_duck.out());
    const std::string margin(indent, ' ');

    if (size > single_line_max) {
        strm << margin << title << " (" << size << " bytes):" << std::endl
             << UString::Dump(data, size, UString::HEXA | UString::ASCII | UString::OFFSET | UString::BPL, indent + 2, 16);
    }
    else if (size > 0) {
        strm << margin << title << " (" << size << " bytes): "
             << UString::Dump(data, size, UString::SINGLE_LINE)
             << std::endl;
    }

    return strm;
}


//----------------------------------------------------------------------------
// Display a table on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayTable(const BinaryTable& table, int indent, uint16_t cas)
{
    std::ostream& strm(_duck.out());

    // Filter invalid tables
    if (!table.isValid()) {
        return strm;
    }

    // Display hexa dump of each section in the table
    if (_raw_dump) {
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            const Section& section(*table.sectionAt(i));
            strm << UString::Dump(section.content(), section.size(), _raw_flags | UString::BPL, indent, 16) << std::endl;
        }
        return strm;
    }

    const std::string margin(indent, ' ');
    const TID tid = table.tableId();
    cas = _duck.casId(cas);

    // Compute total size of table
    size_t total_size = 0;
    for (size_t i = 0; i < table.sectionCount(); ++i) {
        total_size += table.sectionAt(i)->size();
    }

    // Display common header lines.
    strm << margin << UString::Format(u"* %s, TID %d (0x%X)", {names::TID(_duck, tid, cas), table.tableId(), table.tableId()});
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

std::ostream& ts::TablesDisplay::displaySection(const Section& section, int indent, uint16_t cas, bool no_header)
{
    std::ostream& strm(_duck.out());

    // Filter invalid section
    if (!section.isValid()) {
        return strm;
    }

    // Display hexa dump of the section
    if (_raw_dump) {
        strm << UString::Dump(section.content(), section.size(), _raw_flags | UString::BPL, indent, 16) << std::endl;
        return strm;
    }

    const std::string margin(indent, ' ');
    const TID tid = section.tableId();
    cas = _duck.casId(cas);

    // Display common header lines.
    if (!no_header) {
        strm << margin << UString::Format(u"* %s, TID %d (0x%X)", {names::TID(_duck, tid, cas), tid, tid});
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

std::ostream& ts::TablesDisplay::displaySectionData(const Section& section, int indent, uint16_t cas)
{
    // Update CAS with default one if necessary.
    cas = _duck.casId(cas);

    // Find the display handler for this table id (and maybe CAS).
    DisplaySectionFunction handler = PSIRepository::Instance()->getSectionDisplay(section.tableId(), _duck.standards(), section.sourcePID(), cas);

    if (handler != nullptr) {
        handler(*this, section, indent);
    }
    else {
        displayUnkownSectionData(section, indent);
    }
    return _duck.out();
}


//----------------------------------------------------------------------------
// Display the payload of a section on the output stream as a one-line "log" message.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::logSectionData(const Section& section, const UString& header, size_t max_bytes, uint16_t cas)
{
    // Update CAS with default one if necessary.
    cas = _duck.casId(cas);

    // Find the log handler for this table id (and maybe CAS).
    LogSectionFunction handler = PSIRepository::Instance()->getSectionLog(section.tableId(), _duck.standards(), section.sourcePID(), cas);
    if (handler == nullptr) {
        handler = LogUnknownSectionData;
    }

    // Output exactly one line.
    std::ostream& strm(_duck.out());
    strm << header << handler(section, max_bytes) << std::endl;
    return strm;
}


//----------------------------------------------------------------------------
// Log the content of an unknown section.
//----------------------------------------------------------------------------

ts::UString ts::TablesDisplay::LogUnknownSectionData(const Section& section, size_t max_bytes)
{
    // Number of bytes to log.
    size_t log_size = section.payloadSize();
    if (max_bytes > 0 && max_bytes < log_size) {
        log_size = max_bytes;
    }

    // Build log line.
    return UString::Dump(section.payload(), log_size, UString::SINGLE_LINE) + (section.payloadSize() > log_size ? u" ..." : u"");
}


//----------------------------------------------------------------------------
// Display the content of an unknown descriptor.
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayUnkownDescriptor(DID did, const uint8_t * payload, size_t size, int indent, TID tid, PDS pds)
{
    _duck.out() << UString::Dump(payload, size, UString::HEXA | UString::ASCII | UString::OFFSET, indent);
}


//----------------------------------------------------------------------------
// Display an unknown section
//----------------------------------------------------------------------------

void ts::TablesDisplay::displayUnkownSectionData(const ts::Section& section, int indent)
{
    std::ostream& strm(_duck.out());
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
    for (TLVSyntaxVector::const_iterator it = _tlv_syntax.begin(); it != _tlv_syntax.end() && index < payloadSize; ++it) {

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
    std::ostream& strm(_duck.out());

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
        if (_min_nested_tlv > 0 && valueSize >= _min_nested_tlv && tlvInner.locateTLV(value, valueSize, tlvInnerStart, tlvInnerSize)) {
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

std::ostream& ts::TablesDisplay::displayDescriptor(const Descriptor& desc, int indent, TID tid, PDS pds, uint16_t cas)
{
    if (desc.isValid()) {
        return displayDescriptorData(desc.tag(), desc.payload(), desc.payloadSize(), indent, tid, _duck.actualPDS(pds), cas);
    }
    else {
        return _duck.out();
    }
}


//----------------------------------------------------------------------------
// Display a list of descriptors from a PSI Buffer
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayDescriptorListWithLength(const Section& section, PSIBuffer& buf, int indent, const UString& title, size_t length_bits, uint16_t cas)
{
    // Read the length field.
    const size_t length = buf.getUnalignedLength(length_bits);
    bool ok = !buf.readError();

    // Read and display descriptors.
    if (ok && length > 0) {
        if (!title.empty()) {
            _duck.out() << std::string(indent, ' ') << title << std::endl;
        }
        displayDescriptorList(section, buf.currentReadAddress(), length, indent, cas);
        buf.skipBytes(length);
    }
    return _duck.out();
}


//----------------------------------------------------------------------------
// Display a list of descriptors from a memory area
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayDescriptorList(const Section& section, const void* data, size_t size, int indent, uint16_t cas)
{
    std::ostream& strm(_duck.out());
    const std::string margin(indent, ' ');
    const uint8_t* desc_start = reinterpret_cast<const uint8_t*>(data);
    size_t desc_index = 0;
    const TID tid = section.tableId();

    // Compute default PDS. Use fake PDS for descriptors in ATSC context.
    const PDS default_pds = _duck.actualPDS(0);
    PDS pds = default_pds;

    // Loop across all descriptors
    while (size >= 2) {  // descriptor header size

        // Get descriptor header
        uint8_t desc_tag = *desc_start++;
        size_t desc_length = *desc_start++;
        size -= 2;

        if (desc_length > size) {
            strm << margin << "- Invalid descriptor length: " << desc_length << " (" << size << " bytes allocated)" << std::endl;
            break;
        }

        // Display descriptor header
        strm << margin << "- Descriptor " << desc_index++ << ": "
             << names::DID(desc_tag, pds, tid, names::VALUE | names::BOTH) << ", "
             << desc_length << " bytes" << std::endl;

        // If the descriptor contains a private_data_specifier, keep it
        // to establish a private context.
        if (desc_tag == DID_PRIV_DATA_SPECIF && desc_length >= 4) {
            pds = GetUInt32(desc_start);
            // PDS zero means return to default value.
            if (pds == 0) {
                pds = default_pds;
            }
        }

        // Display descriptor.
        displayDescriptorData(desc_tag, desc_start, desc_length, indent + 2, tid, pds, cas);

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

std::ostream& ts::TablesDisplay::displayDescriptorList(const DescriptorList& list, int indent, uint16_t cas)
{
    std::ostream& strm(_duck.out());
    const std::string margin(indent, ' ');
    const TID tid = list.tableId();

    for (size_t i = 0; i < list.count(); ++i) {
        const DescriptorPtr& desc(list[i]);
        if (!desc.isNull()) {
            const PDS pds = list.privateDataSpecifier(i);
            strm << margin << "- Descriptor " << i << ": "
                 << names::DID(desc->tag(), _duck.actualPDS(pds), tid, names::VALUE | names::BOTH) << ", "
                 << desc->size() << " bytes" << std::endl;
            displayDescriptor(*desc, indent + 2, tid, _duck.actualPDS(pds), cas);
        }
    }

    return strm;
}


//----------------------------------------------------------------------------
// Display a descriptor on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayDescriptorData(DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds, uint16_t cas)
{
    std::ostream& strm(_duck.out());

    // Compute extended descriptor id.
    EDID edid;
    if (did >= 0x80) {
        // Private descriptor.
        edid = EDID::Private(did, _duck.actualPDS(pds));
    }
    else if (did == DID_MPEG_EXTENSION && size >= 1) {
        // MPEG extension descriptor, the extension id is in the first byte of the payload.
        const uint8_t ext = *payload++;
        edid = EDID::ExtensionMPEG(ext);
        size--;
        // Display extended descriptor header
        strm << std::string(indent, ' ') << "MPEG extended descriptor: " << NameFromSection(u"MPEGExtendedDescriptorId", ext, names::VALUE | names::BOTH) << std::endl;
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
    DisplayDescriptorFunction handler = PSIRepository::Instance()->getDescriptorDisplay(edid, tid);

    if (handler != nullptr) {
        handler(*this, did, payload, size, indent, tid, _duck.actualPDS(pds));
    }
    else {
        displayUnkownDescriptor(did, payload, size, indent, tid, _duck.actualPDS(pds));
    }

    return strm;
}
