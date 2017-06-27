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
#include "tsFormat.h"
#include "tsDecimal.h"
#include "tsHexa.h"



//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TablesLogger::TablesLogger (TablesLoggerOptions& opt) :
    _opt (opt),
    _abort (false),
    _exit (false),
    _table_count (0),
    _packet_count (0),
    _demux (0, 0, opt.pid),
    _outfile (),
    _out (opt.useCout() ? std::cout : _outfile),
    _sock (false, opt)
{
    // Set either a table or section handler, depending on --all-sections
    if (_opt.all_sections) {
        _demux.setSectionHandler (this);
    }
    else {
        _demux.setTableHandler (this);
    }

    // Open/create the destination
    switch (_opt.mode) {

        case TablesLoggerOptions::TEXT: {
            if (!_opt.destination.empty()) {
                // Open a text file as output (default: standard output)
                _opt.verbose ("creating " + _opt.destination);
                _outfile.open (_opt.destination.c_str(), std::ios::out);
                if (!_outfile) {
                    _opt.error ("cannot create " + _opt.destination);
                    _abort = true;
                    return;
                }
            }
            break;
        }

        case TablesLoggerOptions::BINARY: {
            if (!_opt.multi_files) {
                // Create one single binary file as output
                _opt.verbose ("Creating " + _opt.destination);
                _outfile.open (_opt.destination.c_str(), std::ios::out | std::ios::binary);
                if (!_outfile) {
                    _opt.error ("cannot create " + _opt.destination);
                    _abort = true;
                    return;
                }
            }
            break;
        }

        case TablesLoggerOptions::UDP: {
            // Create UDP socket.
            _abort =
                !_sock.open (_opt) ||
                !_sock.setDefaultDestination (_opt.destination, _opt) ||
                (!_opt.udp_local.empty() && !_sock.setOutgoingMulticast (_opt.udp_local, _opt)) ||
                (_opt.udp_ttl > 0 && !_sock.setTTL (_opt.udp_ttl, _opt));
            if (_abort) {
                _sock.close();
            }
            break;
        }

        default: {
            // Should never get there
            assert (false);
        }
    }
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::TablesLogger::~TablesLogger()
{
    // Files and sockets are automatically closed by their destructors.
}


//----------------------------------------------------------------------------
// The following method feeds the logger with a TS packet.
//----------------------------------------------------------------------------

void ts::TablesLogger::feedPacket (const TSPacket& pkt)
{
    _demux.feedPacket (pkt);
    _packet_count++;
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void ts::TablesLogger::handleTable (SectionDemux&, const BinaryTable& table)
{
    assert (table.sectionCount() > 0);

    // Add PMT PID's when necessary
    if (_opt.add_pmt_pids && table.tableId() == TID_PAT) {
        PAT pat (table);
        if (pat.isValid()) {
            if (pat.nit_pid != PID_NULL) {
                _demux.addPID (pat.nit_pid);
            }
            for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                _demux.addPID (it->second);
            }
        }
    }

    // Ignore table if not to be filtered
    if (!isFiltered (*table.sectionAt(0))) {
        return;
    }

    switch (_opt.mode) {

        case TablesLoggerOptions::TEXT: {
            preDisplay (table.getFirstTSPacketIndex(), table.getLastTSPacketIndex());
            if (_opt.raw_dump) {
                // Display hexa dump of each section in the table
                for (size_t i = 0; i < table.sectionCount(); ++i) {
                    const Section& sect (*table.sectionAt(i));
                    _out << Hexa (sect.content(), sect.size(), _opt.raw_flags | hexa::BPL, 0, 16) << std::endl;
                }
            }
            else if (_opt.logger) {
                // Short log message
                logSection (*table.sectionAt(0));
            }
            else {
                // Full table formatting
                table.display (_out, 0, _opt.cas) << std::endl;
            }
            postDisplay();
            break;
        }

        case TablesLoggerOptions::BINARY: {
            // Save each section in binary format
            for (size_t i = 0; i < table.sectionCount(); ++i) {
                saveSection (*table.sectionAt(i));
            }
            break;
        }

        case TablesLoggerOptions::UDP: {
            ByteBlock bb;
            // Minimize allocation by reserving over size
            bb.reserve (table.totalSize() + 32 + 4 * table.sectionCount());
            if (_opt.raw_dump) {
                // Add raw content of each section the message
                for (size_t i = 0; i < table.sectionCount(); ++i) {
                    const Section& sect (*table.sectionAt(i));
                    bb.append (sect.content(), sect.size());
                }
            }
            else {
                // Build a TLV message. Each section is a separate PRM_SECTION parameter.
                startMessage (bb, tlv::MSG_LOG_TABLE, table.sourcePID());
                for (size_t i = 0; i < table.sectionCount(); ++i) {
                    addSection (bb, *table.sectionAt(i));
                }
            }
            // Send TLV message over UDP
            _sock.send (bb.data(), bb.size(), _opt);
            break;
        }

        default: {
            // Should never get there
            assert (false);
        }
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

void ts::TablesLogger::handleSection (SectionDemux&, const Section& sect)
{
    // Ignore section if not to be filtered
    if (!isFiltered (sect)) {
        return;
    }

    switch (_opt.mode) {

        case TablesLoggerOptions::TEXT: {
            preDisplay (sect.getFirstTSPacketIndex(), sect.getLastTSPacketIndex());
            if (_opt.raw_dump) {
                // Display hexa dump of the section
                _out << Hexa (sect.content(), sect.size(), _opt.raw_flags | hexa::BPL, 0, 16) << std::endl;
            }
            else if (_opt.logger) {
                // Short log message
                logSection (sect);
            }
            else {
                // Full section formatting
                sect.display (_out, 0, _opt.cas) << std::endl;
            }
            postDisplay();
            break;
        }

        case TablesLoggerOptions::BINARY: {
            // Save section in binary format
            saveSection (sect);
            break;
        }

        case TablesLoggerOptions::UDP: {
            if (_opt.raw_dump) {
                // Send raw content of section as one single UDP message
                _sock.send (sect.content(), sect.size(), _opt);
            }
            else {
                ByteBlock bb;
                // Minimize allocation by reserving over size
                bb.reserve (sect.size() + 32);
                // Build a TLV message with one PRM_SECTION parameter.
                startMessage (bb, tlv::MSG_LOG_SECTION, sect.sourcePID());
                addSection (bb, sect);
                // Send TLV message over UDP
                _sock.send (bb.data(), bb.size(), _opt);
            }
            break;
        }

        default: {
            // Should never get there
            assert (false);
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

void ts::TablesLogger::saveSection (const Section& sect)
{
    // Create individual file for this section if required.
    if (_opt.multi_files) {
        // Build a unique file name for this section
        std::string outname (PathPrefix (_opt.destination));
        outname += Format ("_p%04X_t%02X", int (sect.sourcePID()), int (sect.tableId()));
        if (sect.isLongSection()) {
            outname += Format ("_e%04X_v%02X_s%02X", int (sect.tableIdExtension()), int (sect.version()), int (sect.sectionNumber()));
        }
        outname += PathSuffix (_opt.destination);
        // Create the output file
        _opt.verbose ("creating " + outname);
        _outfile.open (outname.c_str(), std::ios::out | std::ios::binary);
        if (!_outfile) {
            _opt.error ("error creating " + outname);
            _abort = true;
            return;
        }
    }

    // Write the section to the file
    if (!sect.write (_outfile, _opt)) {
        _abort = true;
    }

    // Close individual files
    if (_opt.multi_files) {
        _outfile.close();
    }
}


//----------------------------------------------------------------------------
//  Log a table (option --log)
//----------------------------------------------------------------------------

void ts::TablesLogger::logSection (const Section& sect)
{
    // Display time stamp if required
    if (_opt.time_stamp) {
        _out << Time::CurrentLocalTime() << ": ";
    }

    // Display packet index if required
    if (_opt.packet_index) {
        _out << "Packet " << Decimal (sect.getFirstTSPacketIndex())
             << " to " << Decimal (sect.getLastTSPacketIndex())
             << ", ";
    }

    // Is this a SafeAccess EMM?
    ts::TID tid = sect.tableId();
    bool sa_emm =
        _opt.cas == CAS_SAFEACCESS &&
        sect.payloadSize() >= 6 &&
        (tid == TID_SA_EMM_STB_U ||
         tid == TID_SA_EMM_STB_G ||
         tid == TID_SA_EMM_A ||
         tid == TID_SA_EMM_U ||
         tid == TID_SA_EMM_S);

    if (sa_emm) {
        uint32_t addr = GetUInt32 (sect.payload() + 2);
        switch (tid) {
            case TID_SA_EMM_A:
                _out << "UA: all" << std::endl;
                break;
            case TID_SA_EMM_U:
                _out << "UA: " << addr << std::endl;
                break;
            case TID_SA_EMM_S:
                _out << "GROUP: " << ((addr >> 8) & 0x00FFFFFF) << std::endl;
                break;
            case TID_SA_EMM_STB_U:
                _out << "STB: " << addr << std::endl;
                break;
            case TID_SA_EMM_STB_G:
                _out << "STB: all" << std::endl;
                break;
            default:
                break;
        }
    }
    else {
        size_t size = _opt.log_size <= sect.payloadSize() ? _opt.log_size : sect.payloadSize();
        _out << Format ("PID 0x%04X, TID 0x%02X", int (sect.sourcePID()), int (sect.tableId()));
        if (sect.isLongSection()) {
            _out << Format (", TIDext 0x%04X, V%d", int (sect.tableIdExtension()), int (sect.version()));
        }
        _out << ": " << Hexa (sect.payload(), size, hexa::SINGLE_LINE);
        if (sect.payloadSize() > size) {
            _out << " ...";
        }
        _out << std::endl;
    }
}


//----------------------------------------------------------------------------
//  Check if a specific section must be filtered
//----------------------------------------------------------------------------

bool ts::TablesLogger::isFiltered (const Section& sect) const
{
    // Filter by TID / TIDext
    bool tid_set = _opt.tid.find (sect.tableId()) != _opt.tid.end();
    bool tidext_set = _opt.tidext.find (sect.tableIdExtension()) != _opt.tidext.end();
    bool ok =
        // TID ok
        (_opt.tid.empty() || (tid_set && !_opt.negate_tid) || (!tid_set && _opt.negate_tid)) &&
        // TIDext ok
        (!sect.isLongSection() || _opt.tidext.empty() || (tidext_set && !_opt.negate_tidext) || (!tidext_set && _opt.negate_tidext));

    // Filter specific EMM
    if (ok && _opt.cas == CAS_SAFEACCESS && sect.payloadSize() >= 6) {
        // Address field of an EMM (meaningless if section is not an EMM);
        uint32_t addr = GetUInt32 (sect.payload() + 2);
        if (!_opt.emm_ua.empty()) {
            ok = sect.tableId() == TID_SA_EMM_U && _opt.emm_ua.find (addr) != _opt.emm_ua.end();
        }
        if (!_opt.emm_group.empty()) {
            ok = sect.tableId() == TID_SA_EMM_S && _opt.emm_group.find ((addr >> 8) & 0x00FFFFFF) != _opt.emm_group.end();
        }
    }

    // Filter diversified sections
    if (ok && _opt.diversified && sect.payloadSize() >= 2) {
        const uint8_t* pl = sect.payload();
        size_t end = sect.payloadSize() - 1;
        ok = false;
        for (size_t i = 0; !ok && i < end; ++i) {
            ok = pl[i] != pl[i+1];
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
//  Display header information, before a table
//----------------------------------------------------------------------------

void ts::TablesLogger::preDisplay (PacketCounter first, PacketCounter last)
{
    // Initial spacing
    if (_table_count == 0 && !_opt.logger) {
        _out << std::endl;
    }

    // Display time stamp if required
    if ((_opt.time_stamp || _opt.packet_index) && !_opt.logger) {
        _out << "* ";
        if (_opt.time_stamp) {
            _out << "At " << Time::CurrentLocalTime();
        }
        if (_opt.packet_index && _opt.time_stamp) {
            _out << ", ";
        }
        if (_opt.packet_index) {
            _out << "First TS packet: " << Decimal (first) << ", last: " << Decimal (last);
        }
        _out << std::endl;
    }
}


//----------------------------------------------------------------------------
//  Post-display action
//----------------------------------------------------------------------------

void ts::TablesLogger::postDisplay()
{
    // Flush output file if required
    if (_opt.flush) {
        _out.flush();
#if !defined (__windows)
        if (_opt.useCout()) {
            ::fflush (stdout);
            ::fsync (STDOUT_FILENO);
        }
#endif
    }
}


//----------------------------------------------------------------------------
//  Build header of a TLV message
//----------------------------------------------------------------------------

void ts::TablesLogger::startMessage (ByteBlock& bb, uint16_t message_type, PID pid)
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

void ts::TablesLogger::addSection (ByteBlock& bb, const Section& sect)
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

void ts::TablesLogger::reportDemuxErrors (std::ostream& strm)
{
    if (_demux.hasErrors()) {
        SectionDemux::Status status (_demux);
        strm << "* PSI/SI analysis errors:" << std::endl;
        status.display (strm, 4, true);
    }
}
