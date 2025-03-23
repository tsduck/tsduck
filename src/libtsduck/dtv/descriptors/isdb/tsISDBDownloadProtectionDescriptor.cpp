//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBDownloadProtectionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISDB_download_protection_descriptor"
#define MY_CLASS    ts::ISDBDownloadProtectionDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_DOWNLOAD_PROT, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBDownloadProtectionDescriptor::ISDBDownloadProtectionDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ISDBDownloadProtectionDescriptor::ISDBDownloadProtectionDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBDownloadProtectionDescriptor()
{
    deserialize(duck, desc);
}

void ts::ISDBDownloadProtectionDescriptor::clearContent()
{
    DL_system_ID = 0;
    DL_program_ID = PID_NULL;
    encrypt_protocol_number = 0;
    encrypt_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBDownloadProtectionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(DL_system_ID);
    buf.putPID(DL_program_ID);
    buf.putUInt8(encrypt_protocol_number);
    buf.putBytes(encrypt_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBDownloadProtectionDescriptor::deserializePayload(PSIBuffer& buf)
{
    DL_system_ID = buf.getUInt8();
    DL_program_ID = buf.getPID();
    encrypt_protocol_number = buf.getUInt8();
    buf.getBytes(encrypt_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBDownloadProtectionDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Download system id: %n", buf.getUInt8()) << std::endl;
        disp << margin << UString::Format(u"Download PID: %n", buf.getPID()) << std::endl;
        disp << margin << UString::Format(u"Encrypt protocol number: %n", buf.getUInt8()) << std::endl;
        disp.displayPrivateData(u"Encrypt info", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBDownloadProtectionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"DL_system_ID", DL_system_ID, true);
    root->setIntAttribute(u"PID", DL_program_ID, true);
    root->setIntAttribute(u"encrypt_protocol_number", encrypt_protocol_number, true);
    root->addHexaTextChild(u"encrypt_info", encrypt_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBDownloadProtectionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(DL_system_ID, u"DL_system_ID", true) &&
           element->getIntAttribute(DL_program_ID, u"PID", true, 0, 0, PID_NULL) &&
           element->getIntAttribute(encrypt_protocol_number, u"encrypt_protocol_number", true) &&
           element->getHexaTextChild(encrypt_info, u"encrypt_info");
}
