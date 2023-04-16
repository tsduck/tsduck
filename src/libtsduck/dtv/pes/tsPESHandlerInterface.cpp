//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsPESHandlerInterface.h"

// Default implementations are all empty.

void ts::PESHandlerInterface::handlePESPacket(PESDemux&, const PESPacket&) {}
void ts::PESHandlerInterface::handleInvalidPESPacket(PESDemux&, const DemuxedData&) {}
void ts::PESHandlerInterface::handleVideoStartCode(PESDemux&, const PESPacket&, uint8_t, size_t, size_t) {}
void ts::PESHandlerInterface::handleNewMPEG2VideoAttributes(PESDemux&, const PESPacket&, const MPEG2VideoAttributes&) {}
void ts::PESHandlerInterface::handleAccessUnit(PESDemux&, const PESPacket&, uint8_t, size_t, size_t) {}
void ts::PESHandlerInterface::handleSEI(PESDemux&, const PESPacket&, uint32_t, size_t , size_t size) {}
void ts::PESHandlerInterface::handleNewAVCAttributes(PESDemux&, const PESPacket&, const AVCAttributes&) {}
void ts::PESHandlerInterface::handleNewHEVCAttributes(PESDemux&, const PESPacket&, const HEVCAttributes&) {}
void ts::PESHandlerInterface::handleIntraImage(PESDemux&, const PESPacket&, size_t) {}
void ts::PESHandlerInterface::handleNewMPEG2AudioAttributes(PESDemux&, const PESPacket&, const MPEG2AudioAttributes&) {}
void ts::PESHandlerInterface::handleNewAC3Attributes(PESDemux&, const PESPacket&, const AC3Attributes&) {}
