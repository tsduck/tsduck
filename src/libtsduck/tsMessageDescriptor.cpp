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
//  Representation of a message_descriptor
//
//----------------------------------------------------------------------------

#include "tsMessageDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::MessageDescriptor, "message_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::MessageDescriptor, ts::EDID(ts::DID_EXTENSION, ts::EDID_MESSAGE));
TS_ID_DESCRIPTOR_DISPLAY(ts::MessageDescriptor::DisplayDescriptor, ts::EDID(ts::DID_EXTENSION, ts::EDID_MESSAGE));


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::MessageDescriptor::MessageDescriptor() :
    AbstractDescriptor(DID_EXTENSION, "message_descriptor"),
    message_id(0),
    language_code(),
    message()
{
    _is_valid = true;
}

ts::MessageDescriptor::MessageDescriptor(uint8_t id, const std::string& lang, const std::string& text) :
    AbstractDescriptor(DID_EXTENSION, "message_descriptor"),
    message_id(id),
    language_code(lang),
    message(text)
{
    _is_valid = true;
}

ts::MessageDescriptor::MessageDescriptor(const Descriptor& bin) :
    AbstractDescriptor(DID_EXTENSION, "message_descriptor"),
    message_id(0),
    language_code(),
    message()
{
    deserialize(bin);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MessageDescriptor::serialize(Descriptor& desc) const
{
    if (!_is_valid || language_code.length() != 3 || 7 + message.length() > MAX_DESCRIPTOR_SIZE) {
        desc.invalidate();
        return;
    }

    ByteBlockPtr bbp(new ByteBlock(2));
    CheckNonNull(bbp.pointer());

    bbp->appendUInt8(EDID_MESSAGE);
    bbp->appendUInt8(message_id);
    bbp->append(language_code);
    bbp->append(message);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d(bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MessageDescriptor::deserialize(const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    if (!(_is_valid = desc.isValid() && desc.tag() == _tag && size >= 5 && data[0] == EDID_MESSAGE)) {
        return;
    }

    message_id = data[1];
    language_code = std::string(reinterpret_cast<const char*>(data + 2), 3);
    message = std::string(reinterpret_cast<const char*>(data + 5), size - 5);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::MessageDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setIntAttribute(root, "message_id", message_id);
    xml.setAttribute(root, "language_code", language_code);
    xml.addText(xml.addElement(root, "text"), message);
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::MessageDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getIntAttribute(message_id, element, "message_id", true) &&
        xml.getAttribute(language_code, element, "language_code", true, "", 3, 3) &&
        xml.getTextChild(message, element, "text");
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MessageDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        const uint8_t message_id = data[0];
        const std::string lang(Printable(data + 1, 3));
        data += 4; size -= 4;
        strm << margin << "Message id: " << int(message_id)
             << ", language: " << lang << std::endl
             << margin << "Message: \"" << Printable(data, size) << "\"" << std::endl;
        data += size; size -= size;
    }

    display.displayExtraData(data, size, indent);
}
