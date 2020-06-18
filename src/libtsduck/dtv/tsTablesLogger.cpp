//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsTablesLogger.h"
#include "tsTablesLoggerFilterRepository.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"
#include "tstlv.h"
#include "tsTime.h"
#include "tsDuckContext.h"
#include "tsSimulCryptDate.h"
#include "tsDuckProtocol.h"
#include "tsxmlComment.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr size_t ts::TablesLogger::DEFAULT_LOG_SIZE;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TablesLogger::TablesLogger(TablesDisplay& display) :
    ArgsSupplierInterface(),
    TableHandlerInterface(),
    SectionHandlerInterface(),
    _use_text(false),
    _use_xml(false),
    _use_binary(false),
    _use_udp(false),
    _text_destination(),
    _xml_destination(),
    _bin_destination(),
    _udp_destination(),
    _multi_files(false),
    _flush(false),
    _rewrite_xml(false),
    _rewrite_binary(false),
    _udp_local(),
    _udp_ttl(0),
    _udp_raw(false),
    _all_sections(false),
    _all_once(false),
    _max_tables(0),
    _time_stamp(false),
    _packet_index(false),
    _logger(false),
    _log_size(DEFAULT_LOG_SIZE),
    _no_duplicate(false),
    _pack_all_sections(false),
    _pack_and_flush(false),
    _fill_eit(false),
    _use_current(true),
    _use_next(false),
    _xml_tweaks(),
    _initial_pids(),
    _display(display),
    _duck(_display.duck()),
    _report(_duck.report()),
    _abort(false),
    _exit(false),
    _table_count(0),
    _packet_count(0),
    _demux(_duck),
    _cas_mapper(_duck),
    _xmlOut(_report),
    _xmlDoc(_report),
    _xmlOpen(false),
    _binfile(),
    _sock(false, _report),
    _shortSections(),
    _allSections(),
    _sectionsOnce(),
    _section_filters()
{
    // Create an instance of each registered section filter.
    TablesLoggerFilterRepository::Instance()->createFilters(_section_filters);
    _report.debug(u"TablesLogger has %s section filters", {_section_filters.size()});
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::TablesLogger::~TablesLogger()
{
    close();
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TablesLogger::defineArgs(Args& args) const
{
    // Define XML options.
    _xml_tweaks.defineArgs(args);

    // Define options from all section filters.
    for (auto it = _section_filters.begin(); it != _section_filters.end(); ++it) {
        (*it)->defineFilterOptions(args);
    }

    args.option(u"all-once");
    args.help(u"all-once",
              u"Same as --all-sections but collect each section only once per combination of "
              u"PID, table id, table id extension, section number and version.");

    args.option(u"all-sections", 'a');
    args.help(u"all-sections", u"Display/save all sections, as they appear in the stream. By default, "
              u"collect complete tables, with all sections of the tables grouped and "
              u"ordered and collect each version of a table only once. Note that this "
              u"mode is incompatible with --xml-output since valid XML structures may "
              u"contain complete tables only.");

    args.option(u"binary-output", 'b', Args::STRING);
    args.help(u"binary-output", u"filename",
              u"Save sections in the specified binary output file. "
              u"See also option -m, --multiple-files.");

    args.option(u"fill-eit");
    args.help(u"fill-eit",
              u"Before exiting, add missing empty sections in EIT's and flush them. "
              u"This can be useful with segmented EIT schedule where empty sections "
              u"at end of segments are usually not transmitted.");

    args.option(u"flush", 'f');
    args.help(u"flush", u"Flush output after each display.");

    args.option(u"exclude-current");
    args.help(u"exclude-current",
              u"Exclude short sections and long sections with \"current\" indicator. "
              u"This is rarely necessary. See also --include-next.");

    args.option(u"include-next");
    args.help(u"include-next",
              u"Include long sections with \"next\" indicator. By default, they are excluded.");

    args.option(u"ip-udp", 'i', Args::STRING);
    args.help(u"ip-udp", u"address:port",
              u"Send binary tables over UDP/IP to the specified destination. "
              u"The 'address' specifies an IP address which can be either unicast "
              u"or multicast. It can be also a host name that translates to an IP "
              u"address. The 'port' specifies the destination UDP port.");

    args.option(u"local-udp", 0, Args::STRING);
    args.help(u"local-udp", u"address",
              u"With --ip-udp, when the destination is a multicast address, specify "
              u"the IP address of the outgoing local interface. It can be also a host "
              u"name that translates to a local address.");

    args.option(u"log", 0);
    args.help(u"log", u"Display a short one-line log of each table instead of full table display.");

    args.option(u"log-size", 0, Args::UNSIGNED);
    args.help(u"log-size",
              u"With option --log, specify how many bytes are displayed at the "
              u"beginning of the table payload (the header is not displayed). "
              u"The default is 8 bytes.");

    args.option(u"max-tables", 'x', Args::POSITIVE);
    args.help(u"max-tables", u"Maximum number of tables to dump. Stop logging tables when this limit is reached.");

    args.option(u"multiple-files", 'm');
    args.help(u"multiple-files",
              u"Create multiple binary output files, one per section. A binary "
              u"output file name must be specified (option -b or --binary-output). "
              u"Assuming that the specified file name has the form 'base.ext', "
              u"each file is created with the name 'base_pXXXX_tXX.ext' for "
              u"short sections and 'base_pXXXX_tXX_eXXXX_vXX_sXX.ext' for long "
              u"sections, where the XX specify the hexadecimal values of the "
              u"PID, TID (table id), TIDext (table id extension), version and "
              u"section index.");

    args.option(u"no-duplicate");
    args.help(u"no-duplicate",
              u"Do not report consecutive identical tables with a short section in the "
              u"same PID. This can be useful for ECM's. This is the way to display new "
              u"ECM's only. By default, tables with long sections are reported only when "
              u"a new version is detected but tables with a short section are all reported.");

    args.option(u"no-encapsulation");
    args.help(u"no-encapsulation",
              u"With --ip-udp, send the tables as raw binary messages in UDP packets. "
              u"By default, the tables are formatted into TLV messages.");

    args.option(u"output-file", 'o', Args::STRING);
    args.help(u"output-file", u"",
              u"Save the tables or sections in human-readable text format in the specified "
              u"file. By default, when no output option is specified, text is produced on "
              u"the standard output. If you need text formatting on the standard output in "
              u"addition to other output like binary files or UPD/IP, explicitly specify "
              u"this option with \"-\" as output file name.\n\n"
              u"By default, the tables are interpreted and formatted as text on the standard "
              u"output. Several destinations can be specified at the same time: human-readable "
              u"text output, binary output, UDP/IP messages.");

    args.option(u"pack-all-sections");
    args.help(u"pack-all-sections",
              u"Same as --all-sections but also modify each long section so that it becomes a "
              u"valid complete table. Its section_number and last_section_number are forced "
              u"to zero. Use with care because this may create inconsistent tables. This "
              u"option can be useful with tables with sparse sections such as EIT's to save "
              u"them in XML format (as an alternative, see also --fill-eit).");

    args.option(u"pack-and-flush");
    args.help(u"pack-and-flush",
              u"Before exiting, pack incomplete tables, ignoring missing sections, and flush "
              u"them. Use with care because this may create inconsistent tables. Unlike option "
              u"--pack-all-sections, --pack-and-flush does not force --all-sections because it "
              u"only applies to the last incomplete tables before exiting.");

    args.option(u"packet-index");
    args.help(u"packet-index",
              u"Display the index of the first and last TS packet of each displayed "
              u"section or table.");

    args.option(u"rewrite-binary");
    args.help(u"rewrite-binary",
              u"With --binary-output, rewrite the same file with each table. "
              u"The specified file always contains one single table, the latest one.");

    args.option(u"rewrite-xml");
    args.help(u"rewrite-xml",
              u"With --xml-output, rewrite the same file with each table. "
              u"The specified file always contains one single table, the latest one.");

    args.option(u"text-output", 0, Args::STRING);
    args.help(u"text-output", u"A synonym for --output-file.");

    args.option(u"time-stamp");
    args.help(u"time-stamp", u"Display a time stamp (current local time) with each table.");

    args.option(u"ttl", 0, Args::POSITIVE);
    args.help(u"ttl",
              u"With --ip-udp, specifies the TTL (Time-To-Live) socket option. "
              u"The actual option is either \"Unicast TTL\" or \"Multicast TTL\", "
              u"depending on the destination address. Remember that the default "
              u"Multicast TTL is 1 on most systems.");

    args.option(u"xml-output", 0,  Args::STRING);
    args.help(u"xml-output", u"filename",
              u"Save the tables in XML format in the specified file. To output the XML "
              u"text on the standard output, explicitly specify this option with \"-\" "
              u"as output file name.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TablesLogger::loadArgs(DuckContext& duck, Args& args)
{
    // Type of output, text is the default.
    _use_xml = args.present(u"xml-output");
    _use_binary = args.present(u"binary-output");
    _use_udp = args.present(u"ip-udp");
    _use_text = args.present(u"output-file") || args.present(u"text-output") || ( !_use_xml && !_use_binary && !_use_udp);

    // --output-file and --text-output are synonyms.
    if (args.present(u"output-file") && args.present(u"text-output")) {
        args.error(u"--output-file and --text-output are synonyms, do not use both");
    }

    // Output destinations.
    _xml_destination = args.value(u"xml-output");
    _bin_destination = args.value(u"binary-output");
    _udp_destination = args.value(u"ip-udp");
    _text_destination = args.value(u"output-file", args.value(u"text-output").c_str());

    // Accept "-" as a specification for standard output (common convention in UNIX world).
    if (_text_destination == u"-") {
        _text_destination.clear();
    }
    if (_xml_destination == u"-") {
        _xml_destination.clear();
    }

    _multi_files = args.present(u"multiple-files");
    _rewrite_binary = args.present(u"rewrite-binary");
    _rewrite_xml = args.present(u"rewrite-xml");
    _flush = args.present(u"flush");
    _udp_local = args.value(u"local-udp");
    _udp_ttl = args.intValue(u"ttl", 0);
    _pack_all_sections = args.present(u"pack-all-sections");
    _pack_and_flush = args.present(u"pack-and-flush");
    _fill_eit = args.present(u"fill-eit");
    _all_once = args.present(u"all-once");
    _all_sections = _all_once || _pack_all_sections || args.present(u"all-sections");
    _max_tables = args.intValue<uint32_t>(u"max-tables", 0);
    _time_stamp = args.present(u"time-stamp");
    _packet_index = args.present(u"packet-index");
    _logger = args.present(u"log");
    _log_size = args.intValue<size_t>(u"log-size", DEFAULT_LOG_SIZE);
    _no_duplicate = args.present(u"no-duplicate");
    _udp_raw = args.present(u"no-encapsulation");
    _use_current = !args.present(u"exclude-current");
    _use_next = args.present(u"include-next");

    // Check consistency of options.
    if (_rewrite_binary && _multi_files) {
        args.error(u"options --rewrite-binary and --multiple-files are incompatible");
        return false;
    }

    // Load options from all section filters.
    _initial_pids.reset();
    for (auto it = _section_filters.begin(); it != _section_filters.end(); ++it) {
        PIDSet pids;
        if (!(*it)->loadFilterOptions(_duck, args, pids)) {
            return false;
        }
        _initial_pids |= pids;
    }

    // Load XML options.
    return _xml_tweaks.loadArgs(duck, args);
}


//----------------------------------------------------------------------------
// Open files, start operations.
//----------------------------------------------------------------------------

bool ts::TablesLogger::open()
{
    // Reinitialize working data.
    _abort = _exit = false;
    _table_count = 0;
    _packet_count = 0;
    _demux.reset();
    _cas_mapper.reset();
    _xmlOut.close();
    _xmlDoc.clear();
    _xmlOpen = false;
    _shortSections.clear();
    _allSections.clear();
    _sectionsOnce.clear();

    if (_binfile.is_open()) {
        _binfile.close();
    }
    if (_sock.isOpen()) {
        _sock.close(_report);
    }

    // Set PID's to filter.
    _demux.setPIDFilter(_initial_pids);

    // Set either a table or section handler, depending on --all-sections
    if (_all_sections) {
        _demux.setTableHandler(nullptr);
        _demux.setSectionHandler(this);
    }
    else {
        _demux.setTableHandler(this);
        _demux.setSectionHandler(nullptr);
    }

    // Type of sections to get.
    _demux.setCurrentNext(_use_current, _use_next);
    _cas_mapper.setCurrentNext(_use_current, _use_next);

    // Open/create the text output.
    if (_use_text && !_duck.setOutput(_text_destination)) {
        _abort = true;
        return false;
    }

    // Set XML options in document.
    _xmlDoc.setTweaks(_xml_tweaks);

    // Open/create the XML output.
    if (_use_xml && !_rewrite_xml && !createXML(_xml_destination)) {
        _abort = true;
        return false;
    }

    // Open/create the binary output.
    if (_use_binary && !_multi_files && !_rewrite_binary && !createBinaryFile(_bin_destination)) {
        _abort = true;
        return false;
    }

    // Initialize UDP output.
    if (_use_udp) {
        // Create UDP socket.
        _abort =
            !_sock.open(_report) ||
            !_sock.setDefaultDestination(_udp_destination, _report) ||
            (!_udp_local.empty() && !_sock.setOutgoingMulticast(_udp_local, _report)) ||
            (_udp_ttl > 0 && !_sock.setTTL(_udp_ttl, _report));
        if (_abort) {
            _sock.close(_report);
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Close all operations, flush tables if required, close files and sockets.
//----------------------------------------------------------------------------

void ts::TablesLogger::close()
{
    if (!_exit) {

        // Pack sections in incomplete tables if required.
        if (_pack_and_flush) {
            _demux.packAndFlushSections();
        }
        if (_fill_eit) {
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
    const uint16_t cas = _cas_mapper.casId(table.sourcePID());

    // Ignore table if not to be filtered. Keep the table if at least one section shall be kept.
    bool keep = false;
    for (size_t i = 0; !keep && i < table.sectionCount(); ++i) {
        keep = isFiltered(*table.sectionAt(i), cas);
    }
    if (!keep) {
        return;
    }

    // Ignore duplicate tables with a short section.
    if (_no_duplicate && table.isShortSection()) {
        if (_shortSections[pid].isNull() || *_shortSections[pid] != *table.sectionAt(0)) {
            // Not the same section, keep it for next time.
            _shortSections[pid] = new Section(*table.sectionAt(0), ShareMode::COPY);
        }
        else {
            // Same section as previously, ignore it.
            return;
        }
    }

    // Filtering done, now save data.
    if (_use_text) {
        preDisplay(table.getFirstTSPacketIndex(), table.getLastTSPacketIndex());
        if (_logger) {
            // Short log message
            logSection(*table.sectionAt(0));
        }
        else {
            // Full table formatting
            _display.displayTable(table, 0, _cas_mapper.casId(pid)) << std::endl;
        }
        postDisplay();
    }

    if (_use_xml) {
        // In case of rewrite for each table, create a new file.
        if (_rewrite_xml && !createXML(_xml_destination)) {
            return;
        }
        saveXML(table);
        if (_rewrite_xml) {
            closeXML();
        }
    }

    if (_use_binary) {
        // In case of rewrite for each table, create a new file.
        if (_rewrite_binary && !createBinaryFile(_bin_destination)) {
            return;
        }
        // Save each section in binary format
        for (size_t i = 0; i < table.sectionCount(); ++i) {
            saveBinarySection(*table.sectionAt(i));
        }
        if (_rewrite_binary) {
            _binfile.close();
        }
    }

    if (_use_udp) {
        sendUDP(table);
    }

    // Check max table count
    _table_count++;
    if (_max_tables > 0 && _table_count >= _max_tables) {
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
    const uint16_t cas = _cas_mapper.casId(sect.sourcePID());

    // With option --all-once, track duplicate PID/TID/TDIext/secnum/version.
    if (_all_once) {
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
    if (_pack_all_sections) {
        BinaryTable table;
        table.addSection(new Section(sect, ShareMode::SHARE));
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
    if (!isFiltered(sect, cas)) {
        return;
    }

    // Ignore duplicate sections.
    if (_no_duplicate) {
        if (_allSections[pid].isNull() || *_allSections[pid] != sect) {
            // Not the same section, keep it for next time.
            _allSections[pid] = new Section(sect, ShareMode::COPY);
        }
        else {
            // Same section as previously, ignore it.
            return;
        }
    }

    // Filtering done, now save data.
    // Note that no XML can be produced since valid XML structures contain complete tables only.

    if (_use_text) {
        preDisplay(sect.getFirstTSPacketIndex(), sect.getLastTSPacketIndex());
        if (_logger) {
            // Short log message
            logSection(sect);
        }
        else {
            // Full section formatting.
            _display.displaySection(sect, 0, _cas_mapper.casId(pid)) << std::endl;
        }
        postDisplay();
    }

    if (_use_binary) {
        // In case of rewrite for each section, create a new file.
        if (_rewrite_binary && !createBinaryFile(_bin_destination)) {
            return;
        }
        saveBinarySection(sect);
        if (_rewrite_binary) {
            _binfile.close();
        }
    }

    if (_use_udp) {
        sendUDP(sect);
    }

    // Check max table count (actually count sections with --all-sections)
    _table_count++;
    if (_max_tables > 0 && _table_count >= _max_tables) {
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

    if (_udp_raw) {
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
    if (_udp_raw) {
        // Send raw content of section as one single UDP message
        _sock.send(section.content(), section.size(), _report);
    }
    else {
        // Build a TLV message.
        duck::LogSection msg;
        msg.pid = section.sourcePID();
        msg.timestamp = SimulCryptDate(Time::CurrentLocalTime());
        msg.section = new Section(section, ShareMode::SHARE);

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
    if (_multi_files) {
        // Build a unique file name for this section
        UString outname(PathPrefix(_bin_destination));
        outname += UString::Format(u"_p%04X_t%02X", {sect.sourcePID(), sect.tableId()});
        if (sect.isLongSection()) {
            outname += UString::Format(u"_e%04X_v%02X_s%02X", {sect.tableIdExtension(), sect.version(), sect.sectionNumber()});
        }
        outname += PathSuffix(_bin_destination);
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
    if (_multi_files) {
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
    xml::Element* elem = table.toXML(_duck, _xmlDoc.rootElement(), false);
    if (elem == nullptr) {
        // XML conversion error, message already displayed.
        return;
    }

    // Add an XML comment as first child of the table.
    UString comment(UString::Format(u" PID 0x%X (%d)", {table.sourcePID(), table.sourcePID()}));
    if (_time_stamp) {
        comment += u", at " + UString(Time::CurrentLocalTime());
    }
    if (_packet_index) {
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
    if (_time_stamp) {
        header += UString(Time::CurrentLocalTime());
        header += u": ";
    }

    // Display packet index if required.
    if (_packet_index) {
        header += UString::Format(u"Packet %'d to %'d, ", {sect.getFirstTSPacketIndex(), sect.getLastTSPacketIndex()});
    }

    // Table identification.
    header += UString::Format(u"PID 0x%X, TID 0x%X", {sect.sourcePID(), sect.tableId()});
    if (sect.isLongSection()) {
        header += UString::Format(u", TIDext 0x%X, V%d, Sec %d/%d", {sect.tableIdExtension(), sect.version(), sect.sectionNumber(), sect.lastSectionNumber()});
    }
    header += u": ";

    // Output the line through the display object.
    _display.logSectionData(sect, header, _log_size, _cas_mapper.casId(sect.sourcePID()));
}


//----------------------------------------------------------------------------
//  Check if a specific section must be filtered
//----------------------------------------------------------------------------

bool ts::TablesLogger::isFiltered(const Section& sect, uint16_t cas)
{
    // By default, keep the section.
    bool status = true;

    // Call all section filters. Keep the section if all filters agree.
    // Make sure to call all filters, even after one returned false to collect additional PID's.
    for (auto it = _section_filters.begin(); it != _section_filters.end(); ++it) {
        PIDSet pids;
        if (!(*it)->filterSection(_duck, sect, cas, pids)) {
            status = false;
        }
        _demux.addPIDs(pids);
    }
    return status;
}


//----------------------------------------------------------------------------
//  Display header information, before a table
//----------------------------------------------------------------------------

void ts::TablesLogger::preDisplay(PacketCounter first, PacketCounter last)
{
    std::ostream& strm(_duck.out());

    // Initial spacing
    if (_table_count == 0 && !_logger) {
        strm << std::endl;
    }

    // Display time stamp if required
    if ((_time_stamp || _packet_index) && !_logger) {
        strm << "* ";
        if (_time_stamp) {
            strm << "At " << Time::CurrentLocalTime();
        }
        if (_packet_index && _time_stamp) {
            strm << ", ";
        }
        if (_packet_index) {
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
    if (_flush) {
        _duck.flush();
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
