//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
// MacOS implementation of the ts::Tuner class.
//
// WARNING: MacOS support is currently not implemented.
// All methods return errors.
//
//-----------------------------------------------------------------------------

#include "tsTuner.h"
TSDUCK_SOURCE;

#define ERROR(ret) report.error(u"Digital tuners are not implemented on macOS"); return (ret)

// There is nothing in system guts but we need to allocate something.
class ts::Tuner::Guts
{
};

void ts::Tuner::allocateGuts()
{
    _guts = new Guts;
}
void ts::Tuner::deleteGuts()
{
    delete _guts;
}
bool ts::Tuner::GetAllTuners(DuckContext& duck, TunerPtrVector& tuners, Report& report)
{
    ERROR(false);
}
bool ts::Tuner::open(const UString& device_name, bool info_only, Report& report)
{
    ERROR(false);
}
bool ts::Tuner::close(Report& report)
{
    ERROR(false);
}
bool ts::Tuner::signalLocked(Report& report)
{
    ERROR(false);
}
int ts::Tuner::signalStrength(Report& report)
{
    ERROR(-1);
}
int ts::Tuner::signalQuality(Report& report)
{
    ERROR(-1);
}
bool ts::Tuner::getCurrentTuning(ModulationArgs& params, bool reset_unknown, Report& report)
{
    ERROR(false);
}
bool ts::Tuner::tune(ModulationArgs& params, Report& report)
{
    ERROR(false);
}
bool ts::Tuner::start(Report& report)
{
    ERROR(false);
}
bool ts::Tuner::stop(Report& report)
{
    ERROR(false);
}
bool ts::Tuner::setReceiveTimeout(MilliSecond timeout, Report& report)
{
    ERROR(false);
}
size_t ts::Tuner::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report)
{
    ERROR(false);
}
std::ostream& ts::Tuner::displayStatus(std::ostream& strm, const UString& margin, Report& report)
{
    ERROR(strm);
}
