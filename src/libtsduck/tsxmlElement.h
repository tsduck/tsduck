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
//!
//!  @file
//!  Element in an XML document.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlNode.h"
#include "tsxmlAttribute.h"
#include "tsByteBlock.h"
#include "tsVariable.h"
#include "tsIPAddress.h"
#include "tsIPv6Address.h"
#include "tsMACAddress.h"

namespace ts {
    namespace xml {

        class Text;

        //!
        //! Structured element in an XML document.
        //! @ingroup xml
        //!
        class TSDUCKDLL Element: public Node
        {
        private:
            // Attributes are stored indexed by case-(in)sensitive name.
            typedef std::map<UString, Attribute> AttributeMap;

        public:
            //!
            //! Constructor.
            //! @param [in,out] report Where to report errors.
            //! @param [in] line Line number in input document.
            //! @param [in] attributeCase State if attribute names are stored with case sensitivity.
            //!
            explicit Element(Report& report = NULLREP, size_t line = 0, CaseSensitivity attributeCase = CASE_INSENSITIVE);

            //!
            //! Constructor.
            //! @param [in,out] parent The parent into which the element is added.
            //! @param [in] name Name of the element.
            //! @param [in] attributeCase State if attribute names are stored wit case sensitivity.
            //!
            Element(Node* parent, const UString& name, CaseSensitivity attributeCase = CASE_INSENSITIVE);

            //!
            //! Get the element name.
            //! This is the same as the node value.
            //! @return A constant reference to the element name.
            //!
            const UString& name() const { return _value; }

            //!
            //! Check if two XML elements have the same name, case-insensitive.
            //! @param [in] other Another XML element.
            //! @return True is this object and @a other have identical names.
            //!
            bool haveSameName(const Element* other) const { return other != nullptr && _value.similar(other->_value); }

            //!
            //! Find the first child element by name, case-insensitive.
            //! @param [in] name Name of the child element to search. If empty, get the first element.
            //! @param [in] silent If true, do not report error.
            //! @return Child element address or zero if not found.
            //!
            const Element* findFirstChild(const UString& name, bool silent = false) const { return (const_cast<Element*>(this))->findFirstChild(name, silent); }

            //!
            //! Find the first child element by name, case-insensitive.
            //! @param [in] name Name of the child element to search. If empty, get the first element.
            //! @param [in] silent If true, do not report error.
            //! @return Child element address or zero if not found.
            //!
            Element* findFirstChild(const UString& name, bool silent = false);

            //!
            //! Find all children elements by name, case-insensitive.
            //! @param [out] children Returned vector of all children.
            //! @param [in] name Name of the child element to search.
            //! @param [in] minCount Minimum required number of elements of that name.
            //! @param [in] maxCount Maximum allowed number of elements of that name.
            //! @return True on success, false on error.
            //!
            bool getChildren(ElementVector& children, const UString& name, size_t minCount = 0, size_t maxCount = UNLIMITED) const;

            //!
            //! Get text in a child of an element.
            //! @param [out] data The content of the text in the child element.
            //! @param [in] name Name of the child element to search.
            //! @param [in] trim If true, remove leading and trailing spaces.
            //! @param [in] required If true, generate an error if the child element is not found.
            //! @param [in] defValue Default value to return if the child element is not present.
            //! @param [in] minSize Minimum allowed size for the value string.
            //! @param [in] maxSize Maximum allowed size for the value string.
            //! @return True on success, false on error.
            //!
            bool getTextChild(UString& data,
                              const UString& name,
                              bool trim = true,
                              bool required = false,
                              const UString& defValue = UString(),
                              size_t minSize = 0,
                              size_t maxSize = UNLIMITED) const;

            //!
            //! Get text inside an element.
            //! In practice, concatenate the content of all Text children inside the element.
            //! @param [out] data The content of the text children.
            //! @param [in] trim If true, remove leading and trailing spaces.
            //! @param [in] minSize Minimum allowed size for the value string.
            //! @param [in] maxSize Maximum allowed size for the value string.
            //! @return True on success, false on error.
            //!
            bool getText(UString& data, bool trim = true, size_t minSize = 0, size_t maxSize = UNLIMITED) const;

            //!
            //! Get text inside an element.
            //! In practice, concatenate the content of all Text children inside the element.
            //! @param [in] trim If true, remove leading and trailing spaces.
            //! @return The content of the text children, empty if non-existent.
            //!
            UString text(bool trim = true) const;

            //!
            //! Get text in a child containing hexadecimal data.
            //! @param [out] data The content of the text in the child element.
            //! @param [in] name Name of the child element to search.
            //! @param [in] required If true, generate an error if the child element is not found.
            //! @param [in] minSize Minimum allowed size for the value string.
            //! @param [in] maxSize Maximum allowed size for the value string.
            //! @return True on success, false on error.
            //!
            bool getHexaTextChild(ByteBlock& data,
                                  const UString& name,
                                  bool required = false,
                                  size_t minSize = 0,
                                  size_t maxSize = UNLIMITED) const;

