//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-, Paul Higgs
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

#include "tsLCEVCLinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"LCEVC_linkage_descriptor"
#define MY_CLASS ts::LCEVCLinkageDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_LCEVC_LINKAGE
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::LCEVCLinkageDescriptor::LCEVCLinkageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    lcevc_stream_tags()
{
}

void ts::LCEVCLinkageDescriptor::clearContent()
{
    lcevc_stream_tags.clear();

}

ts::LCEVCLinkageDescriptor::LCEVCLinkageDescriptor(DuckContext& duck, const Descriptor& desc) :
    LCEVCLinkageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LCEVCLinkageDescriptor::serializePayload(PSIBuffer& buf) const
{
    uint8_t num_lcevc_stream_tags = lcevc_stream_tags.size() & 0xFF;
    buf.putUInt8(num_lcevc_stream_tags);
    for (auto it : lcevc_stream_tags)
        buf.putUInt8(it);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LCEVCLinkageDescriptor::deserializePayload(PSIBuffer& buf)
{
    uint8_t num_lcevc_stream_tags = buf.getUInt8();
    for (uint8_t i = 0; i < num_lcevc_stream_tags; i++)
        lcevc_stream_tags.push_back(buf.getUInt8());
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LCEVCLinkageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    
    if (buf.canReadBytes(1)) {
        uint8_t num_lcevc_stream_tags = buf.getUInt8();
        if (num_lcevc_stream_tags > 0) {
            disp << margin << "LCEVC stream tag: ";
            for (uint8_t i = 0; i < num_lcevc_stream_tags; i++) {
                disp << buf.getUInt8() << " ";
                if ((i + 1) % 6 == 0) {
                    disp << std::endl;
                    if (i != (num_lcevc_stream_tags - 1))
                        disp << margin << "                  ";
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LCEVCLinkageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{ 
    root->setIntAttribute(u"num_lcevc_stream_tags", lcevc_stream_tags.size());
        for (auto it : lcevc_stream_tags) {
        uint8_t lcevc_stream_tag = it;
        root->addHexaTextChild(u"lcevc_stream_tag", &lcevc_stream_tag, sizeof(lcevc_stream_tag));
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LCEVCLinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    uint8_t num_lcevc_stream_tags;
    bool ok = element->getIntAttribute(num_lcevc_stream_tags, u"num_lcevc_stream_tags", true, 0, 0x00, 0xFF);
    if (ok) {
        xml::ElementVector children;
        ok &= element->getChildren(children, u"lcevc_stream_tag");
        for (size_t i = 0; ok && i < children.size(); ++i) {
            UString hexVal(u"");
            ok &= children[i]->getText(hexVal);
            uint16_t val = 0;
            if (!hexVal.toInteger(val, u",")) {
                element->report().error(u"'%s' is not a valid integer value for attribute '%s' in <%s>, line %d", { hexVal, u"lcevc_stream_tag", element->lineNumber(), element->name() });
                ok = false;
            }
            else if (val < 0 || val > 0xFF) {
                element->report().error(u"'%s' is not in the range %d to %d attribute '%s' in <%s>, line %d", { hexVal, 0, 0xFF, u"lcevc_stream_tag", element->lineNumber(), element->name() });
                ok = false;
            }
        }
    }
    return ok;
}
