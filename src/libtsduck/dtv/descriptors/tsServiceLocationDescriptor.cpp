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

#include "tsServiceLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"service_location_descriptor"
#define MY_CLASS ts::ServiceLocationDescriptor
#define MY_DID ts::DID_ATSC_SERVICE_LOC
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceLocationDescriptor::ServiceLocationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    PCR_PID(PID_NULL),
    entries()
{
}

void ts::ServiceLocationDescriptor::clearContent()
{
    PCR_PID = PID_NULL;
    entries.clear();
}

ts::ServiceLocationDescriptor::ServiceLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceLocationDescriptor()
{
    deserialize(duck, desc);
}

ts::ServiceLocationDescriptor::Entry::Entry(uint8_t type, ts::PID pid, const ts::UString& lang) :
    stream_type(type),
    elementary_PID(pid),
    ISO_639_language_code(lang)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceLocationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(0xE000 | PCR_PID);
    bbp->appendUInt8(uint8_t(entries.size()));
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt8(it->stream_type);
        bbp->appendUInt16(0xE000 | it->elementary_PID);
        if (!SerializeLanguageCode(*bbp, it->ISO_639_language_code, true)) {
            desc.invalidate();
            return;
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ServiceLocationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    entries.clear();
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 3 && (size - 3) % 6 == 0;

    if (_is_valid) {
        // Fixed part.
        PCR_PID = GetUInt16(data) & 0x1FFF;
        size_t count = data[2];
        data += 3; size -= 3;

        // Loop on all component entries.
        while (count-- > 0 && size >= 6) {
            entries.push_back(Entry(data[0], GetUInt16(data + 1) & 0x1FFF, DeserializeLanguageCode(data + 3)));
            data += 6; size -= 6;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceLocationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    if (size >= 3) {
        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        PID pid = GetUInt16(data) & 0x1FFF;
        size_t count = data[2];
        data += 3; size -= 3;

        strm << margin << "PCR PID: ";
        if (pid == PID_NULL) {
            strm << "none";
        }
        else {
            strm << UString::Format(u"0x%X (%d)", {pid, pid});
        }
        strm << ", number of elements: " << count << std::endl;

        // Loop on all component entries.
        while (count-- > 0 && size >= 6) {
            const uint8_t stype = data[0];
            pid = GetUInt16(data + 1) & 0x1FFF;
            const UString lang(DeserializeLanguageCode(data + 3));
            data += 6; size -= 6;

            strm << margin << UString::Format(u"- PID: 0x%X (%d), language: \"%s\", type: %s", {pid, pid, lang, names::ServiceType(stype, names::FIRST)}) << std::endl;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ServiceLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    if (PCR_PID != PID_NULL) {
        root->setIntAttribute(u"PCR_PID", PCR_PID, true);
    }
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"component");
        e->setIntAttribute(u"stream_type", it->stream_type, true);
        e->setIntAttribute(u"elementary_PID", it->elementary_PID, true);
        if (!it->ISO_639_language_code.empty()) {
            e->setAttribute(u"ISO_639_language_code", it->ISO_639_language_code);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ServiceLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute<uint16_t>(PCR_PID, u"PCR_PID", false, PID_NULL, 0, 0x1FFF) &&
        element->getChildren(children, u"component", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute<uint8_t>(entry.stream_type, u"stream_type", true) &&
             children[i]->getIntAttribute<uint16_t>(entry.elementary_PID, u"elementary_PID", true, 0, 0, 0x1FFF) &&
             children[i]->getAttribute(entry.ISO_639_language_code, u"ISO_639_language_code", false, UString(), 0, 3);
        entries.push_back(entry);
    }
    return ok;
}
