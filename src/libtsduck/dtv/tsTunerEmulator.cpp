//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#include "tsTunerEmulator.h"
#include "tsDuckContext.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Constructors and destructors.
//-----------------------------------------------------------------------------

ts::TunerEmulator::TunerEmulator(DuckContext& duck) :
    TunerBase(duck),
    _delivery_systems(),
    _file_path(),
    _info_only(false)
{
}

ts::TunerEmulator::~TunerEmulator()
{
}


//-----------------------------------------------------------------------------
// Open the tuner emulator.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::open(const UString& device_name, bool info_only, Report& report)
{
    _file_path = device_name;
    _info_only = info_only;
    return false;  // @@@@@@@@@@@
}


//-----------------------------------------------------------------------------
// Close the tuner emulator.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::close(Report& report)
{
    _delivery_systems.clear();
    _file_path.clear();
    _info_only = false;
    return false;  // @@@@@@@@@@@
}


//-----------------------------------------------------------------------------
// Basic information.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::isOpen() const
{
    return !_file_path.empty();
}

bool ts::TunerEmulator::infoOnly() const
{
    return _info_only;
}

const ts::DeliverySystemSet& ts::TunerEmulator::deliverySystems() const
{
    return _delivery_systems;
}

ts::UString ts::TunerEmulator::deviceName() const
{
    return _file_path;
}

ts::UString ts::TunerEmulator::deviceInfo() const
{
    return _file_path;
}

ts::UString ts::TunerEmulator::devicePath() const
{
    return _file_path;
}


//-----------------------------------------------------------------------------
// Emulated signal characteristics.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::signalLocked(Report& report)
{
    return false;  // @@@@@@@@@@@
}

int ts::TunerEmulator::signalStrength(Report& report)
{
    return 0;  // @@@@@@@@@@@
}

int ts::TunerEmulator::signalQuality(Report& report)
{
    return 0;  // @@@@@@@@@@@
}


//-----------------------------------------------------------------------------
// Tune to a frequency
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::tune(ModulationArgs& params, Report& report)
{
    return false;  // @@@@@@@@@@@
}

bool ts::TunerEmulator::start(Report& report)
{
    return false;  // @@@@@@@@@@@
}

bool ts::TunerEmulator::stop(Report& report)
{
    return false;  // @@@@@@@@@@@
}

size_t ts::TunerEmulator::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report)
{
    return 0;  // @@@@@@@@@@@
}

bool ts::TunerEmulator::getCurrentTuning(ModulationArgs& params, bool reset_unknown, Report& report)
{
    return false;  // @@@@@@@@@@@
}


//-----------------------------------------------------------------------------
// Display the current tuner emulator state.
//-----------------------------------------------------------------------------

std::ostream& ts::TunerEmulator::displayStatus(std::ostream& strm, const UString& margin, Report& report, bool extended)
{
    //@@@@@@
    return strm;
}
