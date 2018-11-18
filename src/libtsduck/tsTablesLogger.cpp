//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsDuckProtocol.h"
#include "tsxmlComment.h"
#include "tsxmlElement.h"
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
    _demux(nullptr, nullptr, opt.pid),
    _cas_mapper(report),
    _xmlOut(report),
    _xmlDoc(report),
    _xmlOpen(false),
    _binfile(),
    _sock(false, report),
    _shortSections(),
    _allSections(),
    _sectionsOnce()
{
    // Set either a table or section handler, depending on --all-sections
    if (_opt.all_sections) {
        _demux.setSectionHandler(this);
    }
    else {
        _demux.setTableHandler(this);
    }

    // Type of sections to get.
    _demux.setCurrentNext(opt.use_current, opt.use_next);
    _cas_mapper.setCurrentNext(opt.use_current, opt.use_next);

    // Open/create the text output.
    if (_opt.use_text && !_display.redirect(_opt.text_destination)) {
        _abort = true;
        return;
    }

    // Set XML options in document.
    _xmlDoc.setTweaks(_opt.xml_tweaks);

    // Open/create the XML output.
    if (_opt.use_xml && !_opt.rewrite_xml && !createXML(_opt.xml_destination)) {
        return;
    }

    // Open/create the binary output.
    if (_opt.use_binary && !_opt.multi_files && !_opt.rewrite_binary && !createBinaryFile(_opt.bin_destination)) {
        return;
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
            _sock.close(_report);
        }
    }
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::TablesLogger::~TablesLogger()
{
    close();
}


//----------------------------------------------------------------------------
// Close all operations, flush tables if required, close files and sockets.
//----------------------------------------------------------------------------

void ts::TablesLogger::close()
{
    if (!_exit) {

        // Pack sections in incomplete tables if required.
        if (_opt.pack_and_flush) {
            _demux.packAndFlushSections();
        }
        if (_opt.fill_eit) {
            _demux.fillAndFlushEITs();
        }

        // Close files and documents.
        closeXML();
        if (_binfile.is_open()) {
            _binfile.close();
        }
        if (_sock.isOpen()) {
            _sock.close(_report);
        }

        // Now completed.
        _exit = true;
    }
}


//----------------------------------------------------------------------------
// The following method feeds the logger with a TS packet.
//----------------------------------------------------------------------------

