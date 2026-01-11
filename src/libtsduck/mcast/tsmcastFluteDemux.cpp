//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFluteDemux.h"
#include "tsmcastLCTHeader.h"
#include "tsmcast.h"
#include "tsTextTable.h"
#include "tsIPSocketAddress.h"
#include "tsEnvironment.h"

// The garbage collector configuration can be initialized using environment variables.
// The environment variables for the default values are in seconds.
namespace {
    inline cn::microseconds InitGC(const ts::UChar* name, cn::seconds::rep defvalue)
    {
        return cn::duration_cast<cn::microseconds>(cn::seconds(ts::GetIntEnvironment<cn::seconds::rep>(name, defvalue)));
    }
}
const cn::microseconds ts::mcast::FluteDemux::_gc_interval       = InitGC(u"TS_FLUTE_GC_INTERVAL", 5);
const cn::microseconds ts::mcast::FluteDemux::_file_max_lifetime = InitGC(u"TS_FLUTE_GC_MAX_LIFE", 30);


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::mcast::FluteDemux::FluteDemux(DuckContext& duck, FluteHandlerInterface* handler) :
    _duck(duck),
    _handler(handler)
{
}

ts::mcast::FluteDemux::~FluteDemux()
{
}


//----------------------------------------------------------------------------
// Reset the demux.
//----------------------------------------------------------------------------

