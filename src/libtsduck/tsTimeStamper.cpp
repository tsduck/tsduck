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

#include "tsTimeStamper.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Construction and reset.
//----------------------------------------------------------------------------

ts::TimeStamper::TimeStamper(PID referencePID) :
    _pid(referencePID),
{
    reset();
}

void QtsTimeStamper::reset()
{
    _pid = QTS_PID_NULL;
    _source = UNDEFINED;
    _lastTimeStamp = 0;
    _previousClock = 0;
    _delta = 0;
}

void QtsTimeStamper::setDemux(const QtsDemux* demux)
{
    if (_demux != demux) {
        _demux = demux;
        if (_source == PCR) {
            reset();
        }
    }
}


//----------------------------------------------------------------------------
// Process a new clock value in millisecond.
//----------------------------------------------------------------------------

void QtsTimeStamper::processClock(qint64 clock)
{
    if (_source == UNDEFINED) {
        // Source not yet set. The first timestamp is zero by definition.
        // The first clock value shall be substracted to all subsequent clock values.
        _delta = -clock;
    }
    else if (clock < _previousClock) {
        // Our clock has wrapped up after the max value.
        // The clock has restarted at zero and we must add the last
        // time stamp before wrapping to all subsequent clock values.
        _delta = _lastTimeStamp;
    }
    _lastTimeStamp = qMax<qint64>(0, clock + _delta);
    _previousClock = clock;
}

//----------------------------------------------------------------------------
// Get the last timestamp in milliseconds, starting with zero.
//----------------------------------------------------------------------------

quint64 QtsTimeStamper::lastTimeStamp()
{
    if ((_source == UNDEFINED || _source == PCR) && _demux != 0) {
        const qint64 pcr = _demux->lastPcr();
        if (pcr >= 0) {
            // If previously undefined, our source is now PCR.
            processClock(pcr / (QTS_SYSTEM_CLOCK_FREQ / 1000));
            _source = PCR;
        }
        else {
            // If previously PCR, our source is now undefined (problably a demux reset).
            _source = UNDEFINED;
        }
    }
    return _lastTimeStamp;
}


//----------------------------------------------------------------------------
// Process one PES packet from the reference PID.
//----------------------------------------------------------------------------

void QtsTimeStamper::processPesPacket(const QtsPesPacket& packet)
{
    // If our source is PCR, we ignore all PES packets.
    // If the packet has no PTS, it is useless anyway.
    if (_source == PCR || !packet.hasPts()) {
        return;
    }

    // Check or set the PID.
    if (_pid == QTS_PID_NULL) {
        _pid = packet.getSourcePid();
    }
    else if (packet.getSourcePid() != QTS_PID_NULL && packet.getSourcePid() != _pid) {
        // Not the same PID, reject this packet.
        return;
    }

    // We have a PTS on the right PID, PTS will now be our source (if not already).
    processClock(packet.getPts() / (QTS_SYSTEM_CLOCK_SUBFREQ / 1000));
    _source = PTS;
}