            //!
            //! Get and interpret the hexadecimal data inside the element.
            //! In practice, concatenate the content of all Text children inside the element
            //! and interpret the result as hexadecimal data.
            //! @param [out] data Buffer receiving the decoded hexadecimal data.
            //! @param [in] minSize Minimum size of the returned data.
            //! @param [in] maxSize Maximum size of the returned data.
            //! @return True on success, false on error.
            //!
            bool getHexaText(ByteBlock& data, size_t minSize = 0, size_t maxSize = UNLIMITED) const;

            //!
            //! Add a new child element at the end of children.
            //! @param [in] childName Name of new child element to create.
            //! @return New child element or null on error.
            //!
            Element* addElement(const UString& childName);

            //!
            //! Add a new text inside this node.
            //! @param [in] text Text string to add.
            //! @return New child element or null on error.
            //!
            Text* addText(const UString& text);

            //!
            //! Add a new text containing hexadecimal data inside this node.
            //! @param [in] data Address of binary data.
            //! @param [in] size Size in bytes of binary data.
            //! @return New child element or null on error.
            //!
            Text* addHexaText(const void* data, size_t size);

            //!
            //! Add a new text containing hexadecimal data inside this node.
            //! @param [in] data Binary data.
            //! @return New child element or null on error.
            //!
            Text* addHexaText(const ByteBlock& data)
            {
                return addHexaText(data.data(), data.size());
            }

            //!
            //! Check if an attribute exists in the element.
            //! @param [in] attributeName Attribute name.
            //! @return True if the attribute exists.
            //!
            bool hasAttribute(const UString& attributeName) const;

            //!
            //! Get an attribute.
            //! @param [in] attributeName Attribute name.
            //! @param [in] silent If true, do not report error.
            //! @return A constant reference to an attribute.
            //! If the argument does not exist, the referenced object is marked invalid.
            //! The reference is valid as long as the Element object is not modified.
            //!
            const Attribute& attribute(const UString& attributeName, bool silent = false) const;

            //!
            //! Set an attribute.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setAttribute(const UString& name, const UString& value);

            //!
            //! Set a bool attribute to a node.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setBoolAttribute(const UString& name, bool value)
            {
                refAttribute(name).setBool(value);
            }

            //!
            //! Set an optional bool attribute to a node.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setOptionalBoolAttribute(const UString& name, const Variable<bool>& value)
            {
                if (value.set()) {
                    refAttribute(name).setBool(value.value());
                }
            }

            //!
            //! Set an attribute with an integer value to a node.
            //! @tparam INT An integer type.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //! @param [in] hexa If true, use an hexadecimal representation (0x...).
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            void setIntAttribute(const UString& name, INT value, bool hexa = false)
            {
                refAttribute(name).setInteger<INT>(value, hexa);
            }

            //!
            //! Set an optional attribute with an integer value to a node.
            //! @tparam INT An integer type.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute optional value. If the variable is not set, no attribute is set.
            //! @param [in] hexa If true, use an hexadecimal representation (0x...).
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            void setOptionalIntAttribute(const UString& name, const Variable<INT>& value, bool hexa = false)
            {
                if (value.set()) {
                    refAttribute(name).setInteger<INT>(value.value(), hexa);
                }
            }

            //!
            //! Set an enumeration attribute of a node.
            //! @param [in] definition The definition of enumeration values.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setEnumAttribute(const Enumeration& definition, const UString& name, int value)
            {
                refAttribute(name).setEnum(definition, value);
            }

            //!
            //! Set an enumeration attribute of a node.
            //! @tparam INT An integer type.
            //! @param [in] definition The definition of enumeration values.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            void setIntEnumAttribute(const Enumeration& definition, const UString& name, INT value)
            {
                refAttribute(name).setIntEnum(definition, value);
            }

            //!
            //! Set a date/time attribute of an XML element.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setDateTimeAttribute(const UString& name, const Time& value)
            {
                refAttribute(name).setDateTime(value);
            }

            //!
            //! Set a time attribute of an XML element in "hh:mm:ss" format.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setTimeAttribute(const UString& name, Second value)
            {
                refAttribute(name).setTime(value);
            }

            //!
            //! Set an IPv4 address attribute of an XML element in "x.x.x.x" format.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setIPAttribute(const UString& name, const IPAddress& value)
            {
                setAttribute(name, value.toString());
            }

            //!
            //! Set an IPv6 address attribute of an XML element.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setIPv6Attribute(const UString& name, const IPv6Address& value)
            {
                setAttribute(name, value.toString());
            }

            //!
            //! Set a MAC address attribute of an XML element in "x:x:x:x:x:x" format.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setMACAttribute(const UString& name, const MACAddress& value)
            {
                setAttribute(name, value.toString());
            }

