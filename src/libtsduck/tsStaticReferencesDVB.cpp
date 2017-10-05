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

#pragma once
#include "tsStaticReferencesDVB.h"
#include "tsTables.h"
#include "tsDVBCharsetSingleByte.h"
#include "tsDVBCharsetUTF16.h"
#include "tsDVBCharsetUTF8.h"
TSDUCK_SOURCE;

//
// A class which forces a reference to a data without being optimized away by the compiler.
//
namespace {
    class RefClass
    {
    public:
        const void* cs;
        RefClass(const void* _cs) : cs(_cs) {}
    };
}

//
// Macros to generate a dummy reference to a type in ts namespace or a static instance of a type.
//
#define REF_NAME1(a,b) a##b
#define REF_NAME2(a,b) REF_NAME1(a,b)
#define REF_NAME       REF_NAME2(REF,__LINE__)

#define REF_TYPE(type) static const ts::type REF_NAME
#define REF_DATA(data) static const RefClass REF_NAME(&ts::data)

//
// Constructor of the dummy reference-maker.
//
ts::StaticReferencesDVB::StaticReferencesDVB()
{
    // References to all DVB character sets.

    REF_DATA(DVBCharsetSingleByte::ISO_6937);
    REF_DATA(DVBCharsetUTF16::UNICODE);
    REF_DATA(DVBCharsetUTF8::UTF_8);

    // References to all DVB tables.

    REF_TYPE(BAT);
    REF_TYPE(CAT);
    REF_TYPE(EIT);
    REF_TYPE(NIT);
    REF_TYPE(PAT);
    REF_TYPE(PMT);
    REF_TYPE(RST);
    REF_TYPE(SDT);
    REF_TYPE(TDT);
    REF_TYPE(TOT);
    REF_TYPE(TSDT);

    // References to all DVB descriptors.

    REF_TYPE(AACDescriptor);
    REF_TYPE(AC3Descriptor);
    REF_TYPE(ApplicationSignallingDescriptor);
    REF_TYPE(AVCVideoDescriptor);
    REF_TYPE(BouquetNameDescriptor);
    REF_TYPE(CableDeliverySystemDescriptor);
    REF_TYPE(CADescriptor);
    REF_TYPE(CAIdentifierDescriptor);
    REF_TYPE(ContentDescriptor);
    REF_TYPE(ComponentDescriptor);
    REF_TYPE(CountryAvailabilityDescriptor);
    REF_TYPE(DataBroadcastDescriptor);
    REF_TYPE(DataBroadcastIdDescriptor);
    REF_TYPE(DTSDescriptor);
    REF_TYPE(EacemPreferredNameIdentifierDescriptor);
    REF_TYPE(EacemPreferredNameListDescriptor);
    REF_TYPE(EacemStreamIdentifierDescriptor);
    REF_TYPE(EnhancedAC3Descriptor);
    REF_TYPE(EutelsatChannelNumberDescriptor);
    REF_TYPE(ExtendedEventDescriptor);
    REF_TYPE(HDSimulcastLogicalChannelDescriptor);
    REF_TYPE(ISO639LanguageDescriptor);
    REF_TYPE(LinkageDescriptor);
    REF_TYPE(LocalTimeOffsetDescriptor);
    REF_TYPE(LogicalChannelNumberDescriptor);
    REF_TYPE(MessageDescriptor);
    REF_TYPE(NetworkNameDescriptor);
    REF_TYPE(ParentalRatingDescriptor);
    REF_TYPE(PrivateDataSpecifierDescriptor);
    REF_TYPE(S2SatelliteDeliverySystemDescriptor);
    REF_TYPE(SatelliteDeliverySystemDescriptor);
    REF_TYPE(ServiceDescriptor);
    REF_TYPE(ServiceListDescriptor);
    REF_TYPE(ShortEventDescriptor);
    REF_TYPE(SSUDataBroadcastIdDescriptor);
    REF_TYPE(SSULinkageDescriptor);
    REF_TYPE(STDDescriptor);
    REF_TYPE(StreamIdentifierDescriptor);
    REF_TYPE(SubtitlingDescriptor);
    REF_TYPE(SupplementaryAudioDescriptor);
    REF_TYPE(TeletextDescriptor);
    REF_TYPE(TerrestrialDeliverySystemDescriptor);
    REF_TYPE(VBIDataDescriptor);
    REF_TYPE(VBITeletextDescriptor);
}
