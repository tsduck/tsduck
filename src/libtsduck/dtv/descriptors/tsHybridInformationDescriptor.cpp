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
//
// WARNING: This descriptor is currently not active (not registered).
//
// Its descriptor tag is 0x68, which conflicts with DVB allocation.
// We have not yet implemented a way to implement concurrent DVB and ISDB
// descriptor in the non-private range (below 0x80). The code is just here
// for future reference. To be debugged, just in case.
//
// XML template to add, when implemented:
//
//    <hybrid_information_descriptor
//        format="uint4, required"
//        component_tag="uint8, optional"
//        module_id="uint16, optional"
//        URL="string, optional"/>
//
//----------------------------------------------------------------------------

#include "tsHybridInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"hybrid_information_descriptor"
// #define MY_CLASS ts::HybridInformationDescriptor
#define MY_DID ts::DID_ISDB_HYBRID_INFO
// #define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HybridInformationDescriptor::HybridInformationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    has_location(false),
    location_type(false),
    format(0),
    component_tag(0),
    module_id(0),
    URL()
{
}

ts::HybridInformationDescriptor::HybridInformationDescriptor(DuckContext& duck, const Descriptor& desc) :
    HybridInformationDescriptor()
{
    deserialize(duck, desc);
}

void ts::HybridInformationDescriptor::clearContent()
{
    has_location = false;
    location_type = false;
    format = 0;
    component_tag = 0;
    module_id = 0;
    URL.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HybridInformationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((has_location ? 0x80 : 0x00) |
                     (location_type ? 0x40 : 0x00) |
                     uint8_t((format & 0x0F) << 2) |
                     0x03);
    if (has_location) {
        if (location_type) {
            // We assume here that the URL is encoded in ARIB STD-B24. Could be in ASCII ?
            bbp->append(duck.encodedWithByteLength(URL));
        }
        else {
            bbp->appendUInt8(component_tag);
            bbp->appendUInt16(module_id);
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::HybridInformationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;
    URL.clear();

    if (_is_valid) {
        has_location = (data[0] & 0x80) != 0;
        location_type = (data[0] & 0x40) != 0;
        format = (data[0] >> 2) & 0x0F;
        data++; size--;

        if (has_location) {
            if (location_type) {
                _is_valid = size > 0;
                if (_is_valid) {
                    // We assume here that the URL is encoded in ARIB STD-B24. Could be in ASCII ?
                    duck.decodeWithByteLength(URL, data, size);
                    _is_valid = size == 0;
                }
            }
            else {
                _is_valid = size == 3;
                if (_is_valid) {
                    component_tag = data[0];
                    module_id = GetUInt16(data + 1);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HybridInformationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size > 0) {
        const bool has = (data[0] & 0x80) != 0;
        const bool type = (data[0] & 0x40) != 0;
        const uint8_t format = (data[0] >> 2) & 0x0F;
        data++; size--;

        strm << margin << "Has location: " << UString::YesNo(has) << std::endl
             << margin << "Location type: " << (type ? "connected" : "broadcast") << std::endl
             << margin << "Format: " << NameFromSection(u"ISDBHybridInformationFormat", format, names::DECIMAL_FIRST) << std::endl;

        if (has) {
            if (type) {
                strm << margin << "URL: \"" << duck.decodedWithByteLength(data, size) << "\"" << std::endl;
            }
            else if (size >= 3) {
                strm << margin << UString::Format(u"Component tag: 0x0%X (%d)", {data[0], data[0]}) << std::endl
                     << margin << UString::Format(u"Module id: 0x0%X (%d)", {GetUInt16(data + 1), GetUInt16(data + 1)}) << std::endl;
                data += 3; size -= 3;
            }
        }

        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HybridInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"format", format);
    if (has_location) {
        if (location_type) {
            root->setAttribute(u"URL", URL);
        }
        else {
            root->setIntAttribute(u"component_tag", component_tag, true);
            root->setIntAttribute(u"module_id", module_id, true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::HybridInformationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    const bool has_connect = element->hasAttribute(u"URL");
    const int bcast_count = element->hasAttribute(u"component_tag") + element->hasAttribute(u"module_id");

    has_location = has_connect || bcast_count > 0;
    location_type = has_connect;

    if (bcast_count == 1) {
        element->report().error(u"attributes 'component_tag' and 'module_id' must be both present or both absent in <%s>, line %d", {element->name(), element->lineNumber()});
        return false;
    }
    else if (has_connect && bcast_count > 0) {
        element->report().error(u"attribute 'URL' and attributes 'component_tag', 'module_id' are mutually exclusive in <%s>, line %d", {element->name(), element->lineNumber()});
        return false;
    }
    else if (!element->getIntAttribute<uint8_t>(format, u"format", true, 0, 0, 15)) {
        return false;
    }
    else if (!has_location) {
        return true;
    }
    else if (location_type) {
        return element->getAttribute(URL, u"URL");
    }
    else {
        return element->getIntAttribute<uint8_t>(component_tag, u"component_tag") && element->getIntAttribute<uint16_t>(module_id, u"module_id");
    }
}
