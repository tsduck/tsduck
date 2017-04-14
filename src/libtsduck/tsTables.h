//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  All headers for MPEG/DVB tables and descriptors.
//
//----------------------------------------------------------------------------

#pragma once

// Tables

#include "tsBAT.h"
#include "tsCAT.h"
#include "tsDMT.h"
#include "tsNIT.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsTDT.h"
#include "tsTOT.h"
#include "tsTSDT.h"

// Descriptors

#include "tsAC3Descriptor.h"
#include "tsBouquetNameDescriptor.h"
#include "tsCADescriptor.h"
#include "tsCAIdentifierDescriptor.h"
#include "tsContentDescriptor.h"
#include "tsCountryAvailabilityDescriptor.h"
#include "tsDataBroadcastIdDescriptor.h"
#include "tsEnhancedAC3Descriptor.h"
#include "tsExtendedEventDescriptor.h"
#include "tsHDSimulcastLogicalChannelDescriptor.h"
#include "tsISO639LanguageDescriptor.h"
#include "tsLinkageDescriptor.h"
#include "tsLogicalChannelNumberDescriptor.h"
#include "tsNetworkNameDescriptor.h"
#include "tsParentalRatingDescriptor.h"
#include "tsPrivateDataSpecifierDescriptor.h"
#include "tsServiceDescriptor.h"
#include "tsServiceListDescriptor.h"
#include "tsShortEventDescriptor.h"
#include "tsSSUDataBroadcastIdDescriptor.h"
#include "tsSSULinkageDescriptor.h"
#include "tsSTDDescriptor.h"
#include "tsStreamIdentifierDescriptor.h"
#include "tsTerrestrialDeliverySystemDescriptor.h"
