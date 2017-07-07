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
#include "tsTables.h"
#include "tsFormat.h"
#include "tsDecimal.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsStringUtils.h"
#include "tsIntegerUtils.h"
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
// The actual CAS family to use.
//----------------------------------------------------------------------------

ts::CASFamily ts::TablesDisplay::casFamily(CASFamily cas) const
{
    // Default implementation keeps the proposed CAS.
    // A subclass may change this behavior.
    return cas;
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
// A utility method to dump extraneous bytes after expected data.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayExtraData(const void* data, size_t size, int indent)
{
    std::ostream& strm(out());
    if (size > 0) {
        strm << std::string(indent, ' ') << "Extraneous " << size << " bytes:" << std::endl
             << Hexa(data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
    }
    return strm;
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
    const TID tid = table.tableId();
    cas = casFamily(cas);

    // Compute total size of table
    size_t total_size = 0;
    for (size_t i = 0; i < table.sectionCount(); ++i) {
        total_size += table.sectionAt(i)->size();
    }

    // Display common header lines.
    strm << margin << "* " << names::TID(tid, cas)
         << ", TID " << int(table.tableId())
         << Format(" (0x%02X)", int(table.tableId()));
    if (table.sourcePID() != PID_NULL) {
        // If PID is the null PID, this means "unknown PID"
        strm << ", PID " << table.sourcePID() << Format(" (0x%04X)", int(table.sourcePID()));
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
    cas = casFamily(cas);

    // Display common header lines.
    if (!no_header) {
        strm << margin << "* " << names::TID(tid, cas) << ", TID " << int(tid) << ts::Format(" (0x%02X)", tid);
        if (section.sourcePID() != PID_NULL) {
            // If PID is the null PID, this means "unknown PID"
            strm << ", PID " << int(section.sourcePID()) << ts::Format(" (0x%04X)", int(section.sourcePID()));
        }
        strm << std::endl;
        if (section.isShortSection()) {
            strm << margin << "  Short section";
        }
        else {
            strm << margin << "  Section: " << int(section.sectionNumber())
                << " (last: " << int(section.lastSectionNumber())
                << "), version: " << int(section.version());
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
    AbstractTable::DisplaySectionFunction handler = 0;
    const TID tid = section.tableId();
    cas = casFamily(cas);

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
                break;
        }
    }
    if (handler != 0) {
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

std::ostream& ts::TablesDisplay::logSectionData(const Section& section, const std::string& header, size_t max_bytes, CASFamily cas)
{
    std::ostream& strm(out());

    // Number of bytes to log.
    size_t log_size = section.payloadSize();
    if (max_bytes > 0 && max_bytes < log_size) {
        log_size = max_bytes;
    }

    // Output exactly one line.
    strm << header << Hexa(section.payload(), log_size, hexa::SINGLE_LINE);
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
    out() << Hexa(payload, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
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


//----------------------------------------------------------------------------
// Display a descriptor on the output stream.
//----------------------------------------------------------------------------

std::ostream& ts::TablesDisplay::displayDescriptor(const Descriptor& desc, int indent, TID tid, PDS pds, CASFamily cas)
{
    if (desc.isValid()) {
        return displayDescriptorData(desc.tag(), desc.payload(), desc.payloadSize(), indent, tid, pds, cas);
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
        strm << margin << "- Descriptor " << desc_index++
             << ": " << names::DID(desc_tag, pds) << ", Tag " << int(desc_tag)
             << Format(" (0x%02X), ", int(desc_tag)) << desc_length << " bytes" << std::endl;

        // If the descriptor contains a private_data_specifier, keep it
        // to establish a private context.
        if (desc_tag == DID_PRIV_DATA_SPECIF && desc_length >= 4) {
            pds = GetUInt32(desc_start);
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

std::ostream& ts::TablesDisplay::displayDescriptorList(const DescriptorList& list, int indent, TID tid, PDS pds, CASFamily cas)
{
    std::ostream& strm(out());
    const std::string margin(indent, ' ');

    for (size_t i = 0; i < list.count(); ++i) {
        const DescriptorPtr& desc(list[i]);
        if (!desc.isNull()) {
            pds = list.privateDataSpecifier(i);
            strm << margin << "- Descriptor " << i
                 << ": " << names::DID(desc->tag(), pds) << ", Tag " << int(desc->tag())
                 << Format(" (0x%02X), ", int(desc->tag())) << desc->size() << " bytes" << std::endl;
            displayDescriptor(*desc, indent + 2, tid, pds, cas);
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
    AbstractDescriptor::DisplayDescriptorFunction handler = 0;

    switch (did) {
        case DID_AAC:               handler = AACDescriptor::DisplayDescriptor; break;
        case DID_AC3:               handler = AC3Descriptor::DisplayDescriptor; break;
        case DID_APPLI_SIGNALLING:  handler = ApplicationSignallingDescriptor::DisplayDescriptor; break;
        case DID_BOUQUET_NAME:      handler = BouquetNameDescriptor::DisplayDescriptor; break;
        case DID_CA:                handler = CADescriptor::DisplayDescriptor; break;
        case DID_CA_ID:             handler = CAIdentifierDescriptor::DisplayDescriptor; break;
        case DID_CABLE_DELIVERY:    handler = CableDeliverySystemDescriptor::DisplayDescriptor; break; 
        case DID_COMPONENT:         handler = ComponentDescriptor::DisplayDescriptor; break;
        case DID_CONTENT:           handler = ContentDescriptor::DisplayDescriptor; break;
        case DID_COUNTRY_AVAIL:     handler = CountryAvailabilityDescriptor::DisplayDescriptor; break;
        case DID_DATA_BROADCAST:    handler = DataBroadcastDescriptor::DisplayDescriptor; break;
        case DID_DATA_BROADCAST_ID: handler = DataBroadcastIdDescriptor::DisplayDescriptor; break;
        case DID_DTS:               handler = DTSDescriptor::DisplayDescriptor; break;
        case DID_ENHANCED_AC3:      handler = EnhancedAC3Descriptor::DisplayDescriptor; break;
        case DID_EXTENDED_EVENT:    handler = ExtendedEventDescriptor::DisplayDescriptor; break;
        case DID_LANGUAGE:          handler = ISO639LanguageDescriptor::DisplayDescriptor; break;
        case DID_LINKAGE:           handler = LinkageDescriptor::DisplayDescriptor; break;
        case DID_LOCAL_TIME_OFFSET: handler = LocalTimeOffsetDescriptor::DisplayDescriptor; break;
        case DID_NETWORK_NAME:      handler = NetworkNameDescriptor::DisplayDescriptor; break;
        case DID_PARENTAL_RATING:   handler = ParentalRatingDescriptor::DisplayDescriptor; break;
        case DID_PRIV_DATA_SPECIF:  handler = PrivateDataSpecifierDescriptor::DisplayDescriptor; break;
        case DID_SAT_DELIVERY:      handler = SatelliteDeliverySystemDescriptor::DisplayDescriptor; break;
        case DID_SERVICE:           handler = ServiceDescriptor::DisplayDescriptor; break;
        case DID_SERVICE_LIST:      handler = ServiceListDescriptor::DisplayDescriptor; break;
        case DID_SHORT_EVENT:       handler = ShortEventDescriptor::DisplayDescriptor; break;
        case DID_STD:               handler = STDDescriptor::DisplayDescriptor; break;
        case DID_STREAM_ID:         handler = StreamIdentifierDescriptor::DisplayDescriptor; break;
        case DID_SUBTITLING:        handler = SubtitlingDescriptor::DisplayDescriptor; break;
        case DID_TELETEXT:          handler = TeletextDescriptor::DisplayDescriptor; break;
        case DID_TERREST_DELIVERY:  handler = TerrestrialDeliverySystemDescriptor::DisplayDescriptor; break;
        case DID_VBI_DATA:          handler = VBIDataDescriptor::DisplayDescriptor; break;
        case DID_VBI_TELETEXT:      handler = VBITeletextDescriptor::DisplayDescriptor; break;
        case DID_EXTENSION: {
            // Extension descriptor.
            if (size >= 1) {
                // Get extended descriptor tag
                const uint8_t edid = *payload++;
                size--;
                // Display extended descriptor header
                strm << std::string(indent, ' ') << "Extended descriptor: " << names::EDID(edid, 0)
                     << Format(", Tag %d (0x%02X)", int(edid), int(edid)) << std::endl;
                // Determine display handler for the descriptor.
                switch (edid) {
                    case EDID_MESSAGE:     handler = MessageDescriptor::DisplayDescriptor; break;
                    case EDID_SUPPL_AUDIO: handler = SupplementaryAudioDescriptor::DisplayDescriptor; break;
                    default: break;
                }
            }
            break;
        }
        default:
            break;
    }
    
    switch (pds) {
        case PDS_TPS:
            // Incorrect use of TPS private data, TPS broadcasters should
            // use EACEM/EICTA PDS instead.
        case PDS_EICTA:
            // European Association of Consumer Electronics Manufacturers.
            // Descriptors are defined in IEC/CENELEC 62216-1
            // "Baseline terrestrial receiver specification"
            switch (did) {
                case DID_LOGICAL_CHANNEL_NUM: handler = LogicalChannelNumberDescriptor::DisplayDescriptor; break;
                case DID_PREF_NAME_LIST:      handler = EacemPreferredNameListDescriptor::DisplayDescriptor; break;
                case DID_PREF_NAME_ID:        handler = EacemPreferredNameIdentifierDescriptor::DisplayDescriptor; break;
                case DID_EACEM_STREAM_ID:     handler = EacemStreamIdentifierDescriptor::DisplayDescriptor; break;
                case DID_HD_SIMULCAST_LCN:    handler = HDSimulcastLogicalChannelDescriptor::DisplayDescriptor; break;
                default: break;
            }
            break;

        case PDS_EUTELSAT:
            // Eutelsat operator, including Fransat.
            switch (did) {
                case DID_EUTELSAT_CHAN_NUM: handler = EutelsatChannelNumberDescriptor::DisplayDescriptor; break;
                default: break;
            }
            break;

        default:
            break;
    }

    if (handler != 0) {
        handler(*this, did, payload, size, indent, tid, pds);
    }
    else {
        displayUnkownDescriptor(did, payload, size, indent, tid, pds);
    }

    return strm;
}
