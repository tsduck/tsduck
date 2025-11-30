//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFluteHandlerInterface.h"

ts::mcast::FluteHandlerInterface::~FluteHandlerInterface()
{
}

void ts::mcast::FluteHandlerInterface::handleFluteFile(FluteDemux&, const FluteFile&)
{
}

void ts::mcast::FluteHandlerInterface::handleFluteFDT(FluteDemux&, const FluteFDT&)
{
}

void ts::mcast::FluteHandlerInterface::handleFluteNACI(FluteDemux&, const NIPActualCarrierInformation&)
{
}

void ts::mcast::FluteHandlerInterface::handleFluteStatus(FluteDemux&, const FluteSessionId&, const UString&, const UString&, uint64_t, uint64_t, uint64_t)
{
}
