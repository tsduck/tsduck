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

#include "tsPSILogger.h"
#include "tsDuckContext.h"
#include "tsBinaryTable.h"
#include "tsTSPacket.h"
#include "tsNames.h"
#include "tsPAT.h"
TSDUCK_SOURCE;

#define MIN_CLEAR_PACKETS 100000


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::PSILogger::PSILogger(TablesDisplay& display) :
    ArgsSupplierInterface(),
    TableHandlerInterface(),
    SectionHandlerInterface(),
    _all_versions(false),
    _clear(false),
    _cat_only(false),
    _dump(false),
    _output(),
    _use_current(true),
    _use_next(false),
    _display(display),
    _duck(_display.duck()),
    _abort(false),
    _pat_ok(_cat_only),
    _cat_ok(_clear),
    _sdt_ok(_cat_only),
    _bat_ok(false),
    _expected_pmt(0),
    _received_pmt(0),
    _clear_packets_cnt(0),
    _scrambled_packets_cnt(0),
    _demux(_duck, this, _dump ? this : nullptr),
    _standards(Standards::NONE)
{
}

ts::PSILogger::~PSILogger()
{
    close();
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::PSILogger::defineArgs(Args& args) const
{
    args.option(u"all-versions", 'a');
    args.help(u"all-versions",
              u"Display all versions of PSI tables (need to read the complete "
              u"transport stream). By default, display only the first version "
              u"of each PSI table and stop when all expected PSI are extracted.");

    args.option(u"cat-only");
    args.help(u"cat-only", u"Display only the CAT, ignore other PSI tables.");

    args.option(u"clear", 'c');
    args.help(u"clear",
              u"Indicate that this is a clear transport stream, without "
              u"conditional access information. Useful to avoid reading the "
              u"complete transport stream, waiting for a non-existent CAT.");

    args.option(u"dump", 'd');
    args.help(u"dump", u"Dump all PSI sections.");

    args.option(u"exclude-current");
    args.help(u"exclude-current",
              u"Exclude PSI tables with \"current\" indicator. "
              u"This is rarely necessary. See also --include-next.");

    args.option(u"include-next");
    args.help(u"include-next",
              u"Include PSI tables with \"next\" indicator. By default, they are excluded.");

    args.option(u"output-file", 'o', Args::STRING);
    args.help(u"output-file", u"File name for text output.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::PSILogger::loadArgs(DuckContext& duck, Args& args)
{
    _all_versions = args.present(u"all-versions");
    _cat_only = args.present(u"cat-only");
    _clear = args.present(u"clear");
    _dump = args.present(u"dump");
    _output = args.value(u"output-file");
    _use_current = !args.present(u"exclude-current");
    _use_next = args.present(u"include-next");
    return true;
}


//----------------------------------------------------------------------------
// Open / close the PSI logger.
//----------------------------------------------------------------------------

bool ts::PSILogger::open()
{
    // Open/create the destination
    if (!_duck.setOutput(_output)) {
        _abort = true;
        return false;
    }

    // Specify the PID filters
    _demux.reset();
    if (!_cat_only) {
        _demux.addPID(PID_PAT);   // MPEG
        _demux.addPID(PID_TSDT);  // MPEG
        _demux.addPID(PID_SDT);   // DVB, ISDB (also contain BAT)
        _demux.addPID(PID_PCAT);  // ISDB
        _demux.addPID(PID_BIT);   // ISDB
        _demux.addPID(PID_LDT);   // ISDB (also contain NBIT)
        _demux.addPID(PID_PSIP);  // ATSC
    }
    if (!_clear) {
        _demux.addPID(PID_CAT);
    }

    // Type of sections to get.
    _demux.setCurrentNext(_use_current, _use_next);

    // Initial blank line
    _duck.out() << std::endl;
    return true;
}

void ts::PSILogger::close()
{
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

    // Check if the list of standards has changed.
    const Standards new_standards = _duck.standards();
    if (new_standards != _standards) {
        _duck.report().debug(u"standards are now %s", {StandardsNames(new_standards)});
        _standards = new_standards;
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void ts::PSILogger::handleTable(SectionDemux&, const BinaryTable& table)
{
    assert(table.sectionCount() > 0);

    std::ostream& strm(_duck.out());
    const TID tid = table.tableId();
    const PID pid = table.sourcePID();

    switch (table.tableId()) {

        case TID_PAT: {
            PAT pat(_duck, table);
            if (pid != PID_PAT) {
                // A PAT is only expected on PID 0
                strm << UString::Format(u"* Got unexpected PAT on PID %d (0x%X)", {pid, pid}) << std::endl;
            }
            else if (pat.isValid()) {
                // Got the PAT.
                _pat_ok = true;
                // Stop filtering the PAT PID if we don't need all versions.
                if (!_all_versions) {
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
            strm << std::endl;
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
                if (!_all_versions) {
                    _demux.removePID(pid);
                }
            }
            // Display the table
            _display.displayTable(table);
            strm << std::endl;
            break;
        }

        case TID_NIT_ACT:  // NIT and PMT are processed identically.
        case TID_PMT: {
            // Stop filtering this PID if we don't need all versions.
            if (!_all_versions) {
                _demux.removePID(pid);
                _received_pmt++;
            }
            _display.displayTable(table);
            strm << std::endl;
            break;
        }

        case TID_NIT_OTH: {
            // Ignore NIT for other networks if only one version required
            if (_all_versions) {
                _display.displayTable(table);
                strm << std::endl;
            }
            break;
        }

        case TID_TSDT: {
            if (pid != PID_TSDT) {
                // A TSDT is only expected on PID 0x0002
                strm << UString::Format(u"* Got unexpected TSDT on PID %d (0x%X)", {pid, pid}) << std::endl;
            }
            else if (!_all_versions) {
                _demux.removePID(pid);
            }
            _display.displayTable(table);
            strm << std::endl;
            break;
        }

        case TID_SDT_ACT: {
            if (pid != PID_SDT) {
                // An SDT is only expected on PID 0x0011
                strm << UString::Format(u"* Got unexpected SDT on PID %d (0x%X)", {pid, pid}) << std::endl;
                _display.displayTable(table);
                strm << std::endl;
            }
            else if (_all_versions || !_sdt_ok) {
                _sdt_ok = true;
                // We cannot stop filtering this PID if we don't need all versions since a BAT can also be found here.
                _display.displayTable(table);
                strm << std::endl;
            }
            break;
        }

        case TID_SDT_OTH: {
            // Ignore SDT for other networks if only one version required
            if (_all_versions) {
                _display.displayTable(table);
                strm << std::endl;
            }
            break;
        }

        case TID_BAT: {
            if (pid != PID_BAT) {
                // An SDT is only expected on PID 0x0011
                strm << UString::Format(u"* Got unexpected BAT on PID %d (0x%X)", {pid, pid}) << std::endl;
                _display.displayTable(table);
                strm << std::endl;
            }
            else if (_all_versions || !_bat_ok) {
                // Got the BAT.
                _bat_ok = true;
                // We cannot stop filtering this PID if we don't need all versions since the SDT can also be found here.
                _display.displayTable(table);
                strm << std::endl;
            }
            break;
        }

        case TID_PCAT: {
            if (pid != PID_PCAT) {
                // An ISDB PCAT is only expected on PID 0x0022
                strm << UString::Format(u"* Got unexpected ISDB PCAT on PID %d (0x%X)", {pid, pid}) << std::endl;
            }
            else if (!_all_versions) {
                _demux.removePID(pid);
            }
            _display.displayTable(table);
            strm << std::endl;
            break;
        }

        case TID_BIT: {
            if (pid != PID_BIT) {
                // An ISDB BIT is only expected on PID 0x0024
                strm << UString::Format(u"* Got unexpected ISDB BIT on PID %d (0x%X)", {pid, pid}) << std::endl;
            }
            else if (!_all_versions) {
                _demux.removePID(pid);
            }
            _display.displayTable(table);
            strm << std::endl;
            break;
        }

        case TID_NBIT_REF:
        case TID_NBIT_BODY: {
            if (pid != PID_NBIT) {
                // An ISDB BIT is only expected on PID 0x0025
                strm << UString::Format(u"* Got unexpected ISDB NBIT on PID %d (0x%X)", {pid, pid}) << std::endl;
            }
            // We cannot stop filtering this PID if we don't need all versions since the LDT can also be found here.
            _display.displayTable(table);
            strm << std::endl;
            break;
        }

        // case TID_LDT: (same value as TID_MGT)
        case TID_MGT: {
            // ATSC MGT and ISDB LDT use the same table id, so it can be any.
            if (pid != PID_PSIP && pid != PID_LDT) {
                // An ATSC MGT is only expected on PID 0x1FFB.
                // An ISDB LDT is only expected on PID 0x0025.
                strm << UString::Format(u"* Got unexpected ATSC MGT / ISDB LDT on PID %d (0x%X)", {pid, pid}) << std::endl;
            }
            // We cannot stop filtering this PID if we don't need all versions
            // since the TVCT or CVCT (ATSC) and NBIT (ISDB) can also be found here.
            _display.displayTable(table);
            strm << std::endl;
            break;
        }

        case TID_TVCT:
        case TID_CVCT: {
            // ATSC tables with channel description.
            _display.displayTable(table);
            strm << std::endl;
            break;
        }

        default: {
            if (_duck.report().verbose()) {
                strm << UString::Format(u"* Got unexpected TID %d (0x%X) on PID %d (0x%X)", {tid, tid, pid, pid}) << std::endl << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked when a complete section is available.
// Only used with option --all-sections
//----------------------------------------------------------------------------

void ts::PSILogger::handleSection(SectionDemux&, const Section& sect)
{
    sect.dump(_duck.out()) << std::endl;
}


//----------------------------------------------------------------------------
// Report the demux errors (if any)
//----------------------------------------------------------------------------

void ts::PSILogger::reportDemuxErrors()
{
    if (_demux.hasErrors()) {
        SectionDemux::Status status(_demux);
        _duck.out() << "* PSI/SI analysis errors:" << std::endl;
        status.display(_duck.out(), 4, true);
    }
}
