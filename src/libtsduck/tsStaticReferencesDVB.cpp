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

// Macros to generate a unique symbol name.
#define REF_NAME1(a,b) a##b
#define REF_NAME2(a,b) REF_NAME1(a,b)
#define REF_NAME       REF_NAME2(REF,__LINE__)

// Macros to generate a dummy reference to a type using a local unused instance.
#define REF_TYPE(type) static const ts::type REF_NAME; _refs.push_back(&REF_NAME)

// Macros to generate a dummy reference to a type using an existing instance.
#define REF_OBJECT(obj) _refs.push_back(&ts::obj)

//
// Constructor of the dummy reference-maker.
//
ts::StaticReferencesDVB::StaticReferencesDVB() :
    _refs()
{
    _refs.reserve(100);

    // References to all object files containing DVB character sets.

    REF_OBJECT(DVBCharsetSingleByte::ISO_6937);
    REF_OBJECT(DVBCharsetUTF16::UNICODE);
    REF_OBJECT(DVBCharsetUTF8::UTF_8);

    // References to all DVB tables.

    REF_TYPE(BAT);
    REF_TYPE(CAT);
    REF_TYPE(EIT);
    REF_TYPE(INT);
    REF_TYPE(NIT);
    REF_TYPE(PAT);
    REF_TYPE(PMT);
    REF_TYPE(RST);
    REF_TYPE(SDT);
    REF_TYPE(SpliceInformationTable);
    REF_TYPE(TDT);
    REF_TYPE(TOT);
    REF_TYPE(TSDT);

    // References to all DVB descriptors.

    REF_TYPE(AACDescriptor);
    REF_TYPE(AC3Descriptor);
    REF_TYPE(AdaptationFieldDataDescriptor);
    REF_TYPE(AncillaryDataDescriptor);
    REF_TYPE(ApplicationDescriptor);
    REF_TYPE(ApplicationNameDescriptor);
    REF_TYPE(ApplicationSignallingDescriptor);
    REF_TYPE(ApplicationUsageDescriptor);
    REF_TYPE(AVCVideoDescriptor);
    REF_TYPE(BouquetNameDescriptor);
    REF_TYPE(CableDeliverySystemDescriptor);
    REF_TYPE(CADescriptor);
    REF_TYPE(CAIdentifierDescriptor);
    REF_TYPE(ContentDescriptor);
    REF_TYPE(ComponentDescriptor);
    REF_TYPE(CountryAvailabilityDescriptor);
    REF_TYPE(CPDescriptor);
    REF_TYPE(CPIdentifierDescriptor);
    REF_TYPE(CueIdentifierDescriptor);
    REF_TYPE(DataBroadcastDescriptor);
    REF_TYPE(DataBroadcastIdDescriptor);
    REF_TYPE(DIILocationDescriptor);
    REF_TYPE(DTSDescriptor);
    REF_TYPE(DVBHTMLApplicationBoundaryDescriptor);
    REF_TYPE(DVBHTMLApplicationDescriptor);
    REF_TYPE(DVBHTMLApplicationLocationDescriptor);
    REF_TYPE(DVBJApplicationDescriptor);
    REF_TYPE(DVBJApplicationLocationDescriptor);
    REF_TYPE(EacemPreferredNameIdentifierDescriptor);
    REF_TYPE(EacemPreferredNameListDescriptor);
    REF_TYPE(EacemStreamIdentifierDescriptor);
    REF_TYPE(EnhancedAC3Descriptor);
    REF_TYPE(EutelsatChannelNumberDescriptor);
    REF_TYPE(ExtendedEventDescriptor);
    REF_TYPE(HDSimulcastLogicalChannelDescriptor);
    REF_TYPE(HEVCVideoDescriptor);
    REF_TYPE(IPMACPlatformNameDescriptor);
    REF_TYPE(IPMACPlatformProviderNameDescriptor);
    REF_TYPE(IPMACStreamLocationDescriptor);
    REF_TYPE(IPSignallingDescriptor);
    REF_TYPE(ISO639LanguageDescriptor);
    REF_TYPE(LinkageDescriptor);
    REF_TYPE(LocalTimeOffsetDescriptor);
    REF_TYPE(LogicalChannelNumberDescriptor);
    REF_TYPE(MessageDescriptor);
    REF_TYPE(MultilingualBouquetNameDescriptor);
    REF_TYPE(MultilingualComponentDescriptor);
    REF_TYPE(MultilingualNetworkNameDescriptor);
    REF_TYPE(MultilingualServiceNameDescriptor);
    REF_TYPE(NetworkNameDescriptor);
    REF_TYPE(NVODReferenceDescriptor);
    REF_TYPE(ParentalRatingDescriptor);
    REF_TYPE(PrefetchDescriptor);
    REF_TYPE(PrivateDataIndicatorDescriptor);
    REF_TYPE(PrivateDataSpecifierDescriptor);
    REF_TYPE(RegistrationDescriptor);
    REF_TYPE(S2SatelliteDeliverySystemDescriptor);
    REF_TYPE(SatelliteDeliverySystemDescriptor);
    REF_TYPE(ScramblingDescriptor);
    REF_TYPE(ServiceDescriptor);
    REF_TYPE(ServiceIdentifierDescriptor);
    REF_TYPE(ServiceListDescriptor);
    REF_TYPE(ShortEventDescriptor);
    REF_TYPE(SpliceAvailDescriptor);
    REF_TYPE(SpliceDTMFDescriptor);
    REF_TYPE(SpliceSegmentationDescriptor);
    REF_TYPE(SpliceTimeDescriptor);
    REF_TYPE(SSUDataBroadcastIdDescriptor);
    REF_TYPE(SSULinkageDescriptor);
    REF_TYPE(STDDescriptor);
    REF_TYPE(StreamIdentifierDescriptor);
    REF_TYPE(SubtitlingDescriptor);
    REF_TYPE(SupplementaryAudioDescriptor);
    REF_TYPE(T2MIDescriptor);
    REF_TYPE(TargetIPAddressDescriptor);
    REF_TYPE(TargetIPSlashDescriptor);
    REF_TYPE(TargetIPSourceSlashDescriptor);
    REF_TYPE(TargetMACAddressDescriptor);
    REF_TYPE(TargetMACAddressRangeDescriptor);
    REF_TYPE(TargetSerialNumberDescriptor);
    REF_TYPE(TargetSmartcardDescriptor);
    REF_TYPE(TeletextDescriptor);
    REF_TYPE(TerrestrialDeliverySystemDescriptor);
    REF_TYPE(TimeShiftedEventDescriptor);
    REF_TYPE(TimeShiftedServiceDescriptor);
    REF_TYPE(TransportProtocolDescriptor);
    REF_TYPE(VBIDataDescriptor);
    REF_TYPE(VBITeletextDescriptor);
}
