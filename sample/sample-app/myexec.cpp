//----------------------------------------------------------------------------
//
// TSDuck sample application running a chain of plugins.
//
// To run this test, you may need to define the following:
//   export TSPLUGINS_PATH=/usr/lib/tsduck
//
// Notes:
//   - With TSDuck 3.21 and below, use export TSPLUGINS_PATH=/usr/bin
//   - With TSDuck 3.22 and above, use export TSPLUGINS_PATH=/usr/lib/tsduck
//   - Use /usr/local instead of /usr on macOS.
//   - On some 64-bit systems, use lib64 instead of lib.
//
//----------------------------------------------------------------------------

#include "tsduck.h"

int main(int argc, char* argv[])
{
    // Use an asynchronous logger to report errors, logs, debug, etc.
    ts::AsyncReport report;

    // Build tsp options. Accept most default values, except a few ones.
    ts::TSProcessorArgs opt;
    opt.app_name = u"myexec";  // for error messages only.

    // Input plugin. Here, read an IP multicast stream.
    opt.input = {u"ip", {u"230.1.2.3:5555"}};

    // Packet processing plugins. Here, stop processig after 1000 TS packets.
    opt.plugins = {
        {u"until", {u"--packet", u"1000"}},
    };

    // Output pluging. Here, store in a local file.
    opt.output = {u"file", {u"output.ts"}};

    // The TS processing is performed into this object.
    ts::TSProcessor tsproc(report);

    // Start the TS processing.
    if (!tsproc.start(opt)) {
        return EXIT_FAILURE;
    }

    // And wait for TS processing termination.
    tsproc.waitForTermination();
    return EXIT_SUCCESS;
}
