//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
// Transport Stream file cleanup utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsSignalizationDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsTSFile.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsTS.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace ts {
    class FileCleanOptions: public Args
    {
        TS_NOBUILD_NOCOPY(FileCleanOptions);
    public:
        FileCleanOptions(int argc, char *argv[]);
        virtual ~FileCleanOptions() override;

        DuckContext   duck;      // TSDuck execution context.
        UStringVector in_files;  // Input file names.
        UString       out_file;  // Output file name or directory.
        bool          out_dir;   // Output name is a directory.
    };
}

ts::FileCleanOptions::FileCleanOptions(int argc, char *argv[]) :
    Args(u"Cleanup an MPEG transport stream file", u"[options] filename ..."),
    duck(this),
    in_files(),
    out_file(),
    out_dir()
{
    option(u"", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"",
         u"MPEG transport stream input files to cleanup. "
         u"All input files must be regular files (no pipe) since the processing is done on two passes. "
         u"If more than one file is specified, the output name shall specify a directory.");

    option(u"output", 'o', STRING, 1, 1);
    help(u"output", u"path",
         u"Output file or directory. "
         u"This is a mandatory parameter, there is no default. "
         u"If more than one input file is specified, the output name shall specify a directory.");

    analyze(argc, argv);

    getValues(in_files, u"");
    getValue(out_file, u"output");
    out_dir = IsDirectory(out_file);

    if (in_files.size() > 1 && !out_dir) {
        error(u"the output name must be a directory when more than one input file is specified");
    }
    exitOnError();
}

ts::FileCleanOptions::~FileCleanOptions()
{
}


//----------------------------------------------------------------------------
// A class to do the file cleanup.
//----------------------------------------------------------------------------

namespace ts {
    class FileCleaner: private SignalizationHandlerInterface
    {
        TS_NOBUILD_NOCOPY(FileCleaner);
    public:
        // Constructor, performing the TS file cleanup.
        FileCleaner(FileCleanOptions& opt, const UString& infile_name);

        // Status of the cleanup.
        bool success() const { return _success; }

    private:
        // Context of a PMT PID.
        class PMTContext
        {
            TS_NOBUILD_NOCOPY(PMTContext);
        public:
            // Constructor:
            PMTContext(const DuckContext& duck, PID pmt_pid, Report& report);

            // Public fields:
            const PID         pmt_pid;
            PMT               pmt;
            CyclingPacketizer pzer;
        };

        // A map of PMT contexts, indexed by PMT PID.
        typedef SafePtr<PMTContext> PMTContextPtr;
        typedef std::map<PID,PMTContextPtr> PMTContextMap;
        PMTContextPtr getPMTContext(PID pmt_pid, bool create);

        // File cleaner private fields:
        bool               _success;
        FileCleanOptions&  _opt;
        TSFile             _in_file;
        TSFile             _out_file;
        SignalizationDemux _demux;
        PAT                _pat;
        CyclingPacketizer  _pat_pzer;
        CAT                _cat;
        CyclingPacketizer  _cat_pzer;
        SDT                _sdt;
        CyclingPacketizer  _sdt_pzer;
        PMTContextMap      _pmts;

        // Implementation of SignalizationHandlerInterface:
        virtual void handlePAT(const PAT& pat, PID pid) override;
        virtual void handleCAT(const CAT& cat, PID pid) override;
        virtual void handleSDT(const SDT& sdt, PID pid) override;
        virtual void handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed) override;

        // Initialize a packetizer with one table and output the first cycle.
        void initCycle(AbstractTable& table, CyclingPacketizer& pzer);
    };
}


//----------------------------------------------------------------------------
// File cleaner constructor.
//----------------------------------------------------------------------------

ts::FileCleaner::FileCleaner(FileCleanOptions& opt, const UString& infile_name) :
    _success(true),
    _opt(opt),
    _in_file(),
    _out_file(),
    _demux(_opt.duck, this, {TID_PAT, TID_CAT, TID_PMT, TID_SDT_ACT}),
    _pat(),
    _pat_pzer(_opt.duck, PID_PAT, CyclingPacketizer::StuffingPolicy::ALWAYS, 0, &_opt),
    _cat(),
    _cat_pzer(_opt.duck, PID_CAT, CyclingPacketizer::StuffingPolicy::ALWAYS, 0, &_opt),
    _sdt(),
    _sdt_pzer(_opt.duck, PID_SDT, CyclingPacketizer::StuffingPolicy::ALWAYS, 0, &_opt),
    _pmts()
{
    // Mark all tables as invalid. The first occurrence in the input file will initialize them.
    _pat.invalidate();
    _cat.invalidate();
    _sdt.invalidate();

    // Output file name.
    UString outfile_name(_opt.out_file);
    if (_opt.out_dir) {
        // Output name is a directory.
        outfile_name.append(PathSeparator);
        outfile_name.append(BaseName(infile_name));
    }
    _opt.verbose(u"cleaning %s -> %s", {infile_name, outfile_name});

    // Open the input file in rewindable mode.
    if (!_in_file.openRead(infile_name, 0, _opt)) {
        _success = false;
        return;
    }

    // Create output file before first pass to avoid spending time on first pass in case of error when creating output.
    if (!_out_file.open(outfile_name, TSFile::WRITE, _opt)) {
        _success = false;
        return;
    }

    // First pass: read all packets, process TS structure.
    TSPacket pkt;
    while (_in_file.readPackets(&pkt, nullptr, 1, _opt) == 1) {
        _demux.feedPacket(pkt);
    }

    // Start output file. First, issue a full cycle of each PSI/SI.
    initCycle(_pat, _pat_pzer);
    initCycle(_cat, _cat_pzer);
    initCycle(_sdt, _sdt_pzer);
    for (auto it = _pmts.begin(); it != _pmts.end(); ++it) {
        initCycle(it->second->pmt, it->second->pzer);
    }

    // Second pass: read input file again, write output file.
    if (!_in_file.rewind(_opt)) {
        _success = false;
        return;
    }
    while (_in_file.readPackets(&pkt, nullptr, 1, _opt) == 1) {
        //@@@@@@@@@
    }

    // Close files.
    _success = _in_file.close(_opt) && _success;
    _success = _out_file.close(_opt) && _success;
}


