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

#include "tsPDCDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"PDC_descriptor"
#define MY_CLASS ts::PDCDescriptor
#define MY_DID ts::DID_PDC
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PDCDescriptor::PDCDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    pil_month(0),
    pil_day(0),
    pil_hours(0),
    pil_minutes(0)
{
}

void ts::PDCDescriptor::clearContent()
{
    pil_month = 0;
    pil_day = 0;
    pil_hours = 0;
    pil_minutes = 0;
}

ts::PDCDescriptor::PDCDescriptor(DuckContext& duck, const Descriptor& desc) :
    PDCDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PDCDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt24(0xF00000 |
                      uint32_t(uint32_t(pil_day & 0x1F) << 15) |
                      uint32_t(uint32_t(pil_month & 0x0F) << 11) |
                      uint32_t(uint32_t(pil_hours & 0x1F) << 6) |
                      (pil_minutes & 0x3F));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PDCDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() == 3;

    if (_is_valid) {
        const uint32_t date = GetUInt24(desc.payload());
        pil_day = uint8_t((date >> 15) & 0x1F);
        pil_month = uint8_t((date >> 11) & 0x0F);
        pil_hours = uint8_t((date >> 6) & 0x1F);
        pil_minutes = uint8_t(date & 0x3F);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::PDCDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 3) {
        const uint32_t date = GetUInt24(data);
        data += 3; size -= 3;
        strm << margin
             << UString::Format(u"Programme Identification Label: %02d-%02d %02d:%02d (MM-DD hh:mm)",
                                {(date >> 11) & 0x0F, (date >> 15) & 0x1F, (date >> 6) & 0x1F, date & 0x3F})
             << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PDCDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"programme_identification_label", UString::Format(u"%02d-%02d %02d:%02d", {pil_month, pil_day, pil_hours, pil_minutes}));
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::PDCDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    UString date;
    bool ok =
        element->getAttribute(date, u"programme_identification_label", true) &&
        date.scan(u"%d-%d %d:%d", {&pil_month, &pil_day, &pil_hours, &pil_minutes}) &&
        pil_month > 0 && pil_month < 13 &&
        pil_day > 0 && pil_day < 32 &&
        pil_hours < 24 &&
        pil_minutes < 60;
    if (!ok) {
        element->report().error(u"Incorrect value '%s' for attribute 'programme_identification_label' in <%s>, line %d, use 'MM-DD hh:mm'", {date, element->name(), element->lineNumber()});
    }
    return ok;
}
