//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//
//  DVB tuner. MacOS implementation.
//
//  WARNING: MacOS support is currently not implemented and all methods
//  return errors.
//
//-----------------------------------------------------------------------------

#include "tsTuner.h"
TSDUCK_SOURCE;

#define NOT_IMPLEMENTED u"DVB tuners are not implemented on macOS"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const ts::MilliSecond ts::Tuner::DEFAULT_SIGNAL_TIMEOUT;
#endif


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

ts::Tuner::~Tuner()
{
}


//-----------------------------------------------------------------------------
// Default constructor,
//-----------------------------------------------------------------------------

ts::Tuner::Tuner(const UString& device_name) :
    _is_open(false),
    _info_only(true),
    _tuner_type(DVB_T),
    _device_name(device_name),
    _device_info(),
    _signal_timeout(DEFAULT_SIGNAL_TIMEOUT),
    _signal_timeout_silent(false),
    _receive_timeout(0),
    _delivery_systems()
{
}


//-----------------------------------------------------------------------------
// Constructor from one device name.
//-----------------------------------------------------------------------------

ts::Tuner::Tuner(const UString& device_name, bool info_only, Report& report) :
    Tuner(device_name)
{
    report.error(NOT_IMPLEMENTED);
}


//-----------------------------------------------------------------------------
// Get the list of all existing DVB tuners.
//-----------------------------------------------------------------------------

bool ts::Tuner::GetAllTuners(TunerPtrVector& tuners, Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Open the tuner.
//-----------------------------------------------------------------------------

// Flawfinder: ignore: this is our open(), not ::open().
bool ts::Tuner::open(const UString& device_name, bool info_only, Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Close tuner.
//-----------------------------------------------------------------------------

bool ts::Tuner::close(Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Check if a signal is present and locked
//-----------------------------------------------------------------------------

bool ts::Tuner::signalLocked(Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Return signal strength, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalStrength(Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return -1;
}


//-----------------------------------------------------------------------------
// Return signal quality, in percent (0=bad, 100=good)
// Return a negative value on error.
//-----------------------------------------------------------------------------

int ts::Tuner::signalQuality(Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return -1;
}


//-----------------------------------------------------------------------------
// Get the current tuning parameters
//-----------------------------------------------------------------------------

bool ts::Tuner::getCurrentTuning(TunerParameters& params, bool reset_unknown, Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Tune to the specified parameters and start receiving.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::tune(const TunerParameters& params, Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Start receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::start(Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Stop receiving packets.
// Return true on success, false on errors
//-----------------------------------------------------------------------------

bool ts::Tuner::stop(Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Timeout for receive operation (none by default).
// If zero, no timeout is applied.
// Return true on success, false on errors.
//-----------------------------------------------------------------------------

bool ts::Tuner::setReceiveTimeout(MilliSecond timeout, Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Read complete 188-byte TS packets in the buffer and return the
// number of actually received packets (in the range 1 to max_packets).
// Returning zero means error or end of input.
//-----------------------------------------------------------------------------

size_t ts::Tuner::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return false;
}


//-----------------------------------------------------------------------------
// Display the characteristics and status of the tuner.
//-----------------------------------------------------------------------------

std::ostream& ts::Tuner::displayStatus(std::ostream& strm, const UString& margin, Report& report)
{
    report.error(NOT_IMPLEMENTED);
    return strm;
}
