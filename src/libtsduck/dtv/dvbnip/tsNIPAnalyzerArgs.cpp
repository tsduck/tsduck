//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIPAnalyzerArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::NIPAnalyzerArgs::defineArgs(Args& args)
{
    args.option(u"dump-flute-payload");
    args.help(u"dump-flute-payload",
              u"Same as --log-flute-packets and also dump the payload of each FLUTE packet.");

    args.option(u"dump-xml-files");
    args.help(u"dump-xml-files",
              u"Dump the content of XML files when they are received.");

    args.option(u"log-fdt");
    args.help(u"log-fdt",
              u"Log a message describing each FLUTE File Delivery Table (FDT).");

    args.option(u"log-files");
    args.help(u"log-files",
              u"Log a message describing each received file.");

    args.option(u"log-flute-packets");
    args.help(u"log-flute-packets",
              u"Log a message describing the structure of each FLUTE packet.");

    args.option(u"save-bootstrap", 0, Args::FILENAME);
    args.help(u"save-bootstrap",
              u"Save the bootstrap multicast gateway configuration in the specified file. "
              u"This is a XML file. "
              u"If the specified path is '-', the file is written to standard output.");

    args.option(u"save-fdt", 0, Args::FILENAME);
    args.help(u"save-fdt",
              u"Save each FLUTE File Delivery Table (FDT) in a file. "
              u"Each FDT instance is saved in a separate file. "
              u"If the specified path is 'dir/fdt.xml' for instance, the FDT with instance N is saved in file 'dir/fdt-N.xml'. "
              u"If the specified path is '-', the file is written to standard output.");

    args.option(u"save-nif", 0, Args::FILENAME);
    args.help(u"save-nif",
              u"Save the DVB-NIP Network Information File (NIF) in the specified file. "
              u"This is a XML file. "
              u"If the specified path is '-', the file is written to standard output.");

    args.option(u"save-sif", 0, Args::FILENAME);
    args.help(u"save-sif",
              u"Save the DVB-NIP Service Information File (SIF) in the specified file. "
              u"This is a XML file. "
              u"If the specified path is '-', the file is written to standard output.");

    args.option(u"save-slep", 0, Args::FILENAME);
    args.help(u"save-slep",
              u"Save the DVB-I Service List Entry Points (SLEP) in the specified file. "
              u"This is a XML file. "
              u"If the specified path is '-', the file is written to standard output.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::NIPAnalyzerArgs::loadArgs(DuckContext& duck, Args& args)
{
    dump_flute_payload = args.present(u"dump-flute-payload");
    log_flute_packets = dump_flute_payload || args.present(u"log-flute-packets");
    log_fdt = args.present(u"log-fdt");
    log_files = args.present(u"log-files");
    dump_xml_files = args.present(u"dump-xml-files");
    args.getPathValue(save_fdt, u"save-fdt");
    args.getPathValue(save_nif, u"save-nif");
    args.getPathValue(save_sif, u"save-sif");
    args.getPathValue(save_slep, u"save-slep");
    args.getPathValue(save_bootstrap, u"save-bootstrap");
    return true;
}
