//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Send control commands to a running tsp.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsArgs.h"
#include "tsTelnetConnection.h"
#include "tsTSPControlCommand.h"
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

        ts::TSPControlCommand cmdline;
        ts::UString           command;
        ts::IPv4SocketAddress tsp_address;

        // Inherited methods.
        virtual ts::UString getHelpText(HelpFormat format, size_t line_width = DEFAULT_LINE_WIDTH) const override;
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Send control commands to a running tsp", u"[options] command ...", GATHER_PARAMETERS),
    cmdline(*this),
    command(),
    tsp_address()
{
    cmdline.setShell(ts::Args::GetAppName(argc, argv));

    option(u"", 0, STRING, 1, UNLIMITED_COUNT);
    help(u"", u"The control command to send to tsp.");

    option(u"tsp", 't', STRING, 1, 1);
    help(u"tsp", u"[address:]port",
         u"Specify the IP address (or host name) and port where the tsp process "
         u"expects control commands (tsp option --control-port). "
         u"If the IP address is omitted, the local host is used. "
         u"This is a required parameter, there is no default.");

    analyze(argc, argv);

    // Build command line.
    ts::UStringVector args;
    getValues(args, u"");
    command.quotedLine(args);

    // Validate the control command. It will be validated inside tsp anyway
    // but let's not send an invalid command. Not all commands can be fully
    // validated outside the context of the tsp, but let's filter most errors.
    if (!cmdline.analyzeCommand(command)) {
        error(u"invalid tsp control command: %s", {command});
    }

    // Resolve tsp address.
    if (!tsp_address.resolve(value(u"tsp"), *this) || !tsp_address.hasPort()) {
        error(u"invalid tsp address or port, use --tsp [address:]port");
    }
    if (!tsp_address.hasAddress()) {
        tsp_address.setAddress(ts::IPv4Address::LocalHost);
    }

    exitOnError();
}

ts::UString Options::getHelpText(HelpFormat format, size_t line_width) const
{
    // Initial text from superclass.
    ts::UString text(Args::getHelpText(format, line_width));

    // If full help, add help for all commands.
    if (format == HELP_FULL) {
        text.append(u"\nControl commands: \n");
        const size_t margin = line_width > 10 ? 2 : 0;
        text.append(cmdline.getAllHelpText(HELP_FULL, line_width - margin).toIndented(margin));
    }
    return text;
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Decode command line.
    Options opt(argc, argv);

    // Open a text connection to the tsp server.
    ts::TelnetConnection conn;
    ts::IPv4SocketAddress addr;
    ts::UString resp;

    if (conn.open(opt) &&
        conn.bind(addr, opt) &&
        conn.connect(opt.tsp_address, opt) &&
        conn.sendLine(opt.command, opt) &&
        conn.closeWriter(opt))
    {
        // Request successfully sent, read the responses.
        while (conn.receiveLine(resp, nullptr, opt)) {
            std::cout << resp << std::endl;
        }
        conn.close(opt);
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
