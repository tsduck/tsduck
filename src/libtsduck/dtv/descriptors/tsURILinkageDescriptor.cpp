//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsURILinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"URI_linkage_descriptor"
#define MY_CLASS ts::URILinkageDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_URI_LINKAGE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::URILinkageDescriptor::URILinkageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    uri_linkage_type(0),
    uri(),
    min_polling_interval(0),
    private_data()
{
}

void ts::URILinkageDescriptor::clearContent()
{
    uri_linkage_type = 0;
    uri.clear();
    min_polling_interval = 0;
    private_data.clear();
}

ts::URILinkageDescriptor::URILinkageDescriptor(DuckContext& duck, const Descriptor& desc) :
    URILinkageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    bbp->appendUInt8(uri_linkage_type);
    bbp->append(duck.encodedWithByteLength(uri));
    if (uri_linkage_type == 0x00 || uri_linkage_type == 0x01) {
        bbp->appendUInt16(min_polling_interval);
    }
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    private_data.clear();
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 3 && data[0] == MY_EDID;

    if (_is_valid) {
        uri_linkage_type = data[1];
        data += 2; size -= 2;
        duck.decodeWithByteLength(uri, data, size);
        if (uri_linkage_type == 0x00 || uri_linkage_type == 0x01) {
            _is_valid = size >= 2;
            if (_is_valid) {
                min_polling_interval = GetUInt16(data);
                data += 2; size -= 2;
            }
        }
        private_data.copy(data, size);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    if (size >= 2) {
        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        const uint8_t type = data[0];
        strm << margin << "URI linkage type: " << NameFromSection(u"URILinkageType", type, names::HEXA_FIRST) << std::endl;
        data++; size--;
        strm << margin << "URI: " << duck.decodedWithByteLength(data, size) << std::endl;

        if ((type == 0x00 || type == 0x01) && size >= 2) {
            const int interval = GetUInt16(data);
            data += 2; size -= 2;
            strm << margin << UString::Format(u"Min polling interval: %d (%d seconds)", {interval, 2 * interval}) << std::endl;
        }
        display.displayPrivateData(u"Private data", data, size, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"uri_linkage_type", uri_linkage_type, true);
    root->setAttribute(u"uri", uri);
    if (uri_linkage_type == 0x00 || uri_linkage_type == 0x01) {
        root->setIntAttribute(u"min_polling_interval", min_polling_interval);
    }
    if (!private_data.empty()) {
        root->addHexaTextChild(u"private_data", private_data);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::URILinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(uri_linkage_type, u"uri_linkage_type", true) &&
           element->getAttribute(uri, u"uri", true) &&
           element->getIntAttribute<uint16_t>(min_polling_interval, u"min_polling_interval", uri_linkage_type <= 1) &&
           element->getHexaTextChild(private_data, u"private_data", false);
}