bool ts::mcast::FluteDemux::reset(const FluteDemuxArgs& args, bool will_get_files_status)
{
    _args = args;
    _sessions.clear();
    _keep_file_status = will_get_files_status;
    _packet_count = 0;
    _next_gc_timestamp = cn::microseconds::zero();

    // Check that the output directory exists for extracted files.
    if (!_args.output_directory.empty() && !fs::is_directory(_args.output_directory)) {
        _report.error(u"directory not found: %s", _args.output_directory);
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// FileContext: Description of a file being received.
//----------------------------------------------------------------------------

void ts::mcast::FluteDemux::FileContext::clear()
{
    processed = false;
    instance = 0xFFFFFFFF;
    transfer_length = current_length = 0;
    chunks.clear();
};


//----------------------------------------------------------------------------
// The following method feeds the demux with a UDP packet.
//----------------------------------------------------------------------------

void ts::mcast::FluteDemux::feedPacketImpl(const cn::microseconds& timestamp, const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
{
    // Count IP packets.
    if (_packet_count++ == 0) {
        // First packet, initialize the garbage collector time.
        _next_gc_timestamp = timestamp + _gc_interval;
        _report.debug(u"FluteDemux garbage collector every %d seconds, max file life time: %d seconds",
                      cn::duration_cast<cn::seconds>(_gc_interval).count(),
                      cn::duration_cast<cn::seconds>(_file_max_lifetime).count());
    }
    else if (timestamp >= _next_gc_timestamp) {
        // Time to collect the garbage.
        garbageCollector(timestamp);
        _next_gc_timestamp += _gc_interval;
    }

    // Get LCT header.
    LCTHeader lct;
    if (!lct.deserialize(udp, udp_size, FT_FLUTE)) {
        _report.error(u"invalid LCT header from %s to %s", source, destination);
        return;
    }

    // We currently only support the default FEC Encoding ID, value 0.
    if (lct.fec_encoding_id != FEI_COMPACT_NOCODE) {
        _report.error(u"unsupported FEC Encoding ID %d from %s", lct.fec_encoding_id, source);
        return;
    }

    // Log message for the packet.
    if (_args.log_flute_packets) {
        UString line;
        line.format(u"source: %s, destination: %s\n"
                    u"    %s\n"
                    u"    payload: %d bytes",
                    source, destination, lct, udp_size);
        if (_args.dump_flute_payload && udp_size > 0) {
            line += u'\n';
            line.appendDump(udp, udp_size, UString::ASCII | UString::HEXA | UString::BPL, 4, 16);
            line.trim(false, true);
        }
        _report.info(line);
    }

    // Notify NIPActualCarrierInformation.
    if (lct.naci.has_value() && _handler != nullptr) {
        _handler->handleFluteNACI(lct.naci.value());
    }

    // With empty payload, nothing more to do.
    if (udp_size == 0) {
        return;
    }

    // Get/create transport session and file.
    const FluteSessionId sid(source, destination, lct.tsi);
    SessionContext& session(_sessions[sid]);
    FileContext& file(session.files_by_toi[lct.toi]);

    // Keep track of last packet time, for the garbage collector.
    file.last_time = timestamp;

    // If the file is the FDT of the session, it must have FDT and FTI headers.
    if (lct.toi == FLUTE_FDT_TOI) {
        if (!lct.fdt.has_value()) {
            _report.error(u"FDT in FLUTE packet without EXT_FDT header, %s", sid);
            return;
        }
        if (!lct.fti.has_value()) {
            _report.error(u"FDT in FLUTE packet without EXT_FTI header, %s", sid);
            return;
        }
        if (file.instance != lct.fdt->fdt_instance_id) {
            _report.log(2, u"new FDT instance %n, %s", lct.fdt->fdt_instance_id, sid);
            file.clear();
            file.instance = lct.fdt->fdt_instance_id;
        }
    }

    // If the file was already processed, nothing more to do.
    if (file.processed) {
        return;
    }

    // Update/check transfer length coming from FTI or TOL header.
    std::optional<uint64_t> transfer_length;
    if (lct.tol.has_value()) {
        transfer_length = lct.tol.value();  // Typically ROUTE.
    }
    else if (lct.fti.has_value()) {
        transfer_length = lct.fti->transfer_length;  // FLUTE or ROUTE.
    }
    if (transfer_length.has_value() && !updateFileSize(sid, session, lct.toi, file, transfer_length.value())) {
        // File too large, ignored.
        return;
    }

    // Check the FEC payload ID.
    if (!lct.fpi.valid) {
        _report.error(u"FEC payload ID not found in FLUTE packet, %s", sid);
        return;
    }

    // Store the file chunk if not already there.
    if (lct.fpi.source_block_number >= file.chunks.size()) {
        file.chunks.resize(lct.fpi.source_block_number + 1);
    }
    auto& syms(file.chunks[lct.fpi.source_block_number]);
    const size_t sym_index = lct.fpi.encoding_symbol_id;
    if (sym_index >= syms.size()) {
        syms.resize(sym_index + 1);
    }
    if (syms[sym_index] == nullptr) {
        // New chunk.
        syms[sym_index] = std::make_shared<ByteBlock>(udp, udp_size);
        file.current_length += udp_size;
    }
    else if (udp_size != syms[sym_index]->size()) {
        // Chunk already there with a different size.
        // Tolerate new size = 0 in non-strict mode.
        if (udp_size > 0 || _args.strict) {
            _report.error(u"size of file chunk #%n changed in the middle of transmission, was %'d, now %'d, TOI %d, %s",
                          sym_index, syms[sym_index]->size(), udp_size, lct.toi, sid);
        }
        return;
    }

    // If file is complete (and its size is known), process the file.
    // Do not process files before receiving the FDT, if the file name is empty.
    if (file.transfer_length > 0 &&
        file.current_length >= file.transfer_length &&
        (lct.toi == FLUTE_FDT_TOI || !file.name.empty() || session.fdt_instance.has_value()))
    {
        processCompleteFile(sid, session, lct.toi, file);
    }
}


//----------------------------------------------------------------------------
// Update the announced length of a file.
//----------------------------------------------------------------------------

bool ts::mcast::FluteDemux::updateFileSize(const FluteSessionId& sid, SessionContext& session, uint64_t toi, FileContext& file, uint64_t file_size)
{
    // Unlikely case when the file size has changed. Tolerate new size = 0 in non-strict mode.
    if (file.transfer_length > 0 && file.transfer_length != file_size && (file_size > 0 || _args.strict)) {
        _report.error(u"file transfer length changed in the middle of transmission, was %'d, now %'d, TOI %d, %s", file.transfer_length, file_size, toi, sid);
    }

    file.transfer_length = file_size;

    if (_args.max_file_size > 0 && file_size > _args.max_file_size) {
        _report.verbose(u"ignoring file from %s, TOI: %d, too large: %'d bytes", sid, toi, file_size);
        // Mark the file as processed (ignored in the future). Deallocate everything.
        file.processed = true;
        file.chunks.clear();
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Process a complete file.
//----------------------------------------------------------------------------

void ts::mcast::FluteDemux::processCompleteFile(const FluteSessionId& sid, SessionContext& session, uint64_t toi, FileContext& file)
{
    // Rebuild the content of the file.
    ByteBlockPtr data(std::make_shared<ByteBlock>(file.transfer_length));
    size_t next_index = 0;
    for (auto& src : file.chunks) {
        for (auto& sym : src) {
            if (sym != nullptr) {
                if (next_index + sym->size() > data->size()) {
                    // Should not happen, but let's be conservative.
                    _report.debug(u"need to increase size of file buffer, was %'d, now %'d, TOI %d, %s", data->size(), next_index + sym->size(), toi, sid);
                    data->resize(next_index + sym->size());
                }
                MemCopy(data->data() + next_index, sym->data(), sym->size());
                next_index += sym->size();
            }
        }
    }

    // Deallocate chunks after rebuilding the file.
    file.chunks.clear();

    // Shrink rebuilt file data if necessary.
    data->resize(next_index);

    // Important: we currently support FEC Encoding ID zero, meaning no encoding,
    // therefore the raw transport data are identical to the file content.

    if (toi == FLUTE_FDT_TOI) {
        // Process a new FDT.
        const FluteFDT fdt(_report, sid, file.instance, data);
        if (fdt.isValid()) {
            processFDT(session, fdt);
        }
    }
    else {
        // Process a normal file.
        const FluteFile ff(sid, toi, file.name, file.type, data);
        const bool is_xml = file.type.contains(u"xml");

        // Log a description of the file when requested.
        if (_args.log_files || (is_xml && _args.dump_xml_files)) {
            UString line;
            line.format(u"received file \"%s\" (%'d bytes)\n    type: %s\n    %s, TOI: %d", file.name, ff.size(), file.type, sid, toi);

            // Dump XML content when requested.
            if (is_xml && _args.dump_xml_files) {
                line += u"\n    XML content:\n";
                line += ff.toXML();
            }
            _report.info(line);
        }

        // Check if the file shall be extracted.
        if (sid.match(_args.extract_session)) {
            for (const auto& n : _args.extract_files) {
                if (file.name.similar(n)) {
                    // Find last part of the name.
                    size_t start = file.name.find_last_of(u"/:");
                    start = start >= file.name.length() ? 0 : start + 1;
                    // Build output path.
                    fs::path out(_args.output_directory);
                    if (out.empty()) {
                        out = file.name.substr(start);
                    }
                    else {
                        out /= file.name.substr(start);
                    }
                    // Many reference XML files do not have extension.
                    if (is_xml && !file.name.ends_with(u".xml", CASE_INSENSITIVE)) {
                        out += u".xml";
                    }
                    // Save the file.
                    _report.verbose(u"extracting %s", out);
                    data->saveToFile(out, &_report);
                    break;
                }
            }
        }

        // Keep the status of all received files if necessary.
        if (_keep_file_status) {
            auto& f(session.files_by_name[file.name]);
            f.type = file.type;
            f.size = f.received = data->size();
            f.last_toi = toi;
            CleanupFileStatus(f);
        }

        // Notify the application.
        if (_handler != nullptr) {
            _handler->handleFluteFile(ff);
        }
    }

    // Now forget about this file.
    file.processed = true;
}


//----------------------------------------------------------------------------
// Process a File Delivery Table (FDT).
//----------------------------------------------------------------------------

void ts::mcast::FluteDemux::processFDT(SessionContext& session, const FluteFDT& fdt)
{
    // Remember last valid FDT instance.
    session.fdt_instance = fdt.instance_id;

    // Log the content of the FDT.
    if (_args.log_fdt) {
        UString line;
        line.format(u"FDT instance: %d, %s, %d files, expires: %s", fdt.instance_id, fdt.sessionId(), fdt.files.size(), fdt.expires);
        for (const auto& f : fdt.files) {
            line.format(u"\n    TOI: %d, name: %s, %'d bytes, type: %s", f.toi, f.content_location, f.content_length, f.content_type);
        }
        _report.info(line);
    }

    // Save the content of the FDT.
    if (!_args.save_fdt.empty()) {
        // Build the path with instance value.
        fs::path path(_args.save_fdt);
        if (path != u"-") {
            path.replace_extension();
            path += UString::Format(u"-%d", fdt.instance_id);
            path += _args.save_fdt.extension();
        }

        // Save the file.
        _report.debug(u"saving %s", path);
        if (!fdt.toXML().save(path, false, true)) {
            _report.error(u"error creating file %s", path);
        }
    }

    // Register information for other files in the session, as described in the FDT.
    for (const auto& f : fdt.files) {
        FileContext& sf(session.files_by_toi[f.toi]);
        sf.name = f.content_location;
        sf.type = f.content_type;
        updateFileSize(fdt.sessionId(), session, f.toi, sf, f.transfer_length);
    }

    // Notify the application.
    if (_handler != nullptr) {
        _handler->handleFluteFDT(fdt);
    }

    // Process all complete files which were not processed yet because of an absence of FDT.
    for (auto& f : session.files_by_toi) {
        if (f.first != FLUTE_FDT_TOI && !f.second.processed && f.second.transfer_length > 0 && f.second.current_length >= f.second.transfer_length) {
            processCompleteFile(fdt.sessionId(), session, f.first, f.second);
        }
    }
}


//----------------------------------------------------------------------------
// Get the current status of all file transfers.
//----------------------------------------------------------------------------

void ts::mcast::FluteDemux::getFilesStatus(SessionStatus& status) const
{
    status.clear();
    for (const auto& sess : _sessions) {
        // In each session, start with a copy of all received files.
        auto& st_sess(status[sess.first]);
        st_sess = sess.second.files_by_name;

        // Then, add all partially transfered files.
        for (const auto& file : sess.second.files_by_toi) {
            // Process unprocessed files which are not an FDT (TOI = 0).
            if (!file.second.processed && file.first != FLUTE_FDT_TOI) {
                // If the file name is not yet known, build a dummy one.
                UString fname(file.second.name);
                if (fname.empty()) {
                    fname.format(u"(unknown, TOI %d)", file.first);
                }
                // Update the file status.
                auto& st_file(st_sess[fname]);
                st_file.last_toi = file.first;
                st_file.received = file.second.current_length;
                st_file.size = file.second.transfer_length;
                st_file.type = file.second.type;
                CleanupFileStatus(st_file);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Cleanup a FileStatus.
//----------------------------------------------------------------------------

void ts::mcast::FluteDemux::CleanupFileStatus(FileStatus& file)
{
    // Remove qualification such as "charset=utf-8" in type.
    const size_t sc = file.type.find(u";");
    if (sc < file.type.length()) {
        file.type.resize(sc);
    }
}


//----------------------------------------------------------------------------
// Print a list of all received files.
//----------------------------------------------------------------------------

void ts::mcast::FluteDemux::printFilesStatus(std::ostream& out) const
{
    // Collect all files status.
    SessionStatus status;
    getFilesStatus(status);

    // Display the status of all files.
    size_t session_count = 0;
    for (const auto& sess : status) {
        out << "Session #" << (++session_count) << ": " << sess.first << std::endl;
        if (sess.second.empty()) {
            out << "  No file received" << std::endl;
        }
        else {
            TextTable tab;
            enum {SIZE, TOI, STATUS, NAME, TYPE};
            tab.addColumn(SIZE, u"Size", TextTable::Align::RIGHT);
            tab.addColumn(TOI, u"TOI", TextTable::Align::RIGHT);
            tab.addColumn(STATUS, u"Status", TextTable::Align::RIGHT);
            tab.addColumn(NAME, u"Name", TextTable::Align::LEFT);
            tab.addColumn(TYPE, u"Type", TextTable::Align::LEFT);
            for (const auto& file : sess.second) {
                tab.setCell(SIZE, UString::Decimal(file.second.size));
                tab.setCell(TOI, UString::Decimal(file.second.last_toi));
                tab.setCell(STATUS, file.second.received == file.second.size && file.second.size > 0 ? u"complete" : UString::Decimal(file.second.received));
                tab.setCell(NAME, file.first);
                tab.setCell(TYPE, file.second.type);
                tab.newLine();
            }
            tab.output(out, TextTable::Headers::TEXT, true, u"  ", u"  ");
        }
        out << std::endl;
    }
}


//----------------------------------------------------------------------------
// Execute the garbage collector.
//----------------------------------------------------------------------------

void ts::mcast::FluteDemux::garbageCollector(const cn::microseconds& current_timestamp)
{
    _report.debug(u"FluteDemux garbage collector started");
    size_t reclaimed = 0;
    size_t kept = 0;
    for (auto& sess : _sessions) {
        for (auto file = sess.second.files_by_toi.begin(); file != sess.second.files_by_toi.end(); ) {
            if (file->second.last_time + _file_max_lifetime >= current_timestamp) {
                ++reclaimed;
                file = sess.second.files_by_toi.erase(file);
            }
            else {
                ++kept;
                ++file;
            }
        }
    }
    _report.debug(u"FluteDemux garbage collector complete, %'d files reclaimed, %'d kept", reclaimed, kept);
}