            //!
            //! Get a string attribute of an XML element.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @param [in] minSize Minimum allowed size for the value string.
            //! @param [in] maxSize Maximum allowed size for the value string.
            //! @return True on success, false on error.
            //!
            bool getAttribute(UString& value,
                              const UString& name,
                              bool required = false,
                              const UString& defValue = UString(),
                              size_t minSize = 0,
                              size_t maxSize = UNLIMITED) const;

            //!
            //! Get a boolean attribute of an XML element.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            bool getBoolAttribute(bool& value, const UString& name, bool required = false, bool defValue = false) const;

            //!
            //! Get an optional boolean attribute of an XML element.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @return True on success, false on error.
            //!
            bool getOptionalBoolAttribute(Variable<bool>& value, const UString& name) const;

            //!
            //! Get an integer attribute of an XML element.
            //! @tparam INT An integer type.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @param [in] minValue Minimum allowed value for the attribute.
            //! @param [in] maxValue Maximum allowed value for the attribute.
            //! @return True on success, false on error.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            bool getIntAttribute(INT& value,
                                 const UString& name,
                                 bool required = false,
                                 INT defValue = 0,
                                 INT minValue = std::numeric_limits<INT>::min(),
                                 INT maxValue = std::numeric_limits<INT>::max()) const;

            //!
            //! Get an optional integer attribute of an XML element.
            //! @tparam INT An integer type.
            //! @param [out] value Returned value of the attribute. If the attribute is ot present, the variable is reset.
            //! @param [in] name Name of the attribute.
            //! @param [in] minValue Minimum allowed value for the attribute.
            //! @param [in] maxValue Maximum allowed value for the attribute.
            //! @return True on success, false on error.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            bool getOptionalIntAttribute(Variable<INT>& value,
                                         const UString& name,
                                         INT minValue = std::numeric_limits<INT>::min(),
                                         INT maxValue = std::numeric_limits<INT>::max()) const;

            //!
            //! Get an enumeration attribute of an XML element.
            //! Integer literals and integer values are accepted in the attribute.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] definition The definition of enumeration values.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            bool getEnumAttribute(int& value, const Enumeration& definition, const UString& name, bool required = false, int defValue = 0) const;

            //!
            //! Get an enumeration attribute of an XML element.
            //! Integer literals and integer values are accepted in the attribute.
            //! @tparam INT An integer type.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] definition The definition of enumeration values.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            bool getIntEnumAttribute(INT& value, const Enumeration& definition, const UString& name, bool required = false, INT defValue = INT(0)) const;

            //!
            //! Get a date/time attribute of an XML element.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            bool getDateTimeAttribute(Time& value, const UString& name, bool required = false, const Time& defValue = Time()) const;

            //!
            //! Get a time attribute of an XML element in "hh:mm:ss" format.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            bool getTimeAttribute(Second& value, const UString& name, bool required = false, Second defValue = 0) const;

            //!
            //! Get an IPv4 address attribute of an XML element in "x.x.x.x" format or host name.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            bool getIPAttribute(IPAddress& value, const UString& name, bool required = false, const IPAddress& defValue = IPAddress()) const;

            //!
            //! Get an IPv6 address attribute of an XML element.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            bool getIPv6Attribute(IPv6Address& value, const UString& name, bool required = false, const IPv6Address& defValue = IPv6Address()) const;

            //!
            //! Get a MAC address attribute of an XML element in "x:x:x:x:x:x" format.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            bool getMACAttribute(MACAddress& value, const UString& name, bool required = false, const MACAddress& defValue = MACAddress()) const;

            //!
            //! Get the list of all attribute names.
            //! @param [out] names Returned list of all attribute names.
            //!
            void getAttributesNames(UStringList& names) const;

            //!
            //! Get the list of all attribute names, sorted by modification order.
            //! The method is slower than getAttributesNames().
            //! @param [out] names Returned list of all attribute names.
            //!
            void getAttributesNamesInModificationOrder(UStringList& names) const;

            // Inherited from xml::Node.
            virtual void clear() override;
            virtual UString typeName() const override { return u"Element"; }
            virtual void print(TextFormatter& output, bool keepNodeOpen = false) const override;
            virtual void printClose(TextFormatter& output, size_t levels = std::numeric_limits<size_t>::max()) const override;

        protected:
            // Inherited from xml::Node.
            virtual bool parseNode(TextParser& parser, const Node* parent) override;

        private:
            CaseSensitivity _attributeCase;  //!< For attribute names.
            AttributeMap    _attributes;     //!< Map of attributes.

            // Compute the key in the attribute map.
            UString attributeKey(const UString& attributeName) const;

            // Find a key in the attribute map.
            AttributeMap::const_iterator findAttribute(const UString& attributeName) const;

            // Get a modifiable reference to an attribute, create if does not exist.
            Attribute& refAttribute(const UString& attributeName);

            // Inaccessible operations.
            Element(const Element&) = delete;
            Element& operator=(const Element&) = delete;
        };
    }
}

#include "tsxmlElementTemplate.h"
