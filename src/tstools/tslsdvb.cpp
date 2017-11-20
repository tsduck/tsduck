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
//  List DVB devices characteristics.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsCOM.h"
#include "tsTuner.h"
#include "tsTunerArgs.h"
#include "tsFormat.h"
#include "tsStringUtils.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

#if defined(TS_WINDOWS)
    #include "tsDirectShowTest.h"
#endif


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    ts::TunerArgs                tuner;      // Name of device to list (unspecified means all).
#if defined(TS_WINDOWS)
    ts::DirectShowTest::TestType test_type;  // DirectShow test (Windows only).
#endif
    bool                         verbose;    // Verbose output
};

Options::Options (int argc, char *argv[]) :
    ts::Args("DVB Devices Listing Utility.", "[options]"),
    tuner(true, true),
#if defined(TS_WINDOWS)
    test_type(ts::DirectShowTest::NONE),
#endif
    verbose(false)
{
    option(u"debug", 0, POSITIVE, 0, 1, 0, 0, true);
    option(u"verbose", 'v');
#if defined(TS_WINDOWS)
    option(u"enumerate-devices", 'e');  // Legacy, not documented anymore
    option(u"test", 't', ts::DirectShowTest::TestNames);
#endif

    setHelp(u"By default, without device name or adapter, all DVB devices are listed.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
#if defined(TS_WINDOWS)
            u"  -t name\n"
            u"  --test name\n"
            u"      Run a specific DirectShow test. Very verbose output, for debug only.\n"
            u"      The default is none. The names of the available tests are:\n"
            u"      " + ts::DirectShowTest::TestNames.nameList() + ".\n"
            u"\n"
#endif
            u"  -v\n"
            u"  --verbose\n"
            u"      Produce verbose output.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    // Add common tuner options.
    tuner.defineOptions(*this);
    tuner.addHelp(*this);

    // Analyze command line options.
    analyze(argc, argv);
    tuner.load(*this);

    verbose = present(u"verbose");
    setDebugLevel(present(u"debug") ? intValue("debug", 1) : ts::Severity::Info);

#if defined(TS_WINDOWS)
    // Test options on Windows. The legacy option "--enumerate-devices" means "--test enumerate-devices".
    test_type = enumValue("test", present(u"enumerate-devices") ? ts::DirectShowTest::ENUMERATE_DEVICES : ts::DirectShowTest::NONE);
#endif

    exitOnError();
}


//----------------------------------------------------------------------------
//  This routine lists one tuner device.
//  If tuner_index >= 0, print it (Windows only).
//----------------------------------------------------------------------------

namespace {
    void ListTuner(ts::Tuner& tuner, int tuner_index, Options& opt)
    {
        // If not opened, nothing to display.
        if (!tuner.isOpen()) {
            return;
        }

        // Get tuner information.
        const std::string info(tuner.deviceInfo());
        bool something = !info.empty();
        const ts::DeliverySystemSet systems(tuner.deliverySystems());

        // Display name. On Windows, since names are weird, always display
        // the adapter number and use quotes around tuner name.
#if defined(TS_WINDOWS)
        if (tuner_index >= 0) {
            std::cout << tuner_index << ": ";
        }
        std::cout << '"';
#endif
        std::cout << tuner.deviceName();
#if defined(TS_WINDOWS)
        std::cout << '"';
#endif

        // Display tuner information.
        std::cout << " (";
        if (something) {
            std::cout << info;
        }
        for (size_t ds = 0; ds < systems.size(); ++ds) {
            if (systems.test(ds)) {
                if (something) {
                    std::cout << ", ";
                }
                std::cout << ts::DeliverySystemEnum.name(int(ds));
                something = true;
            }
        }
        std::cout << ")" << std::endl;

        // Display verbose information
        if (opt.verbose) {
            std::cout << std::endl;
            tuner.displayStatus(std::cout, "  ", opt);
            std::cout << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
//  Main code. Isolated from main() to ensure that destructors are invoked
//  before COM uninitialize.
//----------------------------------------------------------------------------

namespace {
    void ListMain(Options& opt)
    {
        // List DVB tuner devices
        if (!opt.tuner.device_name.empty()) {
            // One device name specified.
            ts::Tuner tuner(opt.tuner.device_name, true, opt);
            ListTuner(tuner, -1, opt);
        }
        else {
            // List all tuners.
            ts::TunerPtrVector tuners;
            if (!ts::Tuner::GetAllTuners(tuners, opt)) {
                return;
            }
            else if (tuners.empty()) {
                opt.error("no DVB device found");
            }
            else {
                if (opt.verbose) {
                    std::cout << std::endl;
                }
                for (size_t i = 0; i < tuners.size(); ++i) {
                    ListTuner(*tuners[i], int(i), opt);
                }
            }
        }

#if defined (TS_WINDOWS)
        // Specific DirectShow tests on Windows.
        ts::DirectShowTest ds(std::cout, opt);
        ds.runTest(opt.test_type);
#endif
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    Options opt(argc, argv);
    ts::COM com(opt);

    if (com.isInitialized()) {
        ListMain(opt);
    }

    opt.exitOnError();
    return EXIT_SUCCESS;
}
