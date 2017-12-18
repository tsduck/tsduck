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
//  This class logs sections and tables.
//
//----------------------------------------------------------------------------

#include "tsTablesLogger.h"
#include "tsPAT.h"
#include "tstlv.h"
#include "tsTime.h"
#include "tsSimulCryptDate.h"
#include "tsXMLTables.h"
#include "tsxmlComment.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TablesLogger::TablesLogger(const TablesLoggerArgs& opt, TablesDisplay& display, Report& report) :
    TableHandlerInterface(),
    SectionHandlerInterface(),
    _opt(opt),
    _display(display),
    _report(report),
    _abort(false),
    _exit(false),
    _table_count(0),
    _packet_count(0),
    _demux(0, 0, opt.pid),
    _cas_mapper(report),
    _xmlOut(report),
    _xmlDoc(report),
    _xmlOpen(false),
    _binfile(),
    _sock(false, report),
    _shortSections()
{
    // Set either a table or section handler, depending on --all-sections
    if (_opt.all_sections) {
        _demux.setSectionHandler(this);
    }
    else {
        _demux.setTableHandler(this);
    }

    // Open/create the text output.
    if (_opt.use_text && !_display.redirect(_opt.text_destination)) {
        _abort = true;
        return;
    }

    // Open/create the XML output.
    if (_opt.use_xml) {
        if (_opt.xml_destination.empty()) {
            // Use standard output.
            _xmlOut.setStream(std::cout);
        }
        else if (!_xmlOut.setFile(_opt.xml_destination)) {
            _abort = true;
            return;
        }
        // Initialize the XML document.
        _xmlDoc.initialize(u"tsduck");
    }

    // Open/create the binary output.
    if (_opt.use_binary) {
        if (!_opt.multi_files) {
            // Create one single binary file as output
            _report.verbose(u"Creating " + _opt.bin_destination);
            _binfile.open(_opt.bin_destination.toUTF8().c_str(), std::ios::out | std::ios::binary);
            if (!_binfile) {
                _report.error(u"cannot create %s", {_opt.bin_destination});
                _abort = true;
                return;
            }
        }
    }

    // Initialize UDP output.
    if (_opt.use_udp) {
        // Create UDP socket.
        _abort =
            !_sock.open(_report) ||
            !_sock.setDefaultDestination(_opt.udp_destination, _report) ||
            (!_opt.udp_local.empty() && !_sock.setOutgoingMulticast(_opt.udp_local, _report)) ||
            (_opt.udp_ttl > 0 && !_sock.setTTL(_opt.udp_ttl, _report));
        if (_abort) {
            _sock.close();
        }
    }
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::TablesLogger::~TablesLogger()
{
    // Close the XML document if needed.
    if (_xmlOpen) {
        _xmlDoc.printClose(_xmlOut);
        _xmlOpen = false;
    }

    // Other files and sockets are automatically closed by their destructors.
}


//----------------------------------------------------------------------------
// The following method feeds the logger with a TS packet.
//----------------------------------------------------------------------------

void ts::TablesLogger::feedPacket(const TSPacket& pkt)
{
    _demux.feedPacket(pkt);
    _cas_mapper.feedPacket(pkt);
    _packet_count++;
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void ts::TablesLogger::handleTable(SectionDemux&, const BinaryTable& table)
{
    // Give up if completed.
    if (completed()) {
        return;
    }

    assert(table.sectionCount() > 0);
    const PID pid = table.sourcePID();

    // Add PMT PID's when necessary
    if (_opt.add_pmt_pids && table.tableId() == TID_PAT) {
        PAT pat(table);
        if (pat.isValid()) {
            if (pat.nit_pid != PID_NULL) {
                _demux.addPID(pat.nit_pid);
            }
            for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                _demux.addPID(it->second);
            }
        }
    }

    // Ignore table if not to be filtered
    if (!isFiltered(*table.sectionAt(0), _cas_mapper.casFamily(table.sourcePID()))) {
        return;
    }

    // Ignore duplicate tables with a short section.
    if (_opt.no_duplicate && table.isShortSection()) {
        if (_shortSections[pid].isNull() || *_shortSections[pid] != *table.sectionAt(0)) {
            // Not the same section, keep it for next time.
            _shortSections[pid] = new Section(*table.sectionAt(0), COPY);
        }
        else {
            // Same section as previously, ignore it.
            return;
        }
    }

    // Filtering done, now save data.
    if (_opt.use_text) {
        preDisplay(table.getFirstTSPacketIndex(), table.getLastTSPacketIndex());
        if (_opt.logger) {
            // Short log message
            logSection(*table.sectionAt(0));
        }
        else {
            // Full table formatting
            _display.displayTable(table, 0, _cas_mapper.casFamily(pid)) << std::endl;
        }
        postDisplay();
    }

    if (_opt.use_xml) {
        // Convert the table into an XML structure.
        xml::Element* elem = table.toXML(_xmlDoc.rootElement(), false, _display.dvbCharset());
        if (elem != 0) {
            // Add an XML comment as first child of the table.
            new xml::Comment(elem,
                             UString::Format(u" PID 0x%X (%d), first TS packet: %'d, last: %'d ",
                                              {pid, pid, table.getFirstTSPacketIndex(), table.getLastTSPacketIndex()}),
                             false); // first position
            // Print the new table.
            if (_xmlOpen) {
                _xmlOut << ts::margin;
                elem->print(_xmlOut, false);
                _xmlOut << std::endl;
            }
            else {
                // If this is the first table, print the document header with it.
                _xmlOpen = true;
                _xmlDoc.print(_xmlOut, true);
            }
            // Now remove the table from the document. Keeping them would eat up memory for no use.
            // Deallocating the element forces the removal from the document through the destructor.
            delete elem;
        }
    }

    if (_opt.use_binary) {
        // Save each section in binary format
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            saveSection(*table.sectionAt(i));
        }
    }

    if (_opt.use_udp) {
        ByteBlock bb;
        // Minimize allocation by reserving over size
        bb.reserve(table.totalSize() + 32 + 4 * table.sectionCount());
        if (_opt.udp_raw) {
            // Add raw content of each section the message
            for (size_t i = 0; i < table.sectionCount(); ++i) {
                const Section& sect(*table.sectionAt(i));
                bb.append(sect.content(), sect.size());
            }
        }
        else {
            // Build a TLV message. Each section is a separate PRM_SECTION parameter.
            startMessage(bb, tlv::MSG_LOG_TABLE, pid);
            for (size_t i = 0; i < table.sectionCount(); ++i) {
                addSection(bb, *table.sectionAt(i));
            }
        }
        // Send TLV message over UDP
        _sock.send(bb.data(), bb.size(), _report);
    }

    // Check max table count
    _table_count++;
    if (_opt.max_tables > 0 && _table_count >= _opt.max_tables) {
        _exit = true;
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete section is available.
// Only used with option --all-sections
//----------------------------------------------------------------------------

void ts::TablesLogger::handleSection(SectionDemux&, const Section& sect)
{
    // Give up if completed.
    if (completed()) {
        return;
    }

    // Ignore section if not to be filtered
    if (!isFiltered(sect, _cas_mapper.casFamily(sect.sourcePID()))) {
        return;
    }

    // Filtering done, now save data.
    // Note that no XML can be produced since valid XML structures contain complete tables only.

    if (_opt.use_text) {
        preDisplay(sect.getFirstTSPacketIndex(), sect.getLastTSPacketIndex());
        if (_opt.logger) {
            // Short log message
            logSection(sect);
        }
        else {
            // Full section formatting.
            _display.displaySection(sect, 0, _cas_mapper.casFamily(sect.sourcePID())) << std::endl;
        }
        postDisplay();
    }

    if (_opt.use_udp) {
        // Save section in binary format
        saveSection(sect);
    }

    if (_opt.use_udp) {
        if (_opt.udp_raw) {
            // Send raw content of section as one single UDP message
            _sock.send(sect.content(), sect.size(), _report);
        }
        else {
            ByteBlock bb;
            // Minimize allocation by reserving over size
            bb.reserve(sect.size() + 32);
            // Build a TLV message with one PRM_SECTION parameter.
            startMessage(bb, tlv::MSG_LOG_SECTION, sect.sourcePID());
            addSection(bb, sect);
            // Send TLV message over UDP
            _sock.send(bb.data(), bb.size(), _report);
        }
    }

    // Check max table count (actually count sections with --all-sections)
    _table_count++;
    if (_opt.max_tables > 0 && _table_count >= _opt.max_tables) {
        _exit = true;
    }
}


//----------------------------------------------------------------------------
//  Save a section in a binary file
//----------------------------------------------------------------------------

void ts::TablesLogger::saveSection(const Section& sect)
{
    // Create individual file for this section if required.
    if (_opt.multi_files) {
        // Build a unique file name for this section
        UString outname(PathPrefix(_opt.bin_destination));
        outname += UString::Format(u"_p%04X_t%02X", {sect.sourcePID(), sect.tableId()});
        if (sect.isLongSection()) {
            outname += UString::Format(u"_e%04X_v%02X_s%02X", {sect.tableIdExtension(), sect.version(), sect.sectionNumber()});
        }
        outname += PathSuffix(_opt.bin_destination);
        // Create the output file
        _report.verbose(u"creating %s", {outname});
        _binfile.open(outname.toUTF8().c_str(), std::ios::out | std::ios::binary);
        if (!_binfile) {
            _report.error(u"error creating %s", {outname});
            _abort = true;
            return;
        }
    }

    // Write the section to the file
    if (!sect.write(_binfile, _report)) {
        _abort = true;
    }

    // Close individual files
    if (_opt.multi_files) {
        _binfile.close();
    }
}


//----------------------------------------------------------------------------
//  Log a table (option --log)
//----------------------------------------------------------------------------

void ts::TablesLogger::logSection(const Section& sect)
{
    UString header;

    // Display time stamp if required.
    if (_opt.time_stamp) {
        header += UString(Time::CurrentLocalTime());
        header += u": ";
    }

    // Display packet index if required.
    if (_opt.packet_index) {
        header += UString::Format(u"Packet %'d to %'d, ", {sect.getFirstTSPacketIndex(), sect.getLastTSPacketIndex()});
    }

    // Table identification.
    header += UString::Format(u"PID 0x%X, TID 0x%X", {sect.sourcePID(), sect.tableId()});
    if (sect.isLongSection()) {
        header += UString::Format(u", TIDext 0x%X, V%d", {sect.tableIdExtension(), sect.version()});
    }
    header += u": ";

    // Output the line through the display object.
    _display.logSectionData(sect, header, _opt.log_size, _cas_mapper.casFamily(sect.sourcePID()));
}


//----------------------------------------------------------------------------
//  Check if a specific section must be filtered
//----------------------------------------------------------------------------

bool ts::TablesLogger::isFiltered(const Section& sect, CASFamily cas) const
{
    const bool tid_set = _opt.tid.find(sect.tableId()) != _opt.tid.end();
    const bool tidext_set = _opt.tidext.find(sect.tableIdExtension()) != _opt.tidext.end();

    return
        // TID ok
        (_opt.tid.empty() || (tid_set && !_opt.negate_tid) || (!tid_set && _opt.negate_tid)) &&
        // TIDext ok
        (!sect.isLongSection() || _opt.tidext.empty() || (tidext_set && !_opt.negate_tidext) || (!tidext_set && _opt.negate_tidext)) &&
        // Diversified payload ok
        (!_opt.diversified || sect.hasDiversifiedPayload());
}


//----------------------------------------------------------------------------
//  Display header information, before a table
//----------------------------------------------------------------------------

void ts::TablesLogger::preDisplay(PacketCounter first, PacketCounter last)
{
    std::ostream& strm(_display.out());

    // Initial spacing
    if (_table_count == 0 && !_opt.logger) {
        strm << std::endl;
    }

    // Display time stamp if required
    if ((_opt.time_stamp || _opt.packet_index) && !_opt.logger) {
        strm << "* ";
        if (_opt.time_stamp) {
            strm << "At " << Time::CurrentLocalTime();
        }
        if (_opt.packet_index && _opt.time_stamp) {
            strm << ", ";
        }
        if (_opt.packet_index) {
            strm << UString::Format(u"First TS packet: %'d, last: %'d", {first, last});
        }
        strm << std::endl;
    }
}


//----------------------------------------------------------------------------
//  Post-display action
//----------------------------------------------------------------------------

void ts::TablesLogger::postDisplay()
{
    // Flush output file if required
    if (_opt.flush) {
        _display.flush();
    }
}


//----------------------------------------------------------------------------
//  Build header of a TLV message
//----------------------------------------------------------------------------

void ts::TablesLogger::startMessage(ByteBlock& bb, uint16_t message_type, PID pid)
{
    bb.clear();
    // Protocol version
    bb.appendUInt8(tlv::TS_PROTOCOL_VERSION);
    // Message type and length
    bb.appendUInt16(message_type);
    bb.appendUInt16(0); // message length placeholder
    // PID parameter
    bb.appendUInt16(tlv::PRM_PID);
    bb.appendUInt16(2);
    bb.appendUInt16(pid);
    // Timestamp parameter
    SimulCryptDate now(Time::CurrentLocalTime());
    bb.appendUInt16(tlv::PRM_TIMESTAMP);
    bb.appendUInt16(SimulCryptDate::SIZE);
    now.putBinary(bb.enlarge(SimulCryptDate::SIZE));
    // Update message length
    PutUInt16(&bb[3], uint16_t(bb.size() - 5));
}


//----------------------------------------------------------------------------
//  Add a section into a TLV message
//----------------------------------------------------------------------------

void ts::TablesLogger::addSection(ByteBlock& bb, const Section& sect)
{
    // Section parameter
    bb.appendUInt16(tlv::PRM_SECTION);
    bb.appendUInt16(uint16_t(sect.size()));
    bb.append(sect.content(), sect.size());
    // Update message length
    PutUInt16(&bb[3], uint16_t(bb.size() - 5));
}


//----------------------------------------------------------------------------
// Report the demux errors (if any)
//----------------------------------------------------------------------------

void ts::TablesLogger::reportDemuxErrors(std::ostream& strm)
{
    if (_demux.hasErrors()) {
        SectionDemux::Status status(_demux);
        strm << "* PSI/SI analysis errors:" << std::endl;
        status.display(strm, 4, true);
    }
}
