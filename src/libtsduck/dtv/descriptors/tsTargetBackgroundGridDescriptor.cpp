//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsTargetBackgroundGridDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_background_grid_descriptor"
#define MY_CLASS ts::TargetBackgroundGridDescriptor
#define MY_DID ts::DID_TGT_BG_GRID
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetBackgroundGridDescriptor::TargetBackgroundGridDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    horizontal_size(0),
    vertical_size(0),
    aspect_ratio_information(0)
{
}

void ts::TargetBackgroundGridDescriptor::clearContent()
{
    horizontal_size = 0;
    vertical_size = 0;
    aspect_ratio_information = 0;
}

ts::TargetBackgroundGridDescriptor::TargetBackgroundGridDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetBackgroundGridDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetBackgroundGridDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(horizontal_size, 14);
    buf.putBits(vertical_size, 14);
    buf.putBits(aspect_ratio_information, 4);
}

void ts::TargetBackgroundGridDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(horizontal_size, 14);
    buf.getBits(vertical_size, 14);
    buf.getBits(aspect_ratio_information, 4);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetBackgroundGridDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Size: %d", {buf.getBits<uint16_t>(14)});
        disp << UString::Format(u"x%d", {buf.getBits<uint16_t>(14)});
        disp << ", aspect ratio: " << NameFromDTV(u"mpeg2.aspect_ratio", buf.getBits<uint8_t>(4), NamesFlags::DECIMAL_FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetBackgroundGridDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"horizontal_size", horizontal_size);
    root->setIntAttribute(u"vertical_size", vertical_size);
    root->setIntAttribute(u"aspect_ratio_information", aspect_ratio_information);
}

bool ts::TargetBackgroundGridDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(horizontal_size, u"horizontal_size", true, 0, 0, 0x3FFF) &&
           element->getIntAttribute(vertical_size, u"vertical_size", true, 0, 0, 0x3FFF) &&
           element->getIntAttribute(aspect_ratio_information, u"aspect_ratio_information", true, 0, 0, 0x0F);
}
