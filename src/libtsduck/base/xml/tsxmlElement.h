//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsIPv4Address.h"
#include "tsIPv6Address.h"
#include "tsMACAddress.h"

// There is a bug in GCC version 6 and 7 which prevents some template methods in this file from
// compiling correctly. This is specific to GCC 6 and 7. There is no issue with GCC 4.9, 8.x, 9.x
// as well as MSVC and clang. The error typically appears on Debian/Raspbian 10, Ubuntu 18.04,
// as well as obsolete versions of other distros.

#if defined(TS_GCC_ONLY) && !defined(TS_IGNORE_GCC6_BUG) && (__GNUC__ == 6 || __GNUC__ == 7) && !defined(TSXML_GCC_TEMPLATE_SUBSTITUTION_BUG)
    #define TSXML_GCC_TEMPLATE_SUBSTITUTION_BUG 1
#endif

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
            //! @param [in] last If true, the child is added at the end of the list of children.
            //! If false, it is added at the beginning.
            //!
            Element(Node* parent, const UString& name, CaseSensitivity attributeCase = CASE_INSENSITIVE, bool last = true);

            //!
            //! Copy constructor.
            //! @param [in] other Other instance to copy.
            //!
            Element(const Element& other);

            //!
            //! Get the element name.
            //! This is the same as the node value.
            //! @return A constant reference to the element name.
            //!
            const UString& name() const { return value(); }

            //!
            //! Check if two XML elements have the same name, case-insensitive.
            //! @param [in] other Another XML element.
            //! @return True is this object and @a other have identical names.
            //!
            bool haveSameName(const Element* other) const { return other != nullptr && value().similar(other->value()); }

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
                              bool trim = false,
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
            bool getText(UString& data, bool trim = false, size_t minSize = 0, size_t maxSize = UNLIMITED) const;

            //!
            //! Get text inside an element.
            //! In practice, concatenate the content of all Text children inside the element.
            //! @param [in] trim If true, remove leading and trailing spaces.
            //! @return The content of the text children, empty if non-existent.
            //!
            UString text(bool trim = false) const;

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
            //! @param [in] onlyNotEmpty When true, do not add the text if the string is empty.
            //! @return New child element or null on error.
            //!
            Text* addText(const UString& text, bool onlyNotEmpty = false);

            //!
            //! Add a new text containing hexadecimal data inside this node.
            //! @param [in] data Address of binary data.
            //! @param [in] size Size in bytes of binary data.
            //! @param [in] onlyNotEmpty When true, do not add the child element if the data is empty.
            //! @return New child element or null on error or empty data.
            //!
            Text* addHexaText(const void* data, size_t size, bool onlyNotEmpty = false);

            //!
            //! Add a new text containing hexadecimal data inside this node.
            //! @param [in] data Binary data.
            //! @param [in] onlyNotEmpty When true, do not add the child element if the data is empty.
            //! @return New child element or null on error or empty data.
            //!
            Text* addHexaText(const ByteBlock& data, bool onlyNotEmpty = false)
            {
                return addHexaText(data.data(), data.size(), onlyNotEmpty);
            }

            //!
            //! Add a new child element containing an hexadecimal data text.
            //! @param [in] name Name of the child element to search.
            //! @param [in] data Address of binary data.
            //! @param [in] size Size in bytes of binary data.
            //! @param [in] onlyNotEmpty When true, do not add the child element if the data is empty.
            //! @return New child element or null on error or empty data.
            //!
            Text* addHexaTextChild(const UString& name, const void* data, size_t size, bool onlyNotEmpty = false);

            //!
            //! Add a new child element containing an hexadecimal data text.
            //! @param [in] name Name of the child element to search.
            //! @param [in] data Binary data.
            //! @param [in] onlyNotEmpty When true, do not add the child element if the data is empty.
            //! @return New child element or null on error or empty data.
            //!
            Text* addHexaTextChild(const UString& name, const ByteBlock& data, bool onlyNotEmpty = false);

            //!
            //! Check if an attribute exists in the element.
            //! @param [in] attributeName Attribute name.
            //! @return True if the attribute exists.
            //!
            bool hasAttribute(const UString& attributeName) const;

            //!
            //! Check if an attribute exists in the element and has the specified value.
            //! @param [in] attributeName Attribute name.
            //! @param [in] value Expected value.
            //! @param [in] similar If true, the comparison between the actual and expected
            //! values is performed case-insensitive and ignoring blanks. If false, a strict
            //! comparison is performed.
            //! @return True if the attribute exists and has the expected value.
            //!
            bool hasAttribute(const UString& attributeName, const UString& value, bool similar = false) const;

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
            //! Delete an attribute.
            //! @param [in] name Attribute name to delete.
            //!
            void deleteAttribute(const UString& name);

            //!
            //! Set an attribute.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //! @param [in] onlyIfNotEmpty When true, do not insert the attribute if @a value is empty.
            //!
            void setAttribute(const UString& name, const UString& value, bool onlyIfNotEmpty = false);

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
            void setOptionalBoolAttribute(const UString& name, const std::optional<bool>& value)
            {
                if (value.has_value()) {
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
            void setOptionalIntAttribute(const UString& name, const std::optional<INT>& value, bool hexa = false)
            {
                if (value.has_value()) {
                    refAttribute(name).setInteger<INT>(value.value(), hexa);
                }
            }

            //!
            //! Set an attribute with a floating-point value to a node.
            //! @tparam FLT A floating-point type.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //! @param [in] width Width of the formatted number, not including the optional prefix and separator.
            //! @param [in] precision Precision to use after the decimal point.  Default is 6 digits.
            //! @param [in] force_sign If true, force a '+' sign for positive values.
            //!
            template <typename FLT, typename std::enable_if<std::is_floating_point<FLT>::value>::type* = nullptr>
            void setFloatAttribute(const UString& name, FLT value, size_t width = 0, size_t precision = 6, bool force_sign = false)
            {
                refAttribute(name).setFloat<FLT>(value, width, precision, force_sign);
            }

            //!
            //! Set an optional attribute with a floating-point value to a node.
            //! @tparam FLT A floating-point type.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute optional value. If the variable is not set, no attribute is set.
            //! @param [in] width Width of the formatted number, not including the optional prefix and separator.
            //! @param [in] precision Precision to use after the decimal point.  Default is 6 digits.
            //! @param [in] force_sign If true, force a '+' sign for positive values.
            //!
            template <typename FLT, typename std::enable_if<std::is_floating_point<FLT>::value>::type* = nullptr>
            void setOptionalFloatAttribute(const UString& name, const std::optional<FLT>& value, size_t width = 0, size_t precision = 6, bool force_sign = false)
            {
                if (value.has_value()) {
                    refAttribute(name).setFloat<FLT>(value.value(), width, precision, force_sign);
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
            //! Set an optional attribute with an enumeration attribute to a node.
            //! @tparam ENUM An enum type.
            //! @param [in] definition The definition of enumeration values.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute optional value. If the variable is not set, no attribute is set.
            //!
            template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value>::type* = nullptr>
            void setOptionalEnumAttribute(const Enumeration& definition, const UString& name, const std::optional<ENUM>& value)
            {
                if (value.has_value()) {
                    refAttribute(name).setEnum(definition, int(value.value()));
                }
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
            //! Set a date (xithout hours) attribute of an XML element.
            //! @param [in] name Attribute name.
            //! @param [in] value Attribute value.
            //!
            void setDateAttribute(const UString& name, const Time& value)
            {
                refAttribute(name).setDate(value);
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
            void setIPAttribute(const UString& name, const IPv4Address& value)
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
            //! Get an optional string attribute of an XML element.
            //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
            //! @param [in] name Name of the attribute.
            //! @param [in] minSize Minimum allowed size for the value string.
            //! @param [in] maxSize Maximum allowed size for the value string.
            //! @return True on success, false on error.
            //!
            bool getOptionalAttribute(std::optional<UString>& value,
                                      const UString& name,
                                      size_t minSize = 0,
                                      size_t maxSize = UNLIMITED) const;

            //!
            //! Get an optional attribute of an XML element.
            //! getVariableAttribute() is different from getOptionalAttribute() in the result.
            //! With getOptionalAttribute(), if the attribute is missing, the std::optional is unset.
            //! With getVariableAttribute(), if the attribute is missing, the std::optional is set with the default value.
            //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @param [in] minSize Minimum allowed size for the value string.
            //! @param [in] maxSize Maximum allowed size for the value string.
            //! @return True on success, false on error.
            //!
            bool getVariableAttribute(std::optional<UString>& value,
                                      const UString& name,
                                      bool required = false,
                                      const UString& defValue = UString(),
                                      size_t minSize = 0,
                                      size_t maxSize = UNLIMITED) const
            {
                set_default(value, defValue);
                return getAttribute(value.value(), name, required, defValue, minSize, maxSize);
            }

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
            bool getOptionalBoolAttribute(std::optional<bool>& value, const UString& name) const;

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
            template <typename INT,
                      typename INT1 = INT,
                      typename INT2 = INT,
                      typename INT3 = INT,
                      typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            bool getIntAttribute(INT& value,
                                 const UString& name,
                                 bool required = false,
                                 INT1 defValue = static_cast<INT>(0),
                                 INT2 minValue = std::numeric_limits<INT>::min(),
                                 INT3 maxValue = std::numeric_limits<INT>::max()) const;

#if !defined(TSXML_GCC_TEMPLATE_SUBSTITUTION_BUG) || defined(DOXYGEN)
            //!
            //! Get an integer attribute of an XML element in an enum type.
            //! @tparam ENUM An enumeration type.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @param [in] minValue Minimum allowed value for the attribute.
            //! @param [in] maxValue Maximum allowed value for the attribute.
            //! @return True on success, false on error.
            //!
            template <typename ENUM,
                      typename INT1 = typename ts::underlying_type<ENUM>::type,
                      typename INT2 = typename ts::underlying_type<ENUM>::type,
                      typename std::enable_if<std::is_enum<ENUM>::value>::type* = nullptr, typename INT = typename std::underlying_type<ENUM>::type>
            bool getIntAttribute(ENUM& value,
                                 const UString& name,
                                 bool required = false,
                                 ENUM defValue = 0,
                                 INT1 minValue = static_cast<typename ts::underlying_type<ENUM>::type>(0),
                                 INT2 maxValue = std::numeric_limits<typename ts::underlying_type<ENUM>::type>::max()) const;
#endif

            //!
            //! Get an optional integer attribute of an XML element.
            //! @tparam INT An integer type.
            //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
            //! @param [in] name Name of the attribute.
            //! @param [in] minValue Minimum allowed value for the attribute.
            //! @param [in] maxValue Maximum allowed value for the attribute.
            //! @return True on success, false on error.
            //!
            template <typename INT,
                      typename INT1 = INT,
                      typename INT2 = INT,
                      typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            bool getOptionalIntAttribute(std::optional<INT>& value,
                                         const UString& name,
                                         INT1 minValue = std::numeric_limits<INT>::min(),
                                         INT2 maxValue = std::numeric_limits<INT>::max()) const;

            //!
            //! Get an optional integer attribute of an XML element.
            //! getVariableIntAttribute() is different from getOptionalIntAttribute() in the result.
            //! With getOptionalIntAttribute(), if the attribute is missing, the std::optional is unset.
            //! With getVariableIntAttribute(), if the attribute is missing, the std::optional is set with the default value.
            //! @tparam INT An integer type.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @param [in] minValue Minimum allowed value for the attribute.
            //! @param [in] maxValue Maximum allowed value for the attribute.
            //! @return True on success, false on error.
            //!
            template <typename INT,
                      typename INT1 = INT,
                      typename INT2 = INT,
                      typename INT3 = INT,
                      typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
            bool getVariableIntAttribute(std::optional<INT>& value,
                                         const UString& name,
                                         bool required = false,
                                         INT1 defValue = static_cast<INT>(0),
                                         INT2 minValue = std::numeric_limits<INT>::min(),
                                         INT3 maxValue = std::numeric_limits<INT>::max()) const
            {
                set_default(value, defValue);
                return getIntAttribute(value.value(), name, required, defValue, minValue, maxValue);
            }

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
            //! @tparam INT An integer or enum type.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] definition The definition of enumeration values.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            template <typename INT,
                      typename INT1 = INT,
                      typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type* = nullptr>
            bool getIntEnumAttribute(INT& value, const Enumeration& definition, const UString& name, bool required = false, INT1 defValue = INT(0)) const;

            //!
            //! Get an optional enumeration attribute of an XML element.
            //! Integer literals and integer values are accepted in the attribute.
            //! @tparam INT An integer or enum type.
            //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
            //! @param [in] definition The definition of enumeration values.
            //! @param [in] name Name of the attribute.
            //! @return True on success, false on error.
            //!
            template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type* = nullptr>
            bool getOptionalIntEnumAttribute(std::optional<INT>& value, const Enumeration& definition, const UString& name) const;

            //!
            //! Get an optional enumeration attribute of an XML element.
            //! Integer literals and integer values are accepted in the attribute.
            //! getVariableIntEnumAttribute() is different from getOptionalIntEnumAttribute() in the result.
            //! With getOptionalIntEnumAttribute(), if the attribute is missing, the std::optional is unset.
            //! With getVariableIntEnumAttribute(), if the attribute is missing, the std::optional is set with the default value.
            //! @tparam INT An integer or enum type.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] definition The definition of enumeration values.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            template <typename INT,
                      typename INT1 = INT,
                      typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type* = nullptr>
            bool getVariableIntEnumAttribute(std::optional<INT>& value, const Enumeration& definition, const UString& name, bool required = false, INT1 defValue = INT(0)) const
            {
                set_default(value, defValue);
                return getIntEnumAttribute(value.value(), definition, name, required, defValue);
            }

            //!
            //! Get a floating-point attribute of an XML element.
            //! @tparam FLT A floating-point type.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @param [in] minValue Minimum allowed value for the attribute.
            //! @param [in] maxValue Maximum allowed value for the attribute.
            //! @return True on success, false on error.
            //!
            template <typename FLT,
                      typename FLT1 = FLT,
                      typename FLT2 = FLT,
                      typename FLT3 = FLT,
                      typename std::enable_if<std::is_floating_point<FLT>::value>::type* = nullptr>
            bool getFloatAttribute(FLT& value,
                                   const UString& name,
                                   bool required = false,
                                   FLT1 defValue = static_cast<FLT>(0.0),
                                   FLT2 minValue = std::numeric_limits<FLT>::lowest(),
                                   FLT3 maxValue = std::numeric_limits<FLT>::max()) const;

            //!
            //! Get an optional floating-point attribute of an XML element.
            //! @tparam FLT A floating-point type.
            //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
            //! @param [in] name Name of the attribute.
            //! @param [in] minValue Minimum allowed value for the attribute.
            //! @param [in] maxValue Maximum allowed value for the attribute.
            //! @return True on success, false on error.
            //!
            template <typename FLT,
                      typename FLT1 = FLT,
                      typename FLT2 = FLT,
                      typename std::enable_if<std::is_floating_point<FLT>::value>::type* = nullptr>
            bool getOptionalFloatAttribute(std::optional<FLT>& value,
                                           const UString& name,
                                           FLT1 minValue = std::numeric_limits<FLT>::lowest(),
                                           FLT2 maxValue = std::numeric_limits<FLT>::max()) const;

            //!
            //! Get an optional floating-point attribute of an XML element.
            //! getVariableFloatAttribute() is different from getOptionalFloatAttribute() in the result.
            //! With getOptionalFloatAttribute(), if the attribute is missing, the std::optional is unset.
            //! With getVariableFloatAttribute(), if the attribute is missing, the std::optional is set with the default value.
            //! @tparam FLT A floating-point type.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @param [in] minValue Minimum allowed value for the attribute.
            //! @param [in] maxValue Maximum allowed value for the attribute.
            //! @return True on success, false on error.
            //!
            template <typename FLT,
                      typename FLT1 = FLT,
                      typename FLT2 = FLT,
                      typename FLT3 = FLT,
                      typename std::enable_if<std::is_floating_point<FLT>::value>::type* = nullptr>
            bool getVariableFloatAttribute(std::optional<FLT>& value,
                                           const UString& name,
                                           bool required = false,
                                           FLT1 defValue = static_cast<FLT>(0),
                                           FLT2 minValue = std::numeric_limits<FLT>::lowest(),
                                           FLT3 maxValue = std::numeric_limits<FLT>::max()) const
            {
                set_default(value, defValue);
                return getFloatAttribute(value.value(), name, required, defValue, minValue, maxValue);
            }

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
            //! Get a date (without hours) attribute of an XML element.
            //! @param [out] value Returned value of the attribute.
            //! @param [in] name Name of the attribute.
            //! @param [in] required If true, generate an error if the attribute is not found.
            //! @param [in] defValue Default value to return if the attribute is not present.
            //! @return True on success, false on error.
            //!
            bool getDateAttribute(Time& value, const UString& name, bool required = false, const Time& defValue = Time()) const;

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
            bool getIPAttribute(IPv4Address& value, const UString& name, bool required = false, const IPv4Address& defValue = IPv4Address()) const;

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
            //! Get the list of all attributes.
            //! @param [out] attr Returned map of all attribute names (index in the map) and corresponding values.
            //!
            void getAttributes(std::map<UString,UString>& attr) const;

            //!
            //! Get the list of all attribute names, sorted by modification order.
            //! The method is slower than getAttributesNames().
            //! @param [out] names Returned list of all attribute names.
            //!
            void getAttributesNamesInModificationOrder(UStringList& names) const;

            //!
            //! Recursively merge another element into this one.
            //! @param [in,out] other Another element to merge. The @a other object is destroyed,
            //! some of its nodes are reparented into the main object.
            //! @param [in] attrOptions What to do with attributes when merging nodes with identical tags.
            //! @return True on success, false on error.
            //!
            bool merge(Element* other, MergeAttributes attrOptions = MergeAttributes::ADD);

            //!
            //! Sort children elements by alphabetical order of tag name.
            //! @param [in] name When this parameter is not empty, recursively search for elements
            //! with that tag name and sort their children elements.
            //!
            void sort(const UString& name = UString());

            // Inherited from xml::Node.
            virtual Node* clone() const override;
            virtual void clear() override;
            virtual UString typeName() const override;
            virtual void print(TextFormatter& output, bool keepNodeOpen = false) const override;
            virtual void printClose(TextFormatter& output, size_t levels = std::numeric_limits<size_t>::max()) const override;

        protected:
            // Inherited from xml::Node.
            virtual bool parseNode(TextParser& parser, const Node* parent) override;

        private:
            CaseSensitivity _attributeCase {CASE_INSENSITIVE}; // For attribute names.
            AttributeMap _attributes {};

            // Compute the key in the attribute map.
            UString attributeKey(const UString& attributeName) const;

            // Find a key in the attribute map.
            AttributeMap::const_iterator findAttribute(const UString& attributeName) const;

            // Get a modifiable reference to an attribute, create if does not exist.
            Attribute& refAttribute(const UString& attributeName);
        };
    }
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Get an integer attribute of an XML element.
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

// Get an integer attribute of an XML element.
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


// Get an optional integer attribute of an XML element.
template <typename INT, typename INT1, typename INT2, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::xml::Element::getOptionalIntAttribute(std::optional<INT>& value, const UString& name, INT1 minValue, INT2 maxValue) const
{
    INT v = INT(0);
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else if (getIntAttribute<INT>(v, name, false, INT(0), minValue, maxValue)) {
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

// Get an enumeration attribute of an XML element.
template <typename INT, typename INT1, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type*>
bool ts::xml::Element::getIntEnumAttribute(INT& value, const Enumeration& definition, const UString& name, bool required, INT1 defValue) const
{
    int v = 0;
    const bool ok = getEnumAttribute(v, definition, name, required, int(defValue));
    value = ok ? INT(v) : INT(defValue);
    return ok;
}

// Get an optional enumeration attribute of an XML element.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type*>
bool ts::xml::Element::getOptionalIntEnumAttribute(std::optional<INT>& value, const Enumeration& definition, const UString& name) const
{
    INT v = INT(0);
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else if (getIntEnumAttribute<INT>(v, definition, name, false)) {
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

// Get a floating-point attribute of an XML element.
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

// Get an optional floating-point attribute of an XML element.
template <typename FLT, typename FLT1, typename FLT2, typename std::enable_if<std::is_floating_point<FLT>::value>::type*>
bool ts::xml::Element::getOptionalFloatAttribute(std::optional<FLT>& value, const UString& name, FLT1 minValue, FLT2 maxValue) const
{
    FLT v = FLT(0.0);
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else if (getFloatAttribute<FLT>(v, name, false, FLT(0.0), minValue, maxValue)) {
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
