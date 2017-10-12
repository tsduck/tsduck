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

#include "tsAbstractSignalization.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::AbstractSignalization::AbstractSignalization(const char* xml_name) :
    _xml_name(xml_name),
    _is_valid(false)
{
}

ts::AbstractSignalization::AbstractSignalization(const AbstractSignalization& other) :
    _xml_name(other._xml_name),  // Normally a pointer to constant static string.
    _is_valid(other._is_valid)
{
}


ts::AbstractSignalization& ts::AbstractSignalization::operator=(const AbstractSignalization& other)
{
    if (this != &other) {
        assert((_xml_name == 0 && other._xml_name == 0) ||
               (_xml_name != 0 && other._xml_name != 0 && ::strcmp(_xml_name, other._xml_name) == 0));
        _is_valid = other._is_valid;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Get the XMl node name representing this table.
//----------------------------------------------------------------------------

std::string ts::AbstractSignalization::xmlName() const
{
    return _xml_name == 0 ? std::string() : std::string(_xml_name);
}


//----------------------------------------------------------------------------
// Check that an XML element has the right name for this table.
//----------------------------------------------------------------------------

bool ts::AbstractSignalization::checkXMLName(XML& xml, const XML::Element* element) const
{
    if (element == 0) {
        return false;
    }
    else if (!UString::FromUTF8(_xml_name).similar(element->Name())) {
        xml.reportError(Format("Incorrect <%s>, expected <%s>", XML::ElementName(element), _xml_name));
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// This static method serializes a DVB string with a required fixed size.
//----------------------------------------------------------------------------

bool ts::AbstractSignalization::SerializeFixedLength(ByteBlock& bb, const UString& str, const size_t size, const DVBCharset* charset)
{
    const ByteBlock dvb(str.toDVB(0, UString::NPOS, charset));
    if (dvb.size() == size) {
        bb.append(dvb);
        return true;
    }
    else {
        return false;
    }
}
