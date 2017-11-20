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

#pragma once


//----------------------------------------------------------------------------
// Get an integer attribute of an XML element.
//----------------------------------------------------------------------------

template <typename INT>
bool ts::XML::getIntAttribute(INT& value,
                             const Element* elem,
                             const UString& name,
                             bool required,
                             INT defValue,
                             INT minValue,
                             INT maxValue)
{
    INT val;
    UString str;
    if (!getAttribute(str, elem, name, required, Decimal(defValue))) {
        return false;
    }
    else if (!str.toInteger(val, ",")) {
        reportError(u"'%s' is not a valid integer value for attribute '%s' in <%s>, line %d",
                    {str, name, ElementName(elem), elem->GetLineNum()});
        return false;
    }
    else if (value < minValue || value > maxValue) {
        reportError(u"'%s' must be in range %'d to %'d for attribute '%s' in <%s>, line %d",
                    {str, minValue, maxValue, name, ElementName(elem), elem->GetLineNum()});
        return false;
    }
    else {
        value = val;
        return true;
    }
}


//----------------------------------------------------------------------------
// Get an optional integer attribute of an XML element.
//----------------------------------------------------------------------------

template <typename INT>
bool ts::XML::getOptionalIntAttribute(Variable<INT>& value,
                                      const Element* elem,
                                      const UString& name,
                                      INT minValue,
                                      INT maxValue)
{
    INT v = 0;
    if (findAttribute(elem, name, true) == 0) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else if (getIntAttribute<INT>(v, elem, name, false, 0, minValue, maxValue)) {
        // Attribute present, correct value.
        value = v;
        return true;
    }
    else {
        // Attribute present, incorrect value.
        value.reset();
        return false;
    }
}


//----------------------------------------------------------------------------
// Get an enumeration attribute of an XML element.
//----------------------------------------------------------------------------

template <typename INT>
bool ts::XML::getIntEnumAttribute(INT& value, const Enumeration& definition, const Element* elem, const UString& name, bool required, INT defValue)
{
    int v = 0;
    const bool ok = getEnumAttribute(v, definition, elem, name, required, int(defValue));
    value = ok ? INT(v) : defValue;
    return ok;
}
