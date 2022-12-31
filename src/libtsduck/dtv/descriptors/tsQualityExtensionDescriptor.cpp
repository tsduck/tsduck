//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
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

#include "tsQualityExtensionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"quality_extension_descriptor"
#define MY_CLASS ts::QualityExtensionDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_QUALITY_EXT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::QualityExtensionDescriptor::QualityExtensionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    field_size_bytes(0),
    metric_codes()
{
}

ts::QualityExtensionDescriptor::QualityExtensionDescriptor(DuckContext& duck, const Descriptor& desc) :
    QualityExtensionDescriptor()
{
    deserialize(duck, desc);
}

void ts::QualityExtensionDescriptor::clearContent()
{
    field_size_bytes = 0;
    metric_codes.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::QualityExtensionDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::QualityExtensionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(field_size_bytes);
    buf.putBits(metric_codes.size(), 8);
    for (auto it : metric_codes) {
        buf.putUInt32(it);
    }
}

void ts::QualityExtensionDescriptor::deserializePayload(PSIBuffer& buf)
{
    field_size_bytes = buf.getUInt8();
    uint8_t metric_count = buf.getUInt8();
    for (uint8_t i = 1; i <= metric_count; i++) {
        metric_codes.push_back(buf.getUInt32());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::QualityExtensionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Field size bytes: " << int(buf.getUInt8()) << std::endl;
        const uint8_t metric_count = buf.getUInt8();
        for (uint8_t i = 1; i <= metric_count; i++) {
            disp << margin << "Metric code [" << int(i) << "]: " << DataName(MY_XML_NAME, u"metric_code", buf.getUInt32(), NamesFlags::HEXA_FIRST) << std::endl;
         }
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::QualityExtensionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"field_size_bytes", field_size_bytes);
    for (auto it : metric_codes) {
        root->addElement(u"metric")->setIntAttribute(u"code", it, true);
    }
}

bool ts::QualityExtensionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector _metric_codes;
    bool ok =
        element->getIntAttribute(field_size_bytes, u"field_size_bytes", true, 0, 0, 0xFF) &&
        element->getChildren(_metric_codes, u"metric", 1, 0xFF);

    for (size_t i = 0; ok && i < _metric_codes.size(); i++) {
        uint32_t metric_code = 0;
        ok = _metric_codes[i]->getIntAttribute(metric_code, u"code", true);
        metric_codes.push_back(metric_code);
    }
    return ok;
}
