//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFluteHandlerInterface.h"

ts::FluteHandlerInterface::~FluteHandlerInterface()
{
}

void ts::FluteHandlerInterface::handleFluteFile(FluteDemux&, const FluteFile&)
{
}

void ts::FluteHandlerInterface::handleFluteFDT(FluteDemux&, const FluteFDT&)
{
}

void ts::FluteHandlerInterface::handleFluteNACI(FluteDemux&, const NIPActualCarrierInformation&)
{
}

void ts::FluteHandlerInterface::handleFluteStatus(FluteDemux&, const FluteSessionId&, const UString&, const UString&, uint64_t, uint64_t, uint64_t)
{
}
