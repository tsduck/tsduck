//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsIPAddress.h"
#include "tsMACAddress.h"
#include "tsIntegerUtils.h"

namespace ts::xml {

    class Text;

    //!
    //! Structured element in an XML document.
    //! @ingroup libtscore xml
    //!
    class TSCOREDLL Element: public Node
    {
    private:
        // Attributes are stored indexed by case-(in)sensitive name.
        using AttributeMap = std::map<UString, Attribute>;

    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] line Line number in input document.
        //! @param [in] attribute_case State if attribute names are stored with case sensitivity.
        //!
        explicit Element(Report& report = NULLREP, size_t line = 0, CaseSensitivity attribute_case = CASE_INSENSITIVE);

        //!
        //! Constructor.
        //! @param [in,out] parent The parent into which the element is added.
        //! @param [in] name Name of the element.
        //! @param [in] attribute_case State if attribute names are stored wit case sensitivity.
        //! @param [in] last If true, the child is added at the end of the list of children.
        //! If false, it is added at the beginning.
        //!
        Element(Node* parent, const UString& name, CaseSensitivity attribute_case = CASE_INSENSITIVE, bool last = true);

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
        //! Get the parent name.
        //! This is the same as parent()->name() without error when there is not parent.
        //! @return A constant reference to the parent name or to an empty string if there is no parent.
        //!
        const UString& parentName() const;

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
        //! @param [in] min_count Minimum required number of elements of that name.
        //! @param [in] max_count Maximum allowed number of elements of that name.
        //! @return True on success, false on error.
        //!
        bool getChildren(ElementVector& children, const UString& name, size_t min_count = 0, size_t max_count = UNLIMITED) const;

        //!
        //! Check if the element contains at least 1 named child element, case-insensitive.
        //! @param [in] name Name of the child element to search.
        //! @return True if present, false if not present.
        //!
        bool hasChildElement(const UString& name) const;

