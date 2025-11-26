//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFluteDemuxArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Check if something specific was required.
//----------------------------------------------------------------------------

bool ts::FluteDemuxArgs::none() const
{
    return !log_flute_packets && !dump_flute_payload && !log_fdt && !log_files && !dump_xml_files && save_fdt.empty();
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::FluteDemuxArgs::defineArgs(Args& args)
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

    args.option(u"max-file-size", 0, Args::UINT63);
    args.help(u"max-file-size",
              u"Maximum size of files to analyze or extract. "
              u"Each received file is accumulated in memory, chunk by chunk, until the file is complete. "
              u"After processing the file, all chunks are freeed. "
              u"This option is useful when the stream contains many large files which clutter the memory during their reception. "
              u"All files which are announced as larger than the specified size are ignored. "
              u"By default, all files are received and analyzed or saved.");

    args.option(u"save-fdt", 0, Args::FILENAME);
    args.help(u"save-fdt",
              u"Save each FLUTE File Delivery Table (FDT) in a file. "
              u"Each FDT instance is saved in a separate file. "
              u"If the specified path is 'dir/fdt.xml' for instance, the FDT with instance N is saved in file 'dir/fdt-N.xml'. "
              u"If the specified path is '-', the file is written to standard output.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::FluteDemuxArgs::loadArgs(DuckContext& duck, Args& args)
{
    dump_flute_payload = args.present(u"dump-flute-payload");
    log_flute_packets = dump_flute_payload || args.present(u"log-flute-packets");
    log_fdt = args.present(u"log-fdt");
    log_files = args.present(u"log-files");
    dump_xml_files = args.present(u"dump-xml-files");
    args.getIntValue(max_file_size, u"max-file-size");
    args.getPathValue(save_fdt, u"save-fdt");
    return true;
}
