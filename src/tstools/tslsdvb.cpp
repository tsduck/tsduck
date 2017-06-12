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
#include "tsFormat.h"
#include "tsSysUtils.h"

using namespace ts;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public Args
{
    Options (int argc, char *argv[]);

    std::string device_name;  // Name of device to list (empty means all)
    bool        verbose;      // Verbose output
#if defined(__linux)
    bool        s2api;        // Force usage of S2API
#elif defined(__windows)
    bool        enum_devices; // Enumerate DirectShow devices
#endif
};

Options::Options (int argc, char *argv[]) :
    Args("DVB Devices Listing Utility.", "[options] [device]"),
    device_name(),
    verbose(false)
#if defined(__linux)
    ,
    s2api(false)
#elif defined(__windows)
    ,
    enum_devices(false)
#endif
{
    option ("",          0,  STRING, 0, 1);
    option ("adapter",  'a', UNSIGNED);
    option ("debug",    'd', POSITIVE, 0, 1, 0, 0, true);
    option ("verbose",  'v');
#if defined (__linux)
    option ("frontend", 'f', UNSIGNED); // legacy option
    option ("s2api",    '2');
#elif defined (__windows)
    option ("enumerate-devices", 'e');
#endif

    setHelp ("Device:\n"
             "\n"
#if defined (__linux)
             "  Name of device to list, usually /dev/dvb/adapterA[:F] where\n"
             "  A = adapter number and F = frontend number (default: 0).\n"
             "  The legacy options --adapter and --frontend can also be used\n"
             "  instead of the device name.\n"
#elif defined (__windows)
             "  Name of device to list. This is a DirectShow/BDA tuner filter name\n"
             "  (not case sensitive, blanks are ignored).\n"
#endif
             "  By default, all DVB devices are listed.\n"
             "\n"
             "Options:\n"
             "\n"
             "  -a N\n"
             "  --adapter N\n"
#if defined (__linux)
             "      Specifies the Linux DVB adapter N (/dev/dvb/adapterN).\n"
#elif defined (__windows)
             "      Specifies the Nth DVB adapter in the system.\n"
#endif
             "      This option can be used instead of device name.\n"
             "      Use the tslsdvb utility to list all DVB devices.\n"
             "\n"
#if defined (__windows)
             "  -e\n"
             "  --enumerate-devices\n"
             "      Enumerate all relevant DirectShow devices and filters.\n"
             "      Very verbose output, for debug only.\n"
             "\n"
#endif
#if defined (__linux)
             "  -f N\n"
             "  --frontend N\n"
             "      Specifies the frontend N (/dev/dvb/adapter?/frontendN) in the adapter.\n"
             "      This option can be used instead of device name.\n"
             "\n"
#endif
             "  --help\n"
             "      Display this help text.\n"
             "\n"
#if defined (__linux)
             "  -2\n"
             "  --s2api\n"
             "      On Linux kernel 2.6.28 and higher, this option forces the usage of the\n"
             "      S2API for communication with the DVB drivers. By default, for DVB-C and\n"
             "      DVB-T, the legacy Linux DVB API V3 is still used. The DVB-S and DVB-S2\n"
             "      tuners always use the S2API.\n"
             "\n"
#endif
             "  -v\n"
             "  --verbose\n"
             "      Produce verbose output.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    analyze (argc, argv);

    getValue (device_name);
    verbose = present ("verbose");
    setDebugLevel (present ("debug") ? intValue ("debug", 1) : Severity::Info);

#if defined (__linux)
    s2api = present ("s2api");
    if (present ("adapter") || present ("frontend")) {
        if (device_name.empty()) {
            device_name = Format ("/dev/dvb/adapter%d:%d", intValue ("adapter", 0), intValue ("frontend", 0));
        }
        else {
            error ("--adapter and --frontend cannot be used with device name");
        }
    }
#elif defined (__windows)
    enum_devices = present ("enumerate-devices");
    if (present ("adapter")) {
        if (device_name.empty()) {
            device_name = Format (":%d", intValue ("adapter", 0));
        }
        else {
            error ("--adapter cannot be used with device name");
        }
    }
#endif

    exitOnError();
}


//----------------------------------------------------------------------------
//  This routine lists one tuner device.
//  If tuner_index >= 0, print it (Windows only).
//----------------------------------------------------------------------------

namespace {
    void ListTuner (Tuner& tuner, int tuner_index, Options& opt)
    {
        // If not opened, nothing to display.
        if (!tuner.isOpen()) {
            return;
        }

        // Linux-specific mode
#if defined (__linux)
        tuner.setForceS2API (opt.s2api);
#endif

        // Display name and type
#if defined (__windows)
        if (tuner_index >= 0) {
            std::cout << tuner_index << ": ";
        }
#endif
        const std::string info (tuner.deviceInfo());
        bool something = !info.empty();
        const DeliverySystemSet systems (tuner.deliverySystems());

        std::cout << tuner.deviceName() << " (";
        if (something) {
            std::cout << info;
        }
        for (size_t ds = 0; ds < systems.size(); ++ds) {
            if (systems.test (ds)) {
                if (something) {
                    std::cout << ", ";
                }
                std::cout << DeliverySystemEnum.name(int(ds));
                something = true;
            }
        }
        std::cout << ")" << std::endl;

        // Display verbose information
        if (opt.verbose) {
            std::cout << std::endl;
            tuner.displayStatus (std::cout, "  ", opt);
            std::cout << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
//  Main code. Isolated from main() to ensure that destructors are invoked
//  before COM uninitialize.
//----------------------------------------------------------------------------

namespace {
    void ListMain (Options& opt)
    {
        // List DVB tuner devices
        if (!opt.device_name.empty()) {
            // One device name specified
            Tuner tuner (opt.device_name, true, opt);
            ListTuner (tuner, -1, opt);
        }
        else {
            // List all tuners
            TunerPtrVector tuners;
            if (!Tuner::GetAllTuners (tuners, opt)) {
                return;
            }
            else if (tuners.empty()) {
                opt.error ("no DVB device found");
            }
            else {
                if (opt.verbose) {
                    std::cout << std::endl;
                }
                for (size_t i = 0; i < tuners.size(); ++i) {
                    ListTuner (*tuners[i], tuners.size() == 1 ? -1 : int (i), opt);
                }
            }
        }

        // Enumerate all DirectShow devices on Windows
#if defined (__windows)
        if (opt.enum_devices) {
            Tuner::EnumerateDevices (std::cout, "", opt);
        }
#endif
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);
    COM com (opt);

    if (com.isInitialized()) {
        ListMain (opt);
    }

    opt.exitOnError();
    return EXIT_SUCCESS;
}
