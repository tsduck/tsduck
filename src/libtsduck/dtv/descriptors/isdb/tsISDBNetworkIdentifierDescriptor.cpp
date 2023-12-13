//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBNetworkIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"ISDB_network_identifier_descriptor"
#define MY_CLASS ts::ISDBNetworkIdentifierDescriptor
#define MY_DID ts::DID_ISDB_NETWORK_ID
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBNetworkIdentifierDescriptor::ISDBNetworkIdentifierDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ISDBNetworkIdentifierDescriptor::ISDBNetworkIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBNetworkIdentifierDescriptor()
{
    deserialize(duck, desc);
}


void ts::ISDBNetworkIdentifierDescriptor::clearContent()
{
    country_code.clear();
    media_type = 0;
    network_id = 0;
    private_data.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBNetworkIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(country_code);
    buf.putUInt16(media_type);
    buf.putUInt16(network_id);
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBNetworkIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(country_code);
    media_type = buf.getUInt16();
    network_id = buf.getUInt16();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBNetworkIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(7)) {
        disp << margin << "Country code: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Media type: " << DataName(MY_XML_NAME, u"media_type", buf.getUInt16(), NamesFlags::VALUE);
        disp << UString::Format(u", network id: 0x%X", {buf.getUInt16()}) << std::endl;
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML
//----------------------------------------------------------------------------

const ts::Enumeration ts::ISDBNetworkIdentifierDescriptor::MediaTypes({
    // media_type table in ARIB STD-B21, 9.1.8.3 (as corrected in email by ARIB)
    {u"AB", 0x4142},      // Advanced BS
    {u"AC", 0x4143},      // Advanced wide-band CS
    {u"BS", 0x4253},      // BS/broadband CS
    {u"CS", 0x4353},      // Narrow-band CS / Advanced narrow-band
    {u"TB", 0x5442},      // Terrestrial broadcasting
});


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBNetworkIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"country_code", country_code);
    root->setEnumAttribute(MediaTypes, u"media_type", media_type);
    root->setIntAttribute(u"network_id", network_id, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBNetworkIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(country_code, u"country_code", true, u"", 3, 3) &&
           element->getIntEnumAttribute(media_type, MediaTypes, u"media_type", true) &&
           element->getIntAttribute(network_id, u"network_id", true) &&
           element->getHexaTextChild(private_data, u"private_data");
}