void ts::TablesLogger::feedPacket(const TSPacket& pkt)
{
    if (!completed()) {
        _demux.feedPacket(pkt);
        _cas_mapper.feedPacket(pkt);
        _packet_count++;
    }
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
        // In case of rewrite for each table, create a new file.
        if (_opt.rewrite_xml && !createXML(_opt.xml_destination)) {
            return;
        }
        saveXML(table);
        if (_opt.rewrite_xml) {
            closeXML();
        }
    }

    if (_opt.use_binary) {
        // In case of rewrite for each table, create a new file.
        if (_opt.rewrite_binary && !createBinaryFile(_opt.bin_destination)) {
            return;
        }
        // Save each section in binary format
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            saveBinarySection(*table.sectionAt(i));
        }
        if (_opt.rewrite_binary) {
            _binfile.close();
        }
    }

    if (_opt.use_udp) {
        sendUDP(table);
    }

    // Check max table count
    _table_count++;
    if (_opt.max_tables > 0 && _table_count >= _opt.max_tables) {
        _abort = true;
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete section is available.
// Only used with option --all-sections
//----------------------------------------------------------------------------

void ts::TablesLogger::handleSection(SectionDemux& demux, const Section& sect)
{
    const PID pid = sect.sourcePID();

    // With option --all-once, track duplicate PID/TID/TDIext/secnum/version.
    if (_opt.all_once) {
        // Pack PID/TID/TDIext/secnum/version into one single 64-bit integer.
        const uint64_t id =
            (uint64_t(pid) << 40) |
            (uint64_t(sect.tableId()) << 32) |
            (uint64_t(sect.tableIdExtension()) << 16) |
            (uint64_t(sect.sectionNumber()) << 8) |
            uint64_t(sect.version());
        if (_sectionsOnce.count(id) != 0) {
            // Already found this one, give up.
            return;
        }
        else {
            // Remember this combination.
            _sectionsOnce.insert(id);
        }
    }

    // With option --pack-all-sections, force the processing of a complete table.
    if (_opt.pack_all_sections) {
        BinaryTable table;
        table.addSection(new Section(sect, SHARE));
        table.packSections();
        if (table.isValid()) {
            handleTable(demux, table);
        }
        return;
    }

    // Give up if completed.
    if (completed()) {
        return;
    }

    // Ignore section if not to be filtered
    if (!isFiltered(sect, _cas_mapper.casFamily(pid))) {
        return;
    }

    // Ignore duplicate sections.
    if (_opt.no_duplicate) {
        if (_allSections[pid].isNull() || *_allSections[pid] != sect) {
            // Not the same section, keep it for next time.
            _allSections[pid] = new Section(sect, COPY);
        }
        else {
            // Same section as previously, ignore it.
            return;
        }
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
            _display.displaySection(sect, 0, _cas_mapper.casFamily(pid)) << std::endl;
        }
        postDisplay();
    }

    if (_opt.use_binary) {
        // In case of rewrite for each section, create a new file.
        if (_opt.rewrite_binary && !createBinaryFile(_opt.bin_destination)) {
            return;
        }
        saveBinarySection(sect);
        if (_opt.rewrite_binary) {
            _binfile.close();
        }
    }

    if (_opt.use_udp) {
        sendUDP(sect);
    }

    // Check max table count (actually count sections with --all-sections)
    _table_count++;
    if (_opt.max_tables > 0 && _table_count >= _opt.max_tables) {
        _abort = true;
    }
}


//----------------------------------------------------------------------------
// Send UDP table and section.
//----------------------------------------------------------------------------

void ts::TablesLogger::sendUDP(const ts::BinaryTable& table)
{
    ByteBlockPtr bin(new ByteBlock);

    // Minimize allocation by reserving over size
    bin->reserve(table.totalSize() + 32 + 4 * table.sectionCount());

    if (_opt.udp_raw) {
        // Add raw content of each section the message
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            const Section& sect(*table.sectionAt(i));
            bin->append(sect.content(), sect.size());
        }
    }
    else {
        // Build a TLV message.
        duck::LogTable msg;
        msg.pid = table.sourcePID();
        msg.timestamp = SimulCryptDate(Time::CurrentLocalTime());
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            msg.sections.push_back(table.sectionAt(i));
        }
        tlv::Serializer serial(bin);
        msg.serialize(serial);
    }

    // Send TLV message over UDP
    _sock.send(bin->data(), bin->size(), _report);
}

void ts::TablesLogger::sendUDP(const ts::Section& section)
{
    if (_opt.udp_raw) {
        // Send raw content of section as one single UDP message
        _sock.send(section.content(), section.size(), _report);
    }
    else {
        // Build a TLV message.
        duck::LogSection msg;
        msg.pid = section.sourcePID();
        msg.timestamp = SimulCryptDate(Time::CurrentLocalTime());
        msg.section = new Section(section, SHARE);

        // Serialize the message.
        ByteBlockPtr bin(new ByteBlock);
        tlv::Serializer serial(bin);
        msg.serialize(serial);

        // Send TLV message over UDP
        _sock.send(bin->data(), bin->size(), _report);
    }
}


//----------------------------------------------------------------------------
// Static routine to analyze UDP messages as sent with option --ip-udp.
//----------------------------------------------------------------------------

