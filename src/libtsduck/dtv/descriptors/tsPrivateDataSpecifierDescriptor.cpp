//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPrivateDataSpecifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"private_data_specifier_descriptor"
#define MY_CLASS ts::PrivateDataSpecifierDescriptor
#define MY_DID ts::DID_PRIV_DATA_SPECIF
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PrivateDataSpecifierDescriptor::PrivateDataSpecifierDescriptor(PDS pds_) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    pds(pds_)
{
}

ts::PrivateDataSpecifierDescriptor::PrivateDataSpecifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    PrivateDataSpecifierDescriptor()
{
    deserialize(duck, desc);
}

void ts::PrivateDataSpecifierDescriptor::clearContent()
{
    pds = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PrivateDataSpecifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(pds);
}

void ts::PrivateDataSpecifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    pds = buf.getUInt32();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::PrivateDataSpecifierDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "Specifier: " << names::PrivateDataSpecifier(buf.getUInt32(), NamesFlags::FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PrivateDataSpecifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntEnumAttribute(PrivateDataSpecifierEnum, u"private_data_specifier", pds);
}

bool ts::PrivateDataSpecifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntEnumAttribute(pds, PrivateDataSpecifierEnum, u"private_data_specifier", true);
}
