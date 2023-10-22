//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCIAncillaryDataDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"CI_ancillary_data_descriptor"
#define MY_CLASS ts::CIAncillaryDataDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_CI_ANCILLARY_DATA
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::CIAncillaryDataDescriptor::CIAncillaryDataDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::CIAncillaryDataDescriptor::CIAncillaryDataDescriptor(DuckContext& duck, const Descriptor& desc) :
    CIAncillaryDataDescriptor()
{
    deserialize(duck, desc);
}

void ts::CIAncillaryDataDescriptor::clearContent()
{
    ancillary_data.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::CIAncillaryDataDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::CIAncillaryDataDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(ancillary_data);
}

void ts::CIAncillaryDataDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBytes(ancillary_data);
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::CIAncillaryDataDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaTextChild(u"ancillary_data", ancillary_data, true);
}

bool ts::CIAncillaryDataDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaTextChild(ancillary_data, u"ancillary_data", false, 0, MAX_DESCRIPTOR_SIZE - 3);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CIAncillaryDataDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp.displayPrivateData(u"Ancillary data", buf, NPOS, margin);
}
