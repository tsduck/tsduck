//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
ts::PESHandlerInterface::~PESHandlerInterface() {}
