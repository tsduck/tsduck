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

#pragma once


//----------------------------------------------------------------------------
// Get an integer attribute of an XML element.
//----------------------------------------------------------------------------

template <typename INT, typename INT1, typename INT2, typename INT3, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::xml::Element::getIntAttribute(INT& value, const UString& name, bool required, INT1 defValue, INT2 minValue, INT3 maxValue) const
{
    const Attribute& attr(attribute(name, !required));
    if (!attr.isValid()) {
        // Attribute not present.
        value = INT(defValue);
        return !required;
    }

    // Attribute found, get its value.
    UString str(attr.value());
    INT val = INT(0);
    if (!str.toInteger(val, u",")) {
        report().error(u"'%s' is not a valid integer value for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
        return false;
    }
    else if (val < INT(minValue) || val > INT(maxValue)) {
        report().error(u"'%s' must be in range %'d to %'d for attribute '%s' in <%s>, line %d", {str, minValue, maxValue, name, this->name(), lineNumber()});
        return false;
    }
    else {
        value = val;
        return true;
    }
}

#if !defined(TSXML_GCC_TEMPLATE_SUBSTITUTION_BUG)
template <typename ENUM, typename INT1, typename INT2, typename std::enable_if<std::is_enum<ENUM>::value>::type*, typename INT>
bool ts::xml::Element::getIntAttribute(ENUM& value, const UString& name, bool required, ENUM defValue, INT1 minValue, INT2 maxValue) const
{
    INT val = INT(0);
    const bool ok = getIntAttribute<INT>(val, name, required, defValue, minValue, maxValue);
    if (ok) {
        value = ENUM(val);
    }
    return ok;
}
#endif


//----------------------------------------------------------------------------
// Get an optional integer attribute of an XML element.
//----------------------------------------------------------------------------

template <typename INT, typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::xml::Element::getOptionalIntAttribute(Variable<INT>& value, const UString& name, INT1 minValue, INT2 maxValue) const
{
    INT v = INT(0);
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.clear();
        return true;
    }
    else if (getIntAttribute<INT>(v, name, false, INT(0), minValue, maxValue)) {
        // Attribute present, correct value.
        value = v;
        return true;
    }
    else {
        // Attribute present, incorrect value.
        value.clear();
        return false;
    }
}


//----------------------------------------------------------------------------
// Get an enumeration attribute of an XML element.
//----------------------------------------------------------------------------

template <typename INT, typename INT1, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type*>
bool ts::xml::Element::getIntEnumAttribute(INT& value, const Enumeration& definition, const UString& name, bool required, INT1 defValue) const
{
    int v = 0;
    const bool ok = getEnumAttribute(v, definition, name, required, int(defValue));
    value = ok ? INT(v) : INT(defValue);
    return ok;
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type*>
bool ts::xml::Element::getOptionalIntEnumAttribute(Variable<INT>& value, const Enumeration& definition, const UString& name) const
{
    INT v = INT(0);
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.clear();
        return true;
    }
    else if (getIntEnumAttribute<INT>(v, definition, name, false)) {
        // Attribute present, correct value.
        value = v;
        return true;
    }
    else {
        // Attribute present, incorrect value.
        value.clear();
        return false;
    }
}


//----------------------------------------------------------------------------
// Get a floating-point attribute of an XML element.
//----------------------------------------------------------------------------

template <typename FLT, typename FLT1, typename FLT2, typename FLT3, typename std::enable_if<std::is_floating_point<FLT>::value>::type*>
bool ts::xml::Element::getFloatAttribute(FLT& value, const UString& name, bool required, FLT1 defValue, FLT2 minValue, FLT3 maxValue) const
{
    const Attribute& attr(attribute(name, !required));
    if (!attr.isValid()) {
        // Attribute not present.
        value = FLT(defValue);
        return !required;
    }

    // Attribute found, get its value.
    UString str(attr.value());
    FLT val = FLT(0.0);
    if (!str.toFloat(val)) {
        report().error(u"'%s' is not a valid floating-point value for attribute '%s' in <%s>, line %d", {str, name, this->name(), lineNumber()});
        return false;
    }
    else if (val < FLT(minValue) || val > FLT(maxValue)) {
        report().error(u"'%s' must be in range %f to %f for attribute '%s' in <%s>, line %d", {str, double(minValue), double(maxValue), name, this->name(), lineNumber()});
        return false;
    }
    else {
        value = val;
        return true;
    }
}


//----------------------------------------------------------------------------
// Get an optional floating-point attribute of an XML element.
//----------------------------------------------------------------------------

template <typename FLT, typename FLT1, typename FLT2, typename std::enable_if<std::is_floating_point<FLT>::value>::type*>
bool ts::xml::Element::getOptionalFloatAttribute(Variable<FLT>& value, const UString& name, FLT1 minValue, FLT2 maxValue) const
{
    FLT v = FLT(0.0);
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.clear();
        return true;
    }
    else if (getFloatAttribute<FLT>(v, name, false, FLT(0.0), minValue, maxValue)) {
        // Attribute present, correct value.
        value = v;
        return true;
    }
    else {
        // Attribute present, incorrect value.
        value.clear();
        return false;
    }
}
