//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsAbstractSignalization.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and assignments.
//----------------------------------------------------------------------------

ts::AbstractSignalization::AbstractSignalization(const UChar* xml_name) :
    _xml_name(xml_name),
    _is_valid(false)
{
}

ts::AbstractSignalization& ts::AbstractSignalization::operator=(const AbstractSignalization& other)
{
    if (this != &other) {
        // Don't copy the pointer to XML name, this is a const value.
        // In debug mode, check that we have the same XML name.
        assert((_xml_name == nullptr && other._xml_name == nullptr) ||
               (_xml_name != nullptr && other._xml_name != nullptr && UString(_xml_name) == UString(other._xml_name)));
        _is_valid = other._is_valid;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Get the XMl node name representing this table.
//----------------------------------------------------------------------------

ts::UString ts::AbstractSignalization::xmlName() const
{
    return UString(_xml_name);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::xml::Element* ts::AbstractSignalization::toXML(xml::Element* parent) const
{
    xml::Element* root = _is_valid && parent != nullptr ? parent->addElement(_xml_name) : nullptr;
    if (root != nullptr) {
        buildXML(root);
    }
    return root;
}


//----------------------------------------------------------------------------
// Check that an XML element has the right name for this table.
//----------------------------------------------------------------------------

bool ts::AbstractSignalization::checkXMLName(const xml::Element* element) const
{
    if (element == nullptr) {
        return false;
    }
    else if (element->name().similar(_xml_name)) {
        return true;
    }
    else {
        element->report().error(u"Incorrect <%s>, expected <%s>", {element->name(), _xml_name});
        return false;
    }
}


//----------------------------------------------------------------------------
// This static method serializes a DVB string with a required fixed size.
//----------------------------------------------------------------------------

bool ts::AbstractSignalization::SerializeFixedLength(ByteBlock& bb, const UString& str, const size_t size, const DVBCharset* charset)
{
    const ByteBlock dvb(str.toDVB(0, NPOS, charset));
    if (dvb.size() == size) {
        bb.append(dvb);
        return true;
    }
    else {
        return false;
    }
}
