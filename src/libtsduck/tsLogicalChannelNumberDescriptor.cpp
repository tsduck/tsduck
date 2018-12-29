//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  Representation of a logical_channel_number_descriptor.
//  Private descriptor, must be preceeded by the EACEM/EICTA PDS.
//
//----------------------------------------------------------------------------

#include "tsLogicalChannelNumberDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"logical_channel_number_descriptor"
#define MY_DID ts::DID_LOGICAL_CHANNEL_NUM
#define MY_PDS ts::PDS_EACEM

TS_XML_DESCRIPTOR_FACTORY(ts::LogicalChannelNumberDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::LogicalChannelNumberDescriptor, ts::EDID::Private(MY_DID, MY_PDS));
TS_ID_DESCRIPTOR_DISPLAY(ts::LogicalChannelNumberDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, MY_PDS));

// Incorrect use of TPS private data, TPS broadcasters should use EACEM/EICTA PDS instead.
TS_ID_DESCRIPTOR_FACTORY(ts::LogicalChannelNumberDescriptor, ts::EDID::Private(MY_DID, ts::PDS_TPS));
TS_ID_DESCRIPTOR_DISPLAY(ts::LogicalChannelNumberDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, ts::PDS_TPS));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::LogicalChannelNumberDescriptor::LogicalChannelNumberDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_PDS),
    entries()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::LogicalChannelNumberDescriptor::LogicalChannelNumberDescriptor (const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor (MY_DID, MY_XML_NAME, MY_PDS),
    entries ()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Constructor using a variable-length argument list.
// Each entry is described by 2 arguments: service_id and lcn.
// All services are marked as visible by default.
// All arguments are int, not uint16_t, since integer literals are int
// by default. The end of the argument list must be marked by -1.
//----------------------------------------------------------------------------

ts::LogicalChannelNumberDescriptor::LogicalChannelNumberDescriptor (int service_id, int lcn, ...) :
    AbstractDescriptor (MY_DID, MY_XML_NAME, MY_PDS),
    entries ()
{
    _is_valid = true;
    entries.push_back (Entry (uint16_t (service_id), true, uint16_t (lcn)));

    va_list ap;
    va_start (ap, lcn);
    int id, n;
    while ((id = va_arg (ap, int)) >= 0 && (n = va_arg (ap, int)) >= 0) {
        entries.push_back (Entry (uint16_t (id), true, uint16_t (n)));
    }
    va_end (ap);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LogicalChannelNumberDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt16 (it->service_id);
        bbp->appendUInt16 ((it->visible ? 0xFC00 : 0x7C00) | (it->lcn & 0x03FF));
    }

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LogicalChannelNumberDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() % 4 == 0;
    entries.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        while (size >= 4) {
            entries.push_back(Entry(GetUInt16(data), (data[2] & 0x80) != 0, GetUInt16(data + 2) & 0x03FF));
            data += 4;
            size -= 4;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LogicalChannelNumberDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    while (size >= 4) {
        const uint16_t service = GetUInt16(data);
        const uint8_t visible = (data[2] >> 7) & 0x01;
        const uint16_t channel = GetUInt16(data + 2) & 0x03FF;
        data += 4; size -= 4;
        strm << margin
             << UString::Format(u"Service Id: %5d (0x%04X), Visible: %1d, Channel number: %3d", {service, service, visible, channel})
             << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LogicalChannelNumberDescriptor::buildXML(xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it->service_id, true);
        e->setIntAttribute(u"logical_channel_number", it->lcn, false);
        e->setBoolAttribute(u"visible_service", it->visible);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::LogicalChannelNumberDescriptor::fromXML(const xml::Element* element)
{
    entries.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Entry entry;
        _is_valid =
            children[i]->getIntAttribute<uint16_t>(entry.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
            children[i]->getIntAttribute<uint16_t>(entry.lcn, u"logical_channel_number", true, 0, 0x0000, 0x03FF) &&
            children[i]->getBoolAttribute(entry.visible, u"visible_service", false, true);
        if (_is_valid) {
            entries.push_back(entry);
        }
    }
}
