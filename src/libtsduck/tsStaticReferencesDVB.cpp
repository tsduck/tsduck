//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsStaticReferencesDVB.h"
#include "tsTables.h"
#include "tsDVBCharsetSingleByte.h"
#include "tsDVBCharsetUTF16.h"
#include "tsDVBCharsetUTF8.h"
TSDUCK_SOURCE;

//
// Macros to generate a dummy reference to a type.
//
#define REF_NAME1(a,b) a##b
#define REF_NAME2(a,b) REF_NAME1(a,b)
#define REF_NAME       REF_NAME2(REF,__LINE__)
#define REF(type)      static TS_UNUSED const type REF_NAME

//
// Constructor of the dummy reference-maker.
//
ts::StaticReferencesDVB::StaticReferencesDVB() :
    _refs()
{
    _refs.reserve(20);

    // References to all object files containing DVB character sets.

    _refs.push_back(&ts::DVBCharsetSingleByte::ISO_6937);
    _refs.push_back(&ts::DVBCharsetUTF16::UNICODE);
    _refs.push_back(&ts::DVBCharsetUTF8::UTF_8);

    // References to all DVB tables.

    REF(ts::BAT);
    REF(ts::CAT);
    REF(ts::EIT);
    REF(ts::NIT);
    REF(ts::PAT);
    REF(ts::PMT);
    REF(ts::RST);
    REF(ts::SDT);
    REF(ts::TDT);
    REF(ts::TOT);
    REF(ts::TSDT);

    // References to all DVB descriptors.

    REF(ts::AACDescriptor);
    REF(ts::AC3Descriptor);
    REF(ts::ApplicationSignallingDescriptor);
    REF(ts::AVCVideoDescriptor);
    REF(ts::BouquetNameDescriptor);
    REF(ts::CableDeliverySystemDescriptor);
    REF(ts::CADescriptor);
    REF(ts::CAIdentifierDescriptor);
    REF(ts::ContentDescriptor);
    REF(ts::ComponentDescriptor);
    REF(ts::CountryAvailabilityDescriptor);
    REF(ts::DataBroadcastDescriptor);
    REF(ts::DataBroadcastIdDescriptor);
    REF(ts::DTSDescriptor);
    REF(ts::EacemPreferredNameIdentifierDescriptor);
    REF(ts::EacemPreferredNameListDescriptor);
    REF(ts::EacemStreamIdentifierDescriptor);
    REF(ts::EnhancedAC3Descriptor);
    REF(ts::EutelsatChannelNumberDescriptor);
    REF(ts::ExtendedEventDescriptor);
    REF(ts::HDSimulcastLogicalChannelDescriptor);
    REF(ts::ISO639LanguageDescriptor);
    REF(ts::LinkageDescriptor);
    REF(ts::LocalTimeOffsetDescriptor);
    REF(ts::LogicalChannelNumberDescriptor);
    REF(ts::MessageDescriptor);
    REF(ts::NetworkNameDescriptor);
    REF(ts::ParentalRatingDescriptor);
    REF(ts::PrivateDataSpecifierDescriptor);
    REF(ts::S2SatelliteDeliverySystemDescriptor);
    REF(ts::SatelliteDeliverySystemDescriptor);
    REF(ts::ServiceDescriptor);
    REF(ts::ServiceListDescriptor);
    REF(ts::ShortEventDescriptor);
    REF(ts::SSUDataBroadcastIdDescriptor);
    REF(ts::SSULinkageDescriptor);
    REF(ts::STDDescriptor);
    REF(ts::StreamIdentifierDescriptor);
    REF(ts::SubtitlingDescriptor);
    REF(ts::SupplementaryAudioDescriptor);
    REF(ts::T2MIDescriptor);
    REF(ts::TeletextDescriptor);
    REF(ts::TerrestrialDeliverySystemDescriptor);
    REF(ts::VBIDataDescriptor);
    REF(ts::VBITeletextDescriptor);
}