bool ts::TablesLogger::AnalyzeUDPMessage(const uint8_t* data, size_t size, bool no_encapsulation, SectionPtrVector& sections, Time& timestamp)
{
    // Clear output parameters.
    sections.clear();
    timestamp = Time::Epoch;

    // Filter invalid parameters.
    if (data == nullptr) {
        return false;
    }

    Variable<SimulCryptDate> scDate;
    Variable<PID> pid;

    if (no_encapsulation) {

        // Raw sections in UDP packets.
        // Loop on sections in the packet.
        while (size > 0) {
            const size_t sect_size = Section::SectionSize(data, size);
            assert(sect_size <= size);
            if (sect_size == 0) {
                return false;
            }
            const SectionPtr section(new Section(data, sect_size, ts::PID_NULL, ts::CRC32::CHECK));
            if (!section->isValid()) {
                return false;
            }
            sections.push_back(section);
            data += sect_size;
            size -= sect_size;
        }
    }
    else {
        // TLV messages in UDP packets. Decode the message.
        tlv::MessageFactory mf(data, size, duck::Protocol::Instance());
        tlv::MessagePtr msg(mf.factory());

        // We expected only two possible messages:
        const duck::LogSection* logSection = dynamic_cast<const duck::LogSection*>(msg.pointer());
        const duck::LogTable* logTable = dynamic_cast<const duck::LogTable*>(msg.pointer());

        if (logSection != nullptr) {
            scDate = logSection->timestamp;
            pid = logSection->pid;
            if (logSection->section.isNull() || !logSection->section->isValid()) {
                return false;
            }
            else {
                sections.push_back(logSection->section);
            }
        }
        else if (logTable != nullptr) {
            scDate = logTable->timestamp;
            pid = logTable->pid;
            sections = logTable->sections;
        }
        else {
            return false;
        }
    }

    // Set the PID in all sections.
    if (pid.set()) {
        for (SectionPtrVector::const_iterator it = sections.begin(); it != sections.end(); ++it) {
            if (!it->isNull()) {
                (*it)->setSourcePID(pid.value());
            }
        }
    }

    // Interpret the timestamp.
    if (scDate.set()) {
        try {
            timestamp = Time(scDate.value());
        }
        catch (...) {
            timestamp = Time::Epoch;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Create a binary file. On error, set _abort and return false.
//----------------------------------------------------------------------------

bool ts::TablesLogger::createBinaryFile(const ts::UString& name)
{
    _report.verbose(u"creating %s", {name});
    _binfile.open(name.toUTF8().c_str(), std::ios::out | std::ios::binary);

    if (_binfile) {
        return true;
    }
    else {
        _report.error(u"error creating %s", {name});
        _abort = true;
        return false;
    }
}


//----------------------------------------------------------------------------
//  Save a section in a binary file
//----------------------------------------------------------------------------

void ts::TablesLogger::saveBinarySection(const Section& sect)
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
        if (!createBinaryFile(outname)) {
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
// Open/write/close XML file.
//----------------------------------------------------------------------------

bool ts::TablesLogger::createXML(const ts::UString& name)
{
    if (name.empty()) {
        // Use standard output.
        _xmlOut.setStream(std::cout);
    }
    else if (!_xmlOut.setFile(name)) {
        _abort = true;
        return false;
    }

    // Initialize the XML document.
    _xmlDoc.initialize(u"tsduck");
    return true;
}

void ts::TablesLogger::saveXML(const ts::BinaryTable& table)
{
    // Convert the table into an XML structure.
    xml::Element* elem = table.toXML(_xmlDoc.rootElement(), false, _display.dvbCharset());
    if (elem == nullptr) {
        // XML conversion error, message already displayed.
        return;
    }

    // Add an XML comment as first child of the table.
    UString comment(UString::Format(u" PID 0x%X (%d)", {table.sourcePID(), table.sourcePID()}));
    if (_opt.time_stamp) {
        comment += u", at " + UString(Time::CurrentLocalTime());
    }
    if (_opt.packet_index) {
        comment += UString::Format(u", first TS packet: %'d, last: %'d", {table.getFirstTSPacketIndex(), table.getLastTSPacketIndex()});
    }
    new xml::Comment(elem, comment + u" ", false); // first position

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

void ts::TablesLogger::closeXML()
{
    if (_xmlOpen) {
        _xmlDoc.printClose(_xmlOut);
        _xmlOpen = false;
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
