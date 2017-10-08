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
//  Representation of a service_descriptor
//
//----------------------------------------------------------------------------

#include "tsServiceDescriptor.h"
#include "tsUString.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::ServiceDescriptor, "service_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::ServiceDescriptor, ts::EDID(ts::DID_SERVICE));
TS_ID_DESCRIPTOR_DISPLAY(ts::ServiceDescriptor::DisplayDescriptor, ts::EDID(ts::DID_SERVICE));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::ServiceDescriptor::ServiceDescriptor(uint8_t type, const std::string& provider, const std::string& name) :
    AbstractDescriptor(DID_SERVICE, "service_descriptor"),
    service_type(type),
    provider_name(provider),
    service_name(name)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::ServiceDescriptor::ServiceDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(DID_SERVICE, "service_descriptor"),
    service_type(0),
    provider_name(),
    service_name()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    if (provider_name.length() + service_name.length() > 252) {
        desc.invalidate();
        return;
    }

    ByteBlockPtr bbp(new ByteBlock(2));
    CheckNonNull(bbp.pointer());

    bbp->appendUInt8(service_type);
    bbp->appendUInt8(uint8_t(provider_name.length()));
    bbp->append(provider_name);
    bbp->appendUInt8(uint8_t(service_name.length()));
    bbp->append(service_name);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d(bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ServiceDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 3;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        service_type = data[0];
        size_t len = data[1];
        _is_valid = 2 + len < size;
        if (_is_valid) {
            provider_name.assign(reinterpret_cast<const char*>(data + 2), len);
            data += 2 + len;
            size -= 2 + len;
            len = data[0];
            _is_valid = 1 + len <= size;
            if (_is_valid) {
                service_name.assign(reinterpret_cast<const char*>(data + 1), len);
            }
        }
    }

    if (!_is_valid) {
        service_type = 0;
        provider_name.clear();
        service_name.clear();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        // Service type.
        uint8_t stype = *data;
        data += 1; size -= 1;
        strm << margin << Format("Service type: 0x%02X, ", int(stype)) << names::ServiceType(stype) << std::endl;
        
        // Provider and service names (data and size are updated by FromDVBWithByteLength).
        const UString provider(UString::FromDVBWithByteLength(data, size, display.dvbCharset()));
        const UString service(UString::FromDVBWithByteLength(data, size, display.dvbCharset()));
        strm << margin << "Service: \"" << service << "\", Provider: \"" << provider << "\"" << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::ServiceDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setIntAttribute(root, "service_type", service_type, true);
    xml.setAttribute(root, "service_provider_name", provider_name);
    xml.setAttribute(root, "service_name", service_name);
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::ServiceDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getIntAttribute<uint8_t>(service_type, element, "service_type", true) &&
        xml.getAttribute(provider_name, element, "service_provider_name", true) &&
        xml.getAttribute(service_name, element, "service_name", true);
}
