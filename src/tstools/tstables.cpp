//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Collect selected PSI/SI tables from a transport stream.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTSFile.h"
#include "tsTablesDisplay.h"
#include "tsTablesLogger.h"
#include "tsPSIRepository.h"
#include "tsDVBCharset.h"
#include "tsDVBCharTable.h"
#include "tsPagerArgs.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext    duck {this};                // TSDuck execution context.
        ts::TablesDisplay  display {duck};             // Table formatting.
        ts::TablesLogger   logger {display};           // Table logging.
        ts::PagerArgs      pager {true, true};         // Output paging options.
        fs::path           infile {};                  // Input file name.
        ts::TSPacketFormat format = ts::TSPacketFormat::AUTODETECT;
        bool               psi_info = false;           // PSI/SI information only, no inut file.
        bool               dump_psi_repo = false;      // Dump internal state of PSI repository.
        bool               list_tables = false;        // List all known tables.
        bool               list_descriptors = false;   // List all known descriptors.
        bool               list_dvb_charsets = false;  // List order of DVB character sets.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Collect PSI/SI tables from an MPEG transport stream", u"[options] [filename]")
{
    duck.defineArgsForCAS(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForCharset(*this);
    pager.defineArgs(*this);
    logger.defineArgs(*this);
    display.defineArgs(*this);
    ts::DefineTSPacketFormatInputOption(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"Input transport stream file (standard input if omitted).");

    // PSI/SI information options.
    option(u"dump-psi-repository");
    help(u"dump-psi-repository", u"Dump the internal state of the PSI repository. This is a debug function.");

    option(u"list-dvb-charset-order");
    help(u"list-dvb-charset-order",
         u"List all DVB character sets in their preferred order. "
         u"Each time a DVB string is serialized in binary form, these character sets are tried in this order, "
         u"until one is able to encode the string.");

    option(u"list-descriptors");
    help(u"list-descriptors", u"List all supported PSI/SI descriptors.");

    option(u"list-tables");
    help(u"list-tables", u"List all supported PSI/SI tables.");

    analyze(argc, argv);

    duck.loadArgs(*this);
    pager.loadArgs(*this);
    logger.loadArgs(duck, *this);
    display.loadArgs(duck, *this);

    getPathValue(infile, u"");
    format = ts::LoadTSPacketFormatInputOption(*this);

    list_dvb_charsets = present(u"list-dvb-charset-order");
    dump_psi_repo = present(u"dump-psi-repository");
    list_descriptors = present(u"list-descriptors");
    list_tables = present(u"list-tables");
    psi_info = dump_psi_repo || list_dvb_charsets || list_descriptors || list_tables;

    if (psi_info && !infile.empty()) {
        error(u"no input file allowed with PSI/SI information options");
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Decode command line options.
    Options opt(argc, argv);

    // Redirect display on pager process or stdout only.
    opt.duck.setOutput(&opt.pager.output(opt), false);

    // Process PSI/SI information options.
    if (opt.psi_info) {
        const ts::UChar* separator = u"";
        if (opt.list_dvb_charsets) {
            for (const auto& charset : ts::DVBCharset::GetPreferredCharsets()) {
                // This is a list of raw character tables. We prefer to display the corresponding DVB-compliant name.
                ts::UString name(charset->name());
                if (name.starts_with(u"RAW-")) {
                    name.erase(0, 4);
                }
                opt.duck.out() << name << std::endl;
            }
            separator = ts::UString::EOL;
        }
        if (opt.list_tables) {
            opt.duck.out() << separator;
            ts::PSIRepository::Instance().listTables(opt.duck.out());
            separator = ts::UString::EOL;
        }
        if (opt.list_descriptors) {
            opt.duck.out() << separator;
            ts::PSIRepository::Instance().listDescriptors(opt.duck.out());
            separator = ts::UString::EOL;
        }
        if (opt.dump_psi_repo) {
            opt.duck.out() << separator;
            ts::PSIRepository::Instance().dumpInternalState(opt.duck.out());
        }
        // No table processing in that case.
        return EXIT_SUCCESS;
    }

    // Open section logger.
    if (!opt.logger.open()) {
        return EXIT_FAILURE;
    }

    // Open the TS file.
    ts::TSFile file;
    if (!file.openRead(opt.infile, 1, 0, opt, opt.format)) {
        return EXIT_FAILURE;
    }

    // Read all packets in the file and pass them to the logger
    ts::TSPacket pkt;
    while (!opt.logger.completed() && file.readPackets(&pkt, nullptr, 1, opt) > 0) {
        opt.logger.feedPacket(pkt);
    }
    file.close(opt);
    opt.logger.close();

    // Report errors
    if (opt.verbose() && !opt.logger.hasErrors()) {
        opt.logger.reportDemuxErrors(std::cerr);
    }

    return opt.logger.hasErrors() ? EXIT_FAILURE : EXIT_SUCCESS;
}
