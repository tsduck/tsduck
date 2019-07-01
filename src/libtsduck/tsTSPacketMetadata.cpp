//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------

#include "tsTSPacketMetadata.h"
TSDUCK_SOURCE;

const ts::TSPacketMetadata::LabelSet ts::TSPacketMetadata::NoLabel;
const ts::TSPacketMetadata::LabelSet ts::TSPacketMetadata::AllLabels(~NoLabel);


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSPacketMetadata::TSPacketMetadata() :
    _labels(),
    _flush(false),
    _bitrate_changed(false),
    _input_stuffing(false),
    _nullified(false)
{
}


//----------------------------------------------------------------------------
// Reset the content of this instance.
//----------------------------------------------------------------------------

void ts::TSPacketMetadata::reset()
{
    _labels.reset();
    _flush = false;
    _bitrate_changed = false;
    _input_stuffing = false;
    _nullified = false;
}


//----------------------------------------------------------------------------
// Label operations
//----------------------------------------------------------------------------

bool ts::TSPacketMetadata::hasLabel(size_t label) const
{
    return label < _labels.size() && _labels.test(label);
}

bool ts::TSPacketMetadata::hasAnyLabel(const LabelSet& mask) const
{
    return (_labels & mask).any(); 
}

bool ts::TSPacketMetadata::hasAllLabels(const LabelSet& mask) const
{
    return (_labels & mask) == mask; 
}

void ts::TSPacketMetadata::setLabels(const LabelSet& mask)
{
    _labels |= mask;
}

void ts::TSPacketMetadata::clearLabels(const LabelSet& mask)
{
    _labels &= ~mask;
}
