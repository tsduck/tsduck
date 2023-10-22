//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSignalizationHandlerInterface.h"

// All default handlers do nothing.

void ts::SignalizationHandlerInterface::handlePAT(const PAT&, PID) {}
void ts::SignalizationHandlerInterface::handleCAT(const CAT&, PID) {}
void ts::SignalizationHandlerInterface::handlePMT(const PMT&, PID) {}
void ts::SignalizationHandlerInterface::handleTSDT(const TSDT&, PID) {}
void ts::SignalizationHandlerInterface::handleNIT(const NIT&, PID) {}
void ts::SignalizationHandlerInterface::handleSDT(const SDT&, PID) {}
void ts::SignalizationHandlerInterface::handleBAT(const BAT&, PID) {}
void ts::SignalizationHandlerInterface::handleRST(const RST&, PID) {}
void ts::SignalizationHandlerInterface::handleTDT(const TDT&, PID) {}
void ts::SignalizationHandlerInterface::handleTOT(const TOT&, PID) {}
void ts::SignalizationHandlerInterface::handleMGT(const MGT&, PID) {}
void ts::SignalizationHandlerInterface::handleVCT(const VCT&, PID) {}
void ts::SignalizationHandlerInterface::handleCVCT(const CVCT&, PID) {}
void ts::SignalizationHandlerInterface::handleTVCT(const TVCT&, PID) {}
void ts::SignalizationHandlerInterface::handleRRT(const RRT&, PID) {}
void ts::SignalizationHandlerInterface::handleSTT(const STT&, PID) {}
void ts::SignalizationHandlerInterface::handleUTC(const Time&, TID) {}
void ts::SignalizationHandlerInterface::handleSAT(const SAT&, PID) {}
void ts::SignalizationHandlerInterface::handleService(uint16_t, const Service&, const PMT&, bool) {}
ts::SignalizationHandlerInterface::~SignalizationHandlerInterface() {}