        //!
        //! Get text in a child of an element.
        //! @param [out] data The content of the text in the child element.
        //! @param [in] name Name of the child element to search.
        //! @param [in] trim If true, remove leading and trailing spaces.
        //! @param [in] required If true, generate an error if the child element is not found.
        //! @param [in] def_value Default value to return if the child element is not present.
        //! @param [in] min_size Minimum allowed size for the value string.
        //! @param [in] max_size Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getTextChild(UString& data,
                          const UString& name,
                          bool trim = false,
                          bool required = false,
                          const UString& def_value = UString(),
                          size_t min_size = 0,
                          size_t max_size = UNLIMITED) const;

        //!
        //! Get text inside an element.
        //! In practice, concatenate the content of all Text children inside the element.
        //! @param [out] data The content of the text children.
        //! @param [in] trim If true, remove leading and trailing spaces.
        //! @param [in] min_size Minimum allowed size for the value string.
        //! @param [in] max_size Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getText(UString& data, bool trim = false, size_t min_size = 0, size_t max_size = UNLIMITED) const;

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
        //! @param [in] min_size Minimum allowed size for the value string.
        //! @param [in] max_size Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getHexaTextChild(ByteBlock& data,
                              const UString& name,
                              bool required = false,
                              size_t min_size = 0,
                              size_t max_size = UNLIMITED) const;

        //!
        //! Get and interpret the hexadecimal data inside the element.
        //! In practice, concatenate the content of all Text children inside the element
        //! and interpret the result as hexadecimal data.
        //! @param [out] data Buffer receiving the decoded hexadecimal data.
        //! @param [in] min_size Minimum size of the returned data.
        //! @param [in] max_size Maximum size of the returned data.
        //! @return True on success, false on error.
        //!
        bool getHexaText(ByteBlock& data, size_t min_size = 0, size_t max_size = UNLIMITED) const;

        //!
        //! Add a new child element at the end of children.
        //! @param [in] childName Name of new child element to create.
        //! @return New child element or null on error.
        //!
        Element* addElement(const UString& childName);

        //!
        //! Add a new text inside this node.
        //! @param [in] text Text string to add.
        //! @param [in] only_not_empty When true, do not add the text if the string is empty.
        //! @return New child element or null on error.
        //!
        Text* addText(const UString& text, bool only_not_empty = false);

        //!
        //! Add a new text containing hexadecimal data inside this node.
        //! @param [in] data Address of binary data.
        //! @param [in] size Size in bytes of binary data.
        //! @param [in] only_not_empty When true, do not add the child element if the data is empty.
        //! @return New child element or null on error or empty data.
        //!
        Text* addHexaText(const void* data, size_t size, bool only_not_empty = false);

        //!
        //! Add a new text containing hexadecimal data inside this node.
        //! @param [in] data Binary data.
        //! @param [in] only_not_empty When true, do not add the child element if the data is empty.
        //! @return New child element or null on error or empty data.
        //!
        Text* addHexaText(const ByteBlock& data, bool only_not_empty = false)
        {
            return addHexaText(data.data(), data.size(), only_not_empty);
        }

        //!
        //! Add a new child element containing an hexadecimal data text.
        //! @param [in] name Name of the child element to search.
        //! @param [in] data Address of binary data.
        //! @param [in] size Size in bytes of binary data.
        //! @param [in] only_not_empty When true, do not add the child element if the data is empty.
        //! @return New child element or null on error or empty data.
        //!
        Text* addHexaTextChild(const UString& name, const void* data, size_t size, bool only_not_empty = false);

        //!
        //! Add a new child element containing an hexadecimal data text.
        //! @param [in] name Name of the child element to search.
        //! @param [in] data Binary data.
        //! @param [in] only_not_empty When true, do not add the child element if the data is empty.
        //! @return New child element or null on error or empty data.
        //!
        Text* addHexaTextChild(const UString& name, const ByteBlock& data, bool only_not_empty = false);

        //!
        //! Check if an attribute exists in the element.
        //! @param [in] attribute_name Attribute name.
        //! @return True if the attribute exists.
        //!
        bool hasAttribute(const UString& attribute_name) const;

        //!
        //! Check if an attribute exists in the element and has the specified value.
        //! @param [in] attribute_name Attribute name.
        //! @param [in] value Expected value.
        //! @param [in] similar If true, the comparison between the actual and expected
        //! values is performed case-insensitive and ignoring blanks. Additioanlly,
        //! if the values are integers, the integer values are compared (otherwise,
        //! identical values in decimal and hexadecimal wouldn't match).
        //! If @a similar is false, a strict comparison is performed.
        //! @return True if the attribute exists and has the expected value.
        //!
        bool hasAttribute(const UString& attribute_name, const UString& value, bool similar = false) const;

        //!
        //! Get an attribute.
        //! @param [in] attribute_name Attribute name.
        //! @param [in] silent If true, do not report error.
        //! @return A constant reference to an attribute.
        //! If the argument does not exist, the referenced object is marked invalid.
        //! The reference is valid as long as the Element object is not modified.
        //!
        const Attribute& attribute(const UString& attribute_name, bool silent = false) const;

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
        //! Set an optional attribute to a node.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        void setOptionalAttribute(const UString& name, const std::optional<UString>& value)
        {
            if (value.has_value()) {
                setAttribute(name, value.value());
            }
        }

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
        template <typename INT> requires std::integral<INT>
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
        template <typename INT> requires std::integral<INT>
        void setOptionalIntAttribute(const UString& name, const std::optional<INT>& value, bool hexa = false)
        {
            if (value.has_value()) {
                refAttribute(name).setInteger<INT>(value.value(), hexa);
            }
        }

        //!
        //! Set an attribute with a std::chrono::duration value to a node.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //! @param [in] hexa If true, use an hexadecimal representation (0x...).
        //!
        template <class Rep, class Period>
        void setChronoAttribute(const UString& name, cn::duration<Rep,Period> value, bool hexa = false)
        {
            refAttribute(name).setInteger<Rep>(value.count(), hexa);
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
        template <typename FLT> requires std::floating_point<FLT>
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
        template <typename FLT> requires std::floating_point<FLT>
        void setOptionalFloatAttribute(const UString& name, const std::optional<FLT>& value, size_t width = 0, size_t precision = 6, bool force_sign = false)
        {
            if (value.has_value()) {
                refAttribute(name).setFloat<FLT>(value.value(), width, precision, force_sign);
            }
        }

        //!
        //! Set an enumeration attribute of a node.
        //! @tparam INT An integer or enum type.
        //! @param [in] definition The definition of enumeration values.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        template <typename INT> requires ts::int_enum<INT>
        void setEnumAttribute(const Names& definition, const UString& name, INT value)
        {
            refAttribute(name).setEnum(definition, value);
        }

        //!
        //! Set an optional attribute with an enumeration attribute to a node.
        //! @tparam INT An integer or enum type.
        //! @param [in] definition The definition of enumeration values.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute optional value. If the variable is not set, no attribute is set.
        //!
        template <typename INT> requires ts::int_enum<INT>
        void setOptionalEnumAttribute(const Names& definition, const UString& name, const std::optional<INT>& value)
        {
            if (value.has_value()) {
                refAttribute(name).setEnum(definition, value.value());
            }
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
        //! Set an optional date/time attribute of an XML element.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        void setOptionalDateTimeAttribute(const UString& name, const std::optional<Time>& value)
        {
            if (value.has_value()) {
                refAttribute(name).setDateTime(value.value());
            }
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
        //! Set an optional date (xithout hours) attribute of an XML element.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        void setOptionalDateAttribute(const UString& name, const std::optional<Time>& value)
        {
            if (value.has_value()) {
                refAttribute(name).setDate(value.value());
            }
        }

        //!
        //! Set a time attribute of an XML element in "hh:mm:ss" format.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        template <class Rep, class Period>
        void setTimeAttribute(const UString& name, const cn::duration<Rep,Period>& value)
        {
            refAttribute(name).setTime(value);
        }

        //!
        //! Set an optional time attribute of an XML element in "hh:mm:ss" format.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        template <class Rep, class Period>
        void setOptionalTimeAttribute(const UString& name, const std::optional<cn::duration<Rep,Period>>& value)
        {
            if (value.has_value()) {
                refAttribute(name).setTime(value.value());
            }
        }

        //!
        //! Set an IPv4 or IPv6 address attribute of an XML element.
        //! @param [in] name Attribute name.
        //! @param [in] value Attribute value.
        //!
        void setIPAttribute(const UString& name, const IPAddress& value)
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
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @param [in] min_size Minimum allowed size for the value string.
        //! @param [in] max_size Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getAttribute(UString& value,
                          const UString& name,
                          bool required = false,
                          const UString& def_value = UString(),
                          size_t min_size = 0,
                          size_t max_size = UNLIMITED) const;

        //!
        //! Get an optional string attribute of an XML element.
        //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
        //! @param [in] name Name of the attribute.
        //! @param [in] min_size Minimum allowed size for the value string.
        //! @param [in] max_size Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getOptionalAttribute(std::optional<UString>& value,
                                  const UString& name,
                                  size_t min_size = 0,
                                  size_t max_size = UNLIMITED) const;

        //!
        //! Get an optional attribute of an XML element.
        //! getVariableAttribute() is different from getOptionalAttribute() in the result.
        //! With getOptionalAttribute(), if the attribute is missing, the std::optional is unset.
        //! With getVariableAttribute(), if the attribute is missing, the std::optional is set with the default value.
        //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @param [in] min_size Minimum allowed size for the value string.
        //! @param [in] max_size Maximum allowed size for the value string.
        //! @return True on success, false on error.
        //!
        bool getVariableAttribute(std::optional<UString>& value,
                                  const UString& name,
                                  bool required = false,
                                  const UString& def_value = UString(),
                                  size_t min_size = 0,
                                  size_t max_size = UNLIMITED) const
        {
            set_default(value, def_value);
            return getAttribute(value.value(), name, required, def_value, min_size, max_size);
        }

        //!
        //! Get a boolean attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        bool getBoolAttribute(bool& value, const UString& name, bool required = false, bool def_value = false) const;

        //!
        //! Get an optional boolean attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @return True on success, false on error.
        //!
        bool getOptionalBoolAttribute(std::optional<bool>& value, const UString& name) const;

        //!
        //! Get an integer or enum attribute of an XML element.
        //! @tparam INT An integer or enum type.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @param [in] min_value Minimum allowed value for the attribute.
        //! @param [in] max_value Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <typename INT, typename INT1 = INT, typename INT2 = INT, typename INT3 = INT>
            requires ts::int_enum<INT> && ts::int_enum<INT1> && ts::int_enum<INT2> && ts::int_enum<INT3>
        bool getIntAttribute(INT& value,
                             const UString& name,
                             bool required = false,
                             INT1 def_value = static_cast<INT>(0),
                             INT2 min_value = std::numeric_limits<typename ts::underlying_type<INT>::type>::min(),
                             INT3 max_value = std::numeric_limits<typename ts::underlying_type<INT>::type>::max()) const;

        //!
        //! Get an integer or enum attribute of an XML element.
        //! @tparam INT An integer type.
        //! @param [out] value Returned value of the attribute. Always set, possibly to the default value.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @param [in] min_value Minimum allowed value for the attribute.
        //! @param [in] max_value Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <typename INT, typename INT1 = INT, typename INT2 = INT, typename INT3 = INT>
            requires ts::int_enum<INT> && ts::int_enum<INT1> && ts::int_enum<INT2> && ts::int_enum<INT3>
        bool getIntAttribute(std::optional<INT>& value,
                             const UString& name,
                             bool required = false,
                             INT1 def_value = static_cast<INT>(0),
                             INT2 min_value = std::numeric_limits<typename ts::underlying_type<INT>::type>::min(),
                             INT3 max_value = std::numeric_limits<typename ts::underlying_type<INT>::type>::max()) const
        {
            set_default(value, def_value);
            return getIntAttribute(value.value(), name, required, def_value, min_value, max_value);
        }

        //!
        //! Get an optional integer or enum attribute of an XML element.
        //! @tparam INT An integer or enum type.
        //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
        //! @param [in] name Name of the attribute.
        //! @param [in] min_value Minimum allowed value for the attribute.
        //! @param [in] max_value Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <typename INT, typename INT1 = INT, typename INT2 = INT>
            requires ts::int_enum<INT> && ts::int_enum<INT1> && ts::int_enum<INT2>
        bool getOptionalIntAttribute(std::optional<INT>& value,
                                     const UString& name,
                                     INT1 min_value = std::numeric_limits<typename ts::underlying_type<INT>::type>::min(),
                                     INT2 max_value = std::numeric_limits<typename ts::underlying_type<INT>::type>::max()) const;

        //!
        //! Get an integer or enum attribute of an XML element, based on a condition.
        //! @tparam INT An integer or enum type.
        //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
        //! @param [in] name Name of the attribute.
        //! @param [in] condition If true, the attribute must be present. If false, the attribute must nbot be present.
        //! @param [in] min_value Minimum allowed value for the attribute.
        //! @param [in] max_value Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <typename INT, typename INT1 = INT, typename INT2 = INT>
            requires ts::int_enum<INT> && ts::int_enum<INT1> && ts::int_enum<INT2>
        bool getConditionalIntAttribute(std::optional<INT>& value,
                                        const UString& name,
                                        bool condition,
                                        INT1 min_value = std::numeric_limits<typename ts::underlying_type<INT>::type>::min(),
                                        INT2 max_value = std::numeric_limits<typename ts::underlying_type<INT>::type>::max()) const;

        //!
        //! Get an enumeration attribute of an XML element.
        //! Integer literals and integer values are accepted in the attribute.
        //! @tparam INT An integer or enum type.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] definition The definition of enumeration values.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        template <typename INT, typename INT1 = INT>
            requires ts::int_enum<INT> && ts::int_enum<INT1>
        bool getEnumAttribute(INT& value, const Names& definition, const UString& name, bool required = false, INT1 def_value = INT(0)) const;

        //!
        //! Get an enumeration attribute of an XML element.
        //! @tparam INT An integer or enum type.
        //! @param [out] value Returned value of the attribute. Always set, possibly to the default value.
        //! @param [in] definition The definition of enumeration values.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        template <typename INT, typename INT1 = INT>
            requires ts::int_enum<INT> && ts::int_enum<INT1>
        bool getEnumAttribute(std::optional<INT>& value, const Names& definition, const UString& name, bool required = false, INT1 def_value = INT(0)) const
        {
            set_default(value, def_value);
            return getEnumAttribute(value.value(), definition, name, required, def_value);
        }

        //!
        //! Get an optional enumeration attribute of an XML element.
        //! Integer literals and integer values are accepted in the attribute.
        //! @tparam INT An integer or enum type.
        //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
        //! @param [in] definition The definition of enumeration values.
        //! @param [in] name Name of the attribute.
        //! @return True on success, false on error.
        //!
        template <typename INT>
            requires ts::int_enum<INT>
        bool getOptionalEnumAttribute(std::optional<INT>& value, const Names& definition, const UString& name) const;

        //!
        //! Get a floating-point attribute of an XML element.
        //! @tparam FLT A floating-point type.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @param [in] min_value Minimum allowed value for the attribute.
        //! @param [in] max_value Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <typename FLT, typename FLT1 = FLT, typename FLT2 = FLT, typename FLT3 = FLT>
            requires std::floating_point<FLT> && std::is_arithmetic_v<FLT1> && std::is_arithmetic_v<FLT2> && std::is_arithmetic_v<FLT3>
        bool getFloatAttribute(FLT& value,
                               const UString& name,
                               bool required = false,
                               FLT1 def_value = static_cast<FLT>(0.0),
                               FLT2 min_value = std::numeric_limits<FLT>::lowest(),
                               FLT3 max_value = std::numeric_limits<FLT>::max()) const;

        //!
        //! Get an optional floating-point attribute of an XML element.
        //! @tparam FLT A floating-point type.
        //! @param [out] value Returned value of the attribute. If the attribute is not present, the variable is reset.
        //! @param [in] name Name of the attribute.
        //! @param [in] min_value Minimum allowed value for the attribute.
        //! @param [in] max_value Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <typename FLT, typename FLT1 = FLT, typename FLT2 = FLT>
            requires std::floating_point<FLT> && std::is_arithmetic_v<FLT1> && std::is_arithmetic_v<FLT2>
        bool getOptionalFloatAttribute(std::optional<FLT>& value,
                                       const UString& name,
                                       FLT1 min_value = std::numeric_limits<FLT>::lowest(),
                                       FLT2 max_value = std::numeric_limits<FLT>::max()) const;

        //!
        //! Get an optional floating-point attribute of an XML element.
        //! getVariableFloatAttribute() is different from getOptionalFloatAttribute() in the result.
        //! With getOptionalFloatAttribute(), if the attribute is missing, the std::optional is unset.
        //! With getVariableFloatAttribute(), if the attribute is missing, the std::optional is set with the default value.
        //! @tparam FLT A floating-point type.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @param [in] min_value Minimum allowed value for the attribute.
        //! @param [in] max_value Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <typename FLT, typename FLT1 = FLT, typename FLT2 = FLT, typename FLT3 = FLT>
            requires std::floating_point<FLT> && std::is_arithmetic_v<FLT1> && std::is_arithmetic_v<FLT2> && std::is_arithmetic_v<FLT3>
        bool getVariableFloatAttribute(std::optional<FLT>& value,
                                       const UString& name,
                                       bool required = false,
                                       FLT1 def_value = static_cast<FLT>(0),
                                       FLT2 min_value = std::numeric_limits<FLT>::lowest(),
                                       FLT3 max_value = std::numeric_limits<FLT>::max()) const
        {
            set_default(value, def_value);
            return getFloatAttribute(value.value(), name, required, def_value, min_value, max_value);
        }

        //!
        //! Get a std::chrono::duration attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @param [in] min_value Minimum allowed value for the attribute.
        //! @param [in] max_value Maximum allowed value for the attribute.
        //! @return True on success, false on error.
        //!
        template <class Rep, class Period>
        bool getChronoAttribute(cn::duration<Rep, Period>& value,
                                const UString& name,
                                bool required = false,
                                const cn::duration<Rep, Period>& def_value = cn::duration<Rep, Period>::zero(),
                                const cn::duration<Rep, Period>& min_value = cn::duration<Rep, Period>::min(),
                                const cn::duration<Rep, Period>& max_value = cn::duration<Rep, Period>::max()) const;

        //!
        //! Get a date/time attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        bool getDateTimeAttribute(Time& value, const UString& name, bool required = false, const Time& def_value = Time()) const;

        //!
        //! Get an optional date/time attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @return True on success, false on error.
        //!
        bool getOptionalDateTimeAttribute(std::optional<Time>& value, const UString& name) const;

        //!
        //! Get a date (without hours) attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        bool getDateAttribute(Time& value, const UString& name, bool required = false, const Time& def_value = Time()) const;

        //!
        //! Get an optional date (without hours) attribute of an XML element.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @return True on success, false on error.
        //!
        bool getOptionalDateAttribute(std::optional<Time>& value, const UString& name) const;

        //!
        //! Get a time attribute of an XML element in "hh:mm:ss" format.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @return True on success, false on error.
        //!
        template <class Rep, class Period>
        bool getTimeAttribute(cn::duration<Rep,Period>& value, const UString& name, bool required = false) const
        {
            return getTimeAttribute(value, name, required, cn::duration<Rep,Period>::zero());
        }

        //!
        //! Get a time attribute of an XML element in "hh:mm:ss" format.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        template <class Rep1, class Period1, class Rep2, class Period2>
        bool getTimeAttribute(cn::duration<Rep1,Period1>& value, const UString& name, bool required, const cn::duration<Rep2,Period2>& def_value) const;

        //!
        //! Get an optional time attribute of an XML element in "hh:mm:ss" format.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @return True on success, false on error.
        //!
        template <class Rep, class Period>
        bool getOptionalTimeAttribute(std::optional<cn::duration<Rep,Period>>& value, const UString& name) const;

        //!
        //! Get an IPv4 or IPv6 address attribute of an XML element in numerical format or host name.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        bool getIPAttribute(IPAddress& value, const UString& name, bool required = false, const IPAddress& def_value = IPAddress()) const;

        //!
        //! Get a MAC address attribute of an XML element in "x:x:x:x:x:x" format.
        //! @param [out] value Returned value of the attribute.
        //! @param [in] name Name of the attribute.
        //! @param [in] required If true, generate an error if the attribute is not found.
        //! @param [in] def_value Default value to return if the attribute is not present.
        //! @return True on success, false on error.
        //!
        bool getMACAttribute(MACAddress& value, const UString& name, bool required = false, const MACAddress& def_value = MACAddress()) const;

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
        //! Get the number of attributes in the element.
        //! @return The number of attributes in the element.
        //!
        size_t getAttributesCount() const { return _attributes.size(); }

        //!
        //! Recursively merge another element into this one.
        //! @param [in,out] other Another element to merge. The @a other object is destroyed,
        //! some of its nodes are reparented into the main object.
        //! @param [in] attr_options What to do with attributes when merging nodes with identical tags.
        //! @return True on success, false on error.
        //!
        bool merge(Element* other, MergeAttributes attr_options = MergeAttributes::ADD);

        //!
        //! Sort children elements by alphabetical order of tag name.
        //! @param [in] name When this parameter is not empty, recursively search for elements
        //! with that tag name and sort their children elements.
        //!
        void sort(const UString& name = UString());

        // Inherited from xml::Node.
        virtual Node* clone() const override;
        virtual void clear() override;
        virtual void expandEnvironment(bool recurse) override;
        virtual UString typeName() const override;
        virtual void print(TextFormatter& output, bool keepNodeOpen = false) const override;
        virtual void printClose(TextFormatter& output, size_t levels = std::numeric_limits<size_t>::max()) const override;

    protected:
        // Inherited from xml::Node.
        virtual bool parseNode(TextParser& parser, const Node* parent) override;

    private:
        CaseSensitivity _attribute_case = CASE_INSENSITIVE; // For attribute names.
        AttributeMap _attributes {};

        // Compute the key in the attribute map.
        UString attributeKey(const UString& attribute_name) const;

        // Find a key in the attribute map.
        AttributeMap::const_iterator findAttribute(const UString& attribute_name) const;

        // Get a modifiable reference to an attribute, create if does not exist.
        Attribute& refAttribute(const UString& attribute_name);
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Get an integer or enum attribute of an XML element.
template <typename INT, typename INT1, typename INT2, typename INT3>
    requires ts::int_enum<INT> && ts::int_enum<INT1> && ts::int_enum<INT2> && ts::int_enum<INT3>
bool ts::xml::Element::getIntAttribute(INT& value, const UString& name, bool required, INT1 def_value, INT2 min_value, INT3 max_value) const
{
    const Attribute& attr(attribute(name, !required));
    if (!attr.isValid()) {
        // Attribute not present.
        value = INT(def_value);
        return !required;
    }

    // Attribute found, get its value.
    using INTMAX = typename ts::int_max<INT>::type;
    UString str(attr.value());
    INTMAX val = 0;
    if (!str.toInteger(val, u",")) {
        report().error(u"'%s' is not a valid integer value for attribute '%s' in <%s>, line %d", str, name, this->name(), lineNumber());
        return false;
    }
    else if (val < INTMAX(min_value) || val > INTMAX(max_value)) {
        report().error(u"'%s' must be in range %'d to %'d for attribute '%s' in <%s>, line %d", str, min_value, max_value, name, this->name(), lineNumber());
        return false;
    }
    else {
        value = INT(val);
        return true;
    }
}

// Get an optional integer attribute of an XML element.
template <typename INT, typename INT1, typename INT2>
    requires ts::int_enum<INT> && ts::int_enum<INT1> && ts::int_enum<INT2>
bool ts::xml::Element::getOptionalIntAttribute(std::optional<INT>& value, const UString& name, INT1 min_value, INT2 max_value) const
{
    INT v = INT(0);
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else if (getIntAttribute<INT>(v, name, false, INT(0), min_value, max_value)) {
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

// Get an integer attribute of an XML element, based on a condition.
template <typename INT, typename INT1, typename INT2>
    requires ts::int_enum<INT> && ts::int_enum<INT1> && ts::int_enum<INT2>
bool ts::xml::Element::getConditionalIntAttribute(std::optional<INT>& value, const UString& name, bool condition, INT1 min_value, INT2 max_value) const
{
    value.reset();
    INT v = INT(0);
    const bool present = hasAttribute(name);
    if (!present && !condition) {
        // Attribute not present, ok.
        return true;
    }
    else if (present && !condition) {
        // Attribute present, but should not be.
        report().error(u"<%s>, line %d, attribute '%s' is forbidden in this context", this->name(), lineNumber(), name);
        return false;
    }
    else if (getIntAttribute<INT>(v, name, true, INT(0), min_value, max_value)) {
        // Attribute present, correct value.
        value = v;
        return true;
    }
    else {
        // Attribute present with incorrect value, or absent when it should be present.
        return false;
    }
}

// Get an enumeration attribute of an XML element.
template <typename INT, typename INT1>
    requires ts::int_enum<INT> && ts::int_enum<INT1>
bool ts::xml::Element::getEnumAttribute(INT& value, const Names& definition, const UString& name, bool required, INT1 def_value) const
{
    const Attribute& attr(attribute(name, !required));
    if (!attr.isValid()) {
        // Attribute not present.
        value = INT(def_value);
        return !required;
    }
    else {
        // Attribute found, get its value.
        const UString str(attr.value());
        const Names::int_t val = definition.value(str, false);
        if (val == Names::UNKNOWN) {
            report().error(u"'%s' is not a valid value for attribute '%s' in <%s>, line %d", str, name, this->name(), lineNumber());
            return false;
        }
        else {
            value = INT(val);
            return true;
        }
    }
}

// Get an optional enumeration attribute of an XML element.
template <typename INT>
    requires ts::int_enum<INT>
bool ts::xml::Element::getOptionalEnumAttribute(std::optional<INT>& value, const Names& definition, const UString& name) const
{
    INT v = INT(0);
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else if (getEnumAttribute<INT>(v, definition, name, false)) {
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
template <typename FLT, typename FLT1, typename FLT2, typename FLT3>
    requires std::floating_point<FLT> && std::is_arithmetic_v<FLT1> && std::is_arithmetic_v<FLT2> && std::is_arithmetic_v<FLT3>
bool ts::xml::Element::getFloatAttribute(FLT& value, const UString& name, bool required, FLT1 def_value, FLT2 min_value, FLT3 max_value) const
{
    const Attribute& attr(attribute(name, !required));
    if (!attr.isValid()) {
        // Attribute not present.
        value = FLT(def_value);
        return !required;
    }

    // Attribute found, get its value.
    UString str(attr.value());
    FLT val = FLT(0.0);
    if (!str.toFloat(val)) {
        report().error(u"'%s' is not a valid floating-point value for attribute '%s' in <%s>, line %d", str, name, this->name(), lineNumber());
        return false;
    }
    else if (val < FLT(min_value) || val > FLT(max_value)) {
        report().error(u"'%s' must be in range %f to %f for attribute '%s' in <%s>, line %d", str, double(min_value), double(max_value), name, this->name(), lineNumber());
        return false;
    }
    else {
        value = val;
        return true;
    }
}

// Get an optional floating-point attribute of an XML element.
template <typename FLT, typename FLT1, typename FLT2>
    requires std::floating_point<FLT> && std::is_arithmetic_v<FLT1> && std::is_arithmetic_v<FLT2>
bool ts::xml::Element::getOptionalFloatAttribute(std::optional<FLT>& value, const UString& name, FLT1 min_value, FLT2 max_value) const
{
    FLT v = FLT(0.0);
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else if (getFloatAttribute<FLT>(v, name, false, FLT(0.0), min_value, max_value)) {
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

// Get a time attribute of an XML element in "hh:mm:ss" format.
template <class Rep1, class Period1, class Rep2, class Period2>
bool ts::xml::Element::getTimeAttribute(cn::duration<Rep1,Period1>& value, const UString& name, bool required, const cn::duration<Rep2,Period2>& def_value) const
{
    UString str;
    if (!getAttribute(str, name, required)) {
        return false;
    }
    if (!required && str.empty()) {
        value = cn::duration_cast<cn::duration<Rep1,Period1>>(def_value);
        return true;
    }

    // Analyze the time string.
    const bool ok = Attribute::TimeFromString(value, str);
    if (!ok) {
        report().error(u"'%s' is not a valid time for attribute '%s' in <%s>, line %d, use \"hh:mm:ss\"", str, name, this->name(), lineNumber());
    }
    return ok;
}

// Get an optional time attribute of an XML element in "hh:mm:ss" format.
template <class Rep, class Period>
bool ts::xml::Element::getOptionalTimeAttribute(std::optional<cn::duration<Rep,Period>>& value, const UString& name) const
{
    if (!hasAttribute(name)) {
        // Attribute not present, ok.
        value.reset();
        return true;
    }
    else {
        value.emplace(0);
        const bool ok = getTimeAttribute(value.value(), name, true);
        if (!ok) {
            value.reset();
        }
        return ok;
    }
}

// Get a std::chrono::duration attribute of an XML element.
template <class Rep, class Period>
bool ts::xml::Element::getChronoAttribute(cn::duration<Rep, Period>& value,
                                          const UString& name,
                                          bool required,
                                          const cn::duration<Rep, Period>& def_value,
                                          const cn::duration<Rep, Period>& min_value,
                                          const cn::duration<Rep, Period>& max_value) const
{
    using Duration = cn::duration<Rep, Period>;
    typename Duration::rep ivalue = 0;
    const bool ok = getIntAttribute(ivalue, name, required, def_value.count(), min_value.count(), max_value.count());
    value = Duration(ivalue);
    return ok;
}
