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

#include "tsPSILogger.h"
#include "tsNames.h"
#include "tsPAT.h"
TSDUCK_SOURCE;

#define MIN_CLEAR_PACKETS 100000


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PSILogger::PSILogger(PSILoggerArgs& opt, TablesDisplay& display, Report& report) :
    TableHandlerInterface(),
    SectionHandlerInterface(),
    _opt(opt),
    _display(display),
    _report(report),
    _abort(false),
    _pat_ok(_opt.cat_only),
    _cat_ok(_opt.clear),
    _sdt_ok(_opt.cat_only),
    _bat_ok(false),
    _expected_pmt(0),
    _received_pmt(0),
    _clear_packets_cnt(0),
    _scrambled_packets_cnt(0),
    _demux(this, _opt.dump ? this : nullptr)
{
    // Open/create the destination
    if (!_display.redirect(_opt.output)) {
        _abort = true;
        return;
    }

    // Specify the PID filters
    if (!_opt.cat_only) {
        _demux.addPID(PID_PAT);
        _demux.addPID(PID_TSDT);
        _demux.addPID(PID_SDT);
    }
    if (!_opt.clear) {
        _demux.addPID(PID_CAT);
    }

    // Type of sections to get.
    _demux.setCurrentNext(opt.use_current, opt.use_next);

    // Initial blank line
    _display.out() << std::endl;
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::PSILogger::~PSILogger()
{
    // Files are automatically closed by their destructors.
}


//----------------------------------------------------------------------------
// The following method feeds the logger with a TS packet.
//----------------------------------------------------------------------------

void ts::PSILogger::feedPacket(const TSPacket& pkt)
{
    // Feed the packet to the demux
    _demux.feedPacket(pkt);

    // On clear streams, there is no CAT (usually). To avoid waiting indefinitely,
    // if no CAT and no scrambled packet is found after a defined number of packets
    // (~ 4 seconds at satellite bitrate), no longer expect a CAT.
    if (pkt.getScrambling() == SC_CLEAR) {
        _clear_packets_cnt++;
    }
    else {
        _scrambled_packets_cnt++;
    }
    if (_scrambled_packets_cnt == 0 && _clear_packets_cnt > MIN_CLEAR_PACKETS) {
        _cat_ok = true;
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void ts::PSILogger::handleTable(SectionDemux&, const BinaryTable& table)
{
    assert(table.sectionCount() > 0);

    std::ostream& strm(_display.out());
    const TID tid = table.tableId();
    const PID pid = table.sourcePID();

    switch (table.tableId()) {

        case TID_PAT: {
            PAT pat(table);
            if (pid != PID_PAT) {
                // A PAT is only expected on PID 0
                strm << UString::Format(u"* Got unexpected PAT on PID %d (0x%X)", {pid, pid}) << std::endl;
            }
            else if (pat.isValid()) {
                // Got the PAT.
                _pat_ok = true;
                // Stop filtering the PAT PID if we don't need all versions.
                if (!_opt.all_versions) {
                    _demux.removePID(pid);
                }
                // Add a filter on each referenced PID to get the PMT
                for (PAT::ServiceMap::const_iterator it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
                    _demux.addPID(it->second);
                    _expected_pmt++;
                }
                // Also include NIT (considered as a PMT)
                _demux.addPID(pat.nit_pid != PID_NULL ? pat.nit_pid : PID(PID_NIT));
                _expected_pmt++;
            }
            // Display the content of the PAT
            _display.displayTable(table);
            break;
        }

        case TID_CAT: {
            if (pid != PID_CAT) {
                // A CAT is only expected on PID 1
                strm << UString::Format(u"* Got unexpected CAT on PID %d (0x%X)", {pid, pid}) << std::endl;
            }
            else {
                // Got the CAT.
                _cat_ok = true;
                // Stop filtering the CAT PID if we don't need all versions.
                if (!_opt.all_versions) {
                    _demux.removePID(pid);
                }
            }
            // Display the table
            _display.displayTable(table);
            break;
        }

        case TID_NIT_ACT:  // NIT and PMT are processed identically.
        case TID_PMT: {
            // Stop filtering this PID if we don't need all versions.
            if (!_opt.all_versions) {
                _demux.removePID(pid);
                _received_pmt++;
            }
            _display.displayTable(table);
            break;
        }

        case TID_NIT_OTH: {
            // Ignore NIT for other networks if only one version required
            if (_opt.all_versions) {
                _display.displayTable(table);
            }
            break;
        }

        case TID_TSDT: {
            if (pid != PID_TSDT) {
                // A TSDT is only expected on PID 0x0002
                strm << UString::Format(u"* Got unexpected TSDT on PID %d (0x%X)", {pid, pid}) << std::endl;
            }
            else if (!_opt.all_versions) {
                _demux.removePID(pid);
            }
            _display.displayTable(table);
            break;
        }

        case TID_SDT_ACT: {
            if (pid != PID_SDT) {
                // An SDT is only expected on PID 0x0011
                strm << UString::Format(u"* Got unexpected SDT on PID %d (0x%X)", {pid, pid}) << std::endl;
                _display.displayTable(table);
            }
            else if (_opt.all_versions || !_sdt_ok) {
                _sdt_ok = true;
                // We cannot stop filtering this PID if we don't need all versions
                // since a BAT can also be found here.
                _display.displayTable(table);
            }
            break;
        }

        case TID_SDT_OTH: {
            // Ignore SDT for other networks if only one version required
            if (_opt.all_versions) {
                _display.displayTable(table);
            }
            break;
        }

        case TID_BAT: {
            if (pid != PID_BAT) {
                // An SDT is only expected on PID 0x0011
                strm << UString::Format(u"* Got unexpected BAT on PID %d (0x%X)", {pid, pid}) << std::endl;
                _display.displayTable(table);
            }
            else if (_opt.all_versions || !_bat_ok) {
                // Got the BAT.
                _bat_ok = true;
                // We cannot stop filtering this PID if we don't need all versions
                // since the SDT can also be found here.
                _display.displayTable(table);
            }
            break;
        }

        default: {
            if (_report.verbose()) {
                strm << UString::Format(u"* Got unexpected TID %d (0x%X) on PID %d (0x%X)", {tid, tid, pid, pid}) << std::endl;
            }
        }
    }

    strm << std::endl;
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete section is available.
// Only used with option --all-sections
//----------------------------------------------------------------------------

void ts::PSILogger::handleSection(SectionDemux&, const Section& sect)
{
    sect.dump(_display.out()) << std::endl;
}


//----------------------------------------------------------------------------
// Report the demux errors (if any)
//----------------------------------------------------------------------------

void ts::PSILogger::reportDemuxErrors ()
{
    if (_demux.hasErrors()) {
        SectionDemux::Status status(_demux);
        _display.out() << "* PSI/SI analysis errors:" << std::endl;
        status.display(_display.out(), 4, true);
    }
}
