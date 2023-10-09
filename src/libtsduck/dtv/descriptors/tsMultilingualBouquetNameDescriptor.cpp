//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMultilingualBouquetNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"

#define MY_XML_NAME u"multilingual_bouquet_name_descriptor"
#define MY_XML_ATTR u"bouquet_name"
#define MY_CLASS ts::MultilingualBouquetNameDescriptor
#define MY_DID ts::DID_MLINGUAL_BOUQUET

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::MultilingualBouquetNameDescriptor::MultilingualBouquetNameDescriptor() :
    AbstractMultilingualDescriptor(MY_DID, MY_XML_NAME, MY_XML_ATTR)
{
}

ts::MultilingualBouquetNameDescriptor::MultilingualBouquetNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    MultilingualBouquetNameDescriptor()
{
    deserialize(duck, desc);
}

ts::MultilingualBouquetNameDescriptor::~MultilingualBouquetNameDescriptor()
{
}
