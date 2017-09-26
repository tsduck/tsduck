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
// This file is used only when linking the unitary tests against the tsduck
// static library.
//
// This file creates dummy references to table and descriptor classes to
// force their code into the executables. This is required by the way
// these classes register themselves into the deserialization system.
// Without at least one explicit reference, a table or descriptor class
// is not included in the executable and does not register itself. When
// a table or descriptor of that type is found in a binary stream or in
// an XML file, it cannot be analyzed
//
//----------------------------------------------------------------------------

#include "tsTables.h"
TSDUCK_SOURCE;

#define REF_NAME1(a,b) a##b
#define REF_NAME2(a,b) REF_NAME1(a,b)
#define REF_NAME()     REF_NAME2(REF,__LINE__)
#define REF(type)      namespace { const ts::type REF_NAME(); }

REF(BAT)
REF(CAT)
REF(EIT)
REF(NIT)
REF(PAT)
REF(PMT)
REF(RST)
REF(SDT)
REF(TDT)
REF(TOT)
REF(TSDT)

REF(AACDescriptor)
REF(AC3Descriptor)
REF(ApplicationSignallingDescriptor)
REF(AVCVideoDescriptor)
REF(BouquetNameDescriptor)
REF(CableDeliverySystemDescriptor)
REF(CADescriptor)
REF(CAIdentifierDescriptor)
REF(ContentDescriptor)
REF(ComponentDescriptor)
REF(CountryAvailabilityDescriptor)
REF(DataBroadcastDescriptor)
REF(DataBroadcastIdDescriptor)
REF(DTSDescriptor)
REF(EacemPreferredNameIdentifierDescriptor)
REF(EacemPreferredNameListDescriptor)
REF(EacemStreamIdentifierDescriptor)
REF(EnhancedAC3Descriptor)
REF(EutelsatChannelNumberDescriptor)
REF(ExtendedEventDescriptor)
REF(HDSimulcastLogicalChannelDescriptor)
REF(ISO639LanguageDescriptor)
REF(LinkageDescriptor)
REF(LocalTimeOffsetDescriptor)
REF(LogicalChannelNumberDescriptor)
REF(MessageDescriptor)
REF(NetworkNameDescriptor)
REF(ParentalRatingDescriptor)
REF(PrivateDataSpecifierDescriptor)
REF(S2SatelliteDeliverySystemDescriptor)
REF(SatelliteDeliverySystemDescriptor)
REF(ServiceDescriptor)
REF(ServiceListDescriptor)
REF(ShortEventDescriptor)
REF(SSUDataBroadcastIdDescriptor)
REF(SSULinkageDescriptor)
REF(STDDescriptor)
REF(StreamIdentifierDescriptor)
REF(SubtitlingDescriptor)
REF(SupplementaryAudioDescriptor)
REF(TeletextDescriptor)
REF(TerrestrialDeliverySystemDescriptor)
REF(VBIDataDescriptor)
REF(VBITeletextDescriptor)
