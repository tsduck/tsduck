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

void ts::mcast::FluteHandlerInterface::handleFluteFile(const FluteFile&)
{
}

void ts::mcast::FluteHandlerInterface::handleFluteFDT(const FluteFDT&)
{
}

void ts::mcast::FluteHandlerInterface::handleFluteNACI(const NIPActualCarrierInformation&)
{
}
