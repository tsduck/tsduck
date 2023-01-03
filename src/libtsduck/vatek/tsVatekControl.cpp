//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Vision Advance Technology Inc. (VATek)
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

#include "tsVatekControl.h"

#if !defined(TS_NO_VATEK)
#include "tsBeforeStandardHeaders.h"
#include <vatek_sdk_device.h>
#include <core/ui/ui_props_api.h>
#include <core/ui/ui_props/ui_props_chip.h>
#include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::VatekControl::VatekControl(int argc, char *argv[]) :
    Args(u"Control VATek devices", u"[options] [device]"),
    _duck(this),
    _dev_index(-1)
{
    option(u"", 0, UNSIGNED, 0, 1);
    help(u"",
         u"Device index, from 0 to N-1 (with N being the number of VATek devices in the system). "
         u"The default is 0. Use option --all to have a complete list of devices in the system.");

    option(u"all", 'a');
    help(u"all", u"List all VATek devices available on the system.");

    analyze(argc, argv);

    const bool all = present(u"all");
    getIntValue(_dev_index, u"", all ? -1 : 0);
    if (all && _dev_index >= 0) {
        error(u"specifying a device index and --all are mutually exclusive");
    }

    exitOnError();
}

ts::VatekControl::~VatekControl()
{
}


//----------------------------------------------------------------------------
// Execute the Vatek control command.
//----------------------------------------------------------------------------

int ts::VatekControl::execute()
{
#if defined(TS_NO_VATEK)

    error(u"This version of TSDuck was compiled without VATek support");
    return EXIT_FAILURE;

#else

    hvatek_devices hdevices = nullptr;
    vatek_result status = vatek_device_list_enum(DEVICE_BUS_USB, service_transform, &hdevices);
    const int32_t device_count = int32_t(status);

    if (!is_vatek_success(status)) {
        error(u"enumeration VATek device fail, status: %d", {status});
        return EXIT_FAILURE;
    }
    else if (device_count < 1) {
        info(u"No VATek device found");
        return EXIT_SUCCESS;
    }
    else if (_dev_index >= device_count) {
        error(u"invalid device index %d, only %d devices in the system", {_dev_index, device_count});
        return EXIT_FAILURE;
    }

    if (_dev_index < 0) {
        // List all devices.
        std::cout << "Found " << device_count << " VATek devices" << std::endl;
        for (int32_t i = 0; i < device_count; i++) {
            std::cout << " - Device " << i << ": " << vatek_device_list_get_name(hdevices, i) << std::endl;
        }
    }
    else {
        // Display information on one device.
        hvatek_chip hchip = nullptr;
        status = vatek_device_open(hdevices, _dev_index, &hchip);
        if (!is_vatek_success(status)) {
            error(u"open VATek device fail, status: %d", {status});
            return EXIT_FAILURE;
        }
        std::cout << "Device " << _dev_index << ": " << vatek_device_get_name(hchip) << std::endl << std::flush;
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(cast-qual)
        TS_LLVM_NOWARNING(old-style-cast)
        ui_props_printf(" - [%-20s] : %-8s - %s\r\n", nullptr, _ui_struct(chip_info), (uint8_t*)vatek_device_get_info(hchip));
        fflush(stdout);
        TS_POP_WARNING()
    }

    vatek_device_list_free(hdevices);
    return EXIT_SUCCESS;

#endif // TS_NO_VATEK
}
