//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  This class extract files from FLUTE streams in UDP datagrams.
//!
//----------------------------------------------------------------------------

#include "tsFluteDemux.h"
#include "tsFluteFDT.h"
#include "tsFlute.h"
#include "tsIPSocketAddress.h"
#include "tsIPPacket.h"
#include "tsLCTHeader.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::FluteDemux::FluteDemux(DuckContext& duck, FluteHandlerInterface* handler) :
    _duck(duck),
    _handler(handler)
{
}

ts::FluteDemux::~FluteDemux()
{
}


//----------------------------------------------------------------------------
// Reset the demux.
//----------------------------------------------------------------------------

bool ts::FluteDemux::reset(const FluteDemuxArgs& args)
{
    _args = args;
    _sessions.clear();

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

void ts::FluteDemux::FileContext::clear()
{
    processed = false;
    instance = 0xFFFFFFFF;
    transfer_length = current_length = 0;
    chunks.clear();
};


//----------------------------------------------------------------------------
// The following method feeds the demux with an IP packet.
//----------------------------------------------------------------------------

void ts::FluteDemux::feedPacket(const IPPacket& pkt)
{
    if (pkt.isUDP()) {
        feedPacket(pkt.source(), pkt.destination(), pkt.protocolData(), pkt.protocolDataSize());
    }
}


//----------------------------------------------------------------------------
// The following method feeds the demux with a UDP packet.
//----------------------------------------------------------------------------

void ts::FluteDemux::feedPacket(const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
{
    // Get LCT header.
    LCTHeader lct;
    if (!lct.deserialize(udp, udp_size)) {
        _report.error(u"invalid LCT header from %s", source);
        return;
    }

    // The FEC Encoding ID is stored in codepoint (RFC 3926, section 5.1).
    // We currently only support the default one, value 0.
    if (lct.codepoint != FEI_COMPACT_NOCODE) {
        _report.error(u"unsupported FEC Encoding ID %d from %s", lct.codepoint, source);
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
    if (lct.naci.valid && _handler != nullptr) {
        _handler->handleFluteNACI(*this, lct.naci);
    }

    // With empty payload, nothing more to do.
    if (udp_size == 0) {
        return;
    }

    // Get/create transport session and file.
    const FluteSessionId sid(source, destination, lct.tsi);
    SessionContext& session(_sessions[sid]);
    FileContext& file(session.files[lct.toi]);

    // If the file is the FDT of the session, it must have FDT and FTI headers.
    if (lct.toi == FLUTE_FDT_TOI) {
        if (!lct.fdt.valid) {
            _report.error(u"FDT in FLUTE packet without EXT_FDT header, %s", sid);
            return;
        }
        if (!lct.fti.valid) {
            _report.error(u"FDT in FLUTE packet without EXT_FTI header, %s", sid);
            return;
        }
        if (file.instance != lct.fdt.fdt_instance_id) {
            _report.debug(u"new FDT instance %n, %s", lct.fdt.fdt_instance_id, sid);
            file.clear();
            file.instance = lct.fdt.fdt_instance_id;
        }
    }

    // If the file was already processed, nothing more to do.
    if (file.processed) {
        return;
    }

    // Update/check transfer length coming from FTI header.
    if (lct.fti.valid && !updateFileSize(sid, session, lct.toi, file, lct.fti.transfer_length)) {
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
        _report.error(u"size of file chunk #%n changed in the middle of transmission, was %'d, now %'d, TOI %d, %s",
                      sym_index, syms[sym_index]->size(), udp_size, lct.toi, sid);
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

bool ts::FluteDemux::updateFileSize(const FluteSessionId& sid, SessionContext& session, uint64_t toi, FileContext& file, uint64_t file_size)
{
    // Unlikely case when the file size has changed.
    if (file.transfer_length > 0 && file.transfer_length != file_size) {
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

void ts::FluteDemux::processCompleteFile(const FluteSessionId& sid, SessionContext& session, uint64_t toi, FileContext& file)
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

        // Notify the application.
        if (_handler != nullptr) {
            _handler->handleFluteFile(*this, ff);
        }
    }

    // Now forget about this file.
    file.processed = true;
}


//----------------------------------------------------------------------------
// Process a File Delivery Table (FDT).
//----------------------------------------------------------------------------

void ts::FluteDemux::processFDT(SessionContext& session, const FluteFDT& fdt)
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
        FileContext& sf(session.files[f.toi]);
        sf.name = f.content_location;
        sf.type = f.content_type;
        updateFileSize(fdt.sessionId(), session, f.toi, sf, f.transfer_length);
    }

    // Notify the application.
    if (_handler != nullptr) {
        _handler->handleFluteFDT(*this, fdt);
    }

    // Process all complete files which were not processed yet because of an absence of FDT.
    for (auto& f : session.files) {
        if (f.first != FLUTE_FDT_TOI && !f.second.processed && f.second.transfer_length > 0 && f.second.current_length >= f.second.transfer_length) {
            processCompleteFile(fdt.sessionId(), session, f.first, f.second);
        }
    }
}


//----------------------------------------------------------------------------
// Get the current status of all file transfers.
//----------------------------------------------------------------------------

void ts::FluteDemux::getFilesStatus()
{
    if (_handler != nullptr) {
        for (const auto& sess : _sessions) {
            for (const auto& file : sess.second.files) {
                _handler->handleFluteStatus(*this,
                                            sess.first,
                                            file.second.name,
                                            file.second.type,
                                            file.first,
                                            file.second.transfer_length,
                                            file.second.processed ? file.second.transfer_length : file.second.current_length);
            }
        }
    }
}
