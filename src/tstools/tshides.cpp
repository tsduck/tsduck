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
//  Control HiDes modulator devices.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsVersionInfo.h"
#include "tsHiDesDevice.h"
#include "tsCOM.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

class HiDesOptions: public ts::Args
{
public:
    HiDesOptions(int argc, char *argv[]);
};

// Constructor.
HiDesOptions::HiDesOptions(int argc, char *argv[]) :
    ts::Args(u"List HiDes modulator devices", u"[options]")
{
    setHelp(u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    analyze(argc, argv);
    exitOnError();
}


//----------------------------------------------------------------------------
//  Main code. Isolated from main() to ensure that destructors are invoked
//  before COM uninitialize.
//----------------------------------------------------------------------------

namespace {
    void MainCode(HiDesOptions& opt)
    {
        // Get all HiDes devices.
        ts::HiDesDevice::InfoList devices;
        if (ts::HiDesDevice::GetAllDevices(devices, opt)) {
            if (devices.empty()) {
                std::cout << "No HiDes device found" << std::endl;
            }
            else {
                std::cout << "Found " << devices.size() << " HiDes devices" << std::endl;
                for (auto dev = devices.begin(); dev != devices.end(); ++dev) {
                    std::cout << std::endl
                              << ts::UString::Format(u"%2d: \"%s\"", {dev->index, dev->name})
                              << std::endl;
                }
                std::cout << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    TSDuckLibCheckVersion();
    HiDesOptions opt(argc, argv);
    ts::COM com(opt);

    if (com.isInitialized()) {
        MainCode(opt);
    }

    opt.exitOnError();
    return EXIT_SUCCESS;
}
