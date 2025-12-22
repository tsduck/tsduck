//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNIPHandlerInterface.h"

ts::mcast::NIPHandlerInterface::~NIPHandlerInterface()
{
}

void ts::mcast::NIPHandlerInterface::handleNetworkInformationFile(const NetworkInformationFile&)
{
}

void ts::mcast::NIPHandlerInterface::handleServiceInformationFile(const ServiceInformationFile&)
{
}

void ts::mcast::NIPHandlerInterface::handleGatewayConfiguration(const GatewayConfiguration&)
{
}

void ts::mcast::NIPHandlerInterface::handleServiceListEntryPoints(const ServiceListEntryPoints&)
{
}

void ts::mcast::NIPHandlerInterface::handleServiceList(const ServiceList&)
{
}

void ts::mcast::NIPHandlerInterface::handleNewService(const NIPService&)
{
}