//----------------------------------------------------------------------------
// Invoke for each PAT in the first pass.
//----------------------------------------------------------------------------

void ts::FileCleaner::handlePAT(const PAT& pat, PID pid)
{
    _opt.debug(u"got PAT version %d", {pat.version});
    if (!_pat.isValid()) {
        // First PAT.
        _pat = pat;
        _pat.nit_pid = PID_NULL; // no NIT in output TS
    }
    else {
        // Updated PAT, add new services, check inconsistencies.
        _opt.verbose(u"got PAT update, version %d", {pat.version});
        for (auto it = pat.pmts.begin(); it != pat.pmts.end(); ++it) {
            const auto cur = _pat.pmts.find(it->first);
            if (cur == _pat.pmts.end()) {
                // Add new service in PAT update.
                _opt.verbose(u"added service 0x%X (%<d) from PAT update", {it->first});
                _pat.pmts[it->first] = it->second;
            }
            else if (it->second != cur->second) {
                // Existing service changes PMT PID, not allowed.
                _opt.error(u"service 0x%X (%<d) changed PMT PID from 0x%X (%<d) to 0x%X (%<d) in PAT update", {it->first, cur->second, it->second});
                _success = false;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Invoke for each CAT in the first pass.
//----------------------------------------------------------------------------

void ts::FileCleaner::handleCAT(const CAT& cat, PID pid)
{
    _opt.debug(u"got CAT version %d", {cat.version});
    if (!_cat.isValid()) {
        // First CAT.
        _cat = cat;
    }
    else {
        // Updated CAT, merge descriptors (don't duplicate existing ones).
        _opt.verbose(u"got CAT update, version %d", {cat.version});
        _cat.descs.merge(_opt.duck, cat.descs);
    }
}


//----------------------------------------------------------------------------
// Invoke for each SDT in the first pass.
//----------------------------------------------------------------------------

void ts::FileCleaner::handleSDT(const SDT& sdt, PID pid)
{
    _opt.debug(u"got SDT version %d", {sdt.version});
    if (!_sdt.isValid()) {
        // First SDT.
        _sdt = sdt;
    }
    else {
        // Updated SDT, add new services, merge others.
        _opt.verbose(u"got SDT update, version %d", {sdt.version});
        for (auto it = sdt.services.begin(); it != sdt.services.end(); ++it) {
            const auto cur = _sdt.services.find(it->first);
            if (cur == _sdt.services.end()) {
                // Add new service in SDT update.
                _opt.verbose(u"added service 0x%X (%<d) from SDT update", {it->first});
                _sdt.services[it->first] = it->second;
            }
            else {
                // Existing service, merge descriptors.
                cur->second.descs.merge(_opt.duck, it->second.descs);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Invoke each time a service is modified in the first pass.
//----------------------------------------------------------------------------

void ts::FileCleaner::handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed)
{
    //@@@@@@@@@@@@@@
}


//----------------------------------------------------------------------------
// Context of a PMT PID.
//----------------------------------------------------------------------------

ts::FileCleaner::PMTContext::PMTContext(const DuckContext& duck, PID pid, Report& report) :
    pmt_pid(pid),
    pmt(),
    pzer(duck, pmt_pid, CyclingPacketizer::StuffingPolicy::ALWAYS, 0, &report)
{
    pmt.invalidate();
}

ts::FileCleaner::PMTContextPtr ts::FileCleaner::getPMTContext(PID pmt_pid, bool create)
{
    auto it = _pmts.find(pmt_pid);
    if (it != _pmts.end()) {
        return it->second;
    }
    else if (create) {
        return _pmts[pmt_pid] = PMTContextPtr(new PMTContext(_opt.duck, pmt_pid, _opt));
    }
    else {
        return PMTContextPtr();
    }
}


//----------------------------------------------------------------------------
// Initialize a packetizer with one table and output the first cycle.
//----------------------------------------------------------------------------

void ts::FileCleaner::initCycle(AbstractTable& table, CyclingPacketizer& pzer)
{
    if (table.isValid()) {
        pzer.addTable(_opt.duck, table);
        TSPacket pkt;
        do {
            if (pzer.getNextPacket(pkt) && !_out_file.writePackets(&pkt, nullptr, 1, _opt)) {
                _success = false;
                break;
            }
        } while (!pzer.atCycleBoundary());
    }
}


//----------------------------------------------------------------------------
// Program entry point.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    ts::FileCleanOptions opt(argc, argv);
    bool success = true;

    for (size_t i = 0; i < opt.in_files.size(); ++i) {
        ts::FileCleaner fclean(opt, opt.in_files[i]);
        success = success && fclean.success();
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
