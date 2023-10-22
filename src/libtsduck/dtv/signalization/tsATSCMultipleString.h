//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ATSC multiple_string_structure.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {

    // Incomplete declarations:
    class DuckContext;
    class TablesDisplay;
    namespace xml {
        class Element;
    }

    //!
    //! Representation of an ATSC multiple_string_structure.
    //!
    //! An ATSC multiple_string_structure is a set of strings. Each string has
    //! a language code and a compression mode. In this implementation, we only
    //! support non-compressed text.
    //!
    //! @see ATSC A/65, section 6.10.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL ATSCMultipleString
    {
    public:
        //!
        //! Default constructor.
        //!
        ATSCMultipleString() = default;

        //!
        //! Constructor with one string.
        //! @param [in] language 3-character ISO-639 language code.
        //! @param [in] text Text string.
        //!
        ATSCMultipleString(const UString& language, const UString& text);

        //!
        //! Clear all strings.
        //!
        void clear() { _strings.clear(); }

        //!
        //! Check if this instance has strings.
        //! @return True if there is no string in this instance.
        //!
        bool empty() const { return _strings.empty(); }

        //!
        //! Get the number of strings in this instance.
        //! @return The number of strings in this instance.
        //!
        size_t size() const { return _strings.size(); }

        //!
        //! Set the number of strings in this instance.
        //! @param [in] count The new number of strings. Either truncate current list
        //! of strings or create additional empty strings.
        //!
        void resize(size_t count) { _strings.resize(count); }

        //!
        //! Allocate the appropriate memory for a target number of strings.
        //! The actual number of strings in unchanged.
        //! @param [in] count The target number of strings.
        //!
        void reserve(size_t count) { _strings.reserve(count); }

        //!
        //! Get the language of the specified string (first string by default).
        //! @param [in] index String index.
        //! @return The 3-character ISO-639 language code for the specified (or first) string.
        //! Empty string if the string does not exist.
        //!
        UString language(size_t index = 0) const;

        //!
        //! Get the concatenation of all texts of the specified language.
        //! @param [in] language 3-character ISO-639 language code. If empty, use the language code of the first string.
        //! @return The concatenation of all texts of the specified language.
        //!
        UString text(const UString& language = UString()) const;

        //!
        //! Get the text of the specified string.
        //! @param [in] index String index.
        //! @return The text for the specified string.
        //! Empty string if the string does not exist.
        //!
        UString text(size_t index) const;

        //!
        //! Search the first string with a given language.
        //! @param [in] language 3-character ISO-639 language code.
        //! @return The index of the first string with the specified language or @c NPOS if not found.
        //!
        size_t searchLanguage(const UString& language) const;

        //!
        //! Check if a given language is present.
        //! @param [in] language 3-character ISO-639 language code.
        //! @return True if the language is present.
        //!
        bool hasLanguage(const UString& language) const { return searchLanguage(language) != NPOS; }

        //!
        //! Add a new string.
        //! @param [in] language 3-character ISO-639 language code.
        //! @param [in] text Text string.
        //!
        void add(const UString& language, const UString& text);

        //!
        //! Set the value of an existing string.
        //! @param [in] index String index.
        //! @param [in] language 3-character ISO-639 language code.
        //! @param [in] text Text string.
        //! @return True if the string was set, false if @a index is out of range.
        //!
        bool set(size_t index, const UString& language, const UString& text);

        //!
        //! Append text to an existing string.
        //! @param [in] index String index.
        //! @param [in] text Text string to append.
        //! @return True if the string was updated, false if @a index is out of range.
        //!
        bool append(size_t index, const UString& text);

        //!
        //! Convert to an XML structure.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent Parent XML node.
        //! @param [in] name Name of the XML node to create.
        //! @param [in] ignore_empty Do not insert the node if the structure is empty.
        //! @return The created XML element or @c nullptr on error or empty and @a ignore_empty is true.
        //!
        //! An ATSC multiple_string_structure can be represented as an XML
        //! element with a predefined structure. The name is application-dependent.
        //! The XML structure is the following:
        //! @code
        //! <XXX>
        //!   <string language="char3" text="string"/>
        //!   <string language="char3" text="string"/>
        //!   ...
        //! </XXX>
        //! @endcode
        //!
        xml::Element* toXML(DuckContext& duck, xml::Element* parent, const UString& name, bool ignore_empty) const;

        //!
        //! Decode an XML structure and assign the result to this instance.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] elem XML node to decode as an ATSC multiple_string_structure.
        //! @return True on success, false on invalid XLM structure.
        //! @see toXML()
        //!
        bool fromXML(DuckContext& duck, const xml::Element* elem);

        //!
        //! Decode an XML structure and assign the result to this instance.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] parent Parent XML node.
        //! @param [in] name Name of the child XML node to decode as an ATSC multiple_string_structure.
        //! @param [in] required When true, the @a name node shall be present. When false, if the
        //! node if not present, simply clear this object.
        //! @return True on success, false on invalid XLM structure.
        //! @see toXML()
        //!
        bool fromXML(DuckContext& duck, const xml::Element* parent, const UString& name, bool required);

        //!
        //! Serialize a binary multiple_string_structure.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] data Address of the buffer where to serialize the structure.
        //! On return, it is updated to point after the structure.
        //! @param [in,out] size Size in bytes of the buffer.
        //! On return, it is updated to the remaining size in the buffer.
        //! @param [in] max_size Max size to serialize, possibly lower than the buffer size.
        //! @param [in] ignore_empty If true and the multiple_string_structure is empty, do nothing.
        //! @return The number of serialized bytes.
        //!
        size_t serialize(DuckContext& duck, uint8_t*& data, size_t& size, size_t max_size = NPOS, bool ignore_empty = false) const;

        //!
        //! Serialize a binary multiple_string_structure and append to a byte block.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] data Byte block where to serialize the structure. The structure is added at the end.
        //! @param [in] max_size Max size to serialize.
        //! @param [in] ignore_empty If true and the multiple_string_structure is empty, do nothing.
        //! @return The number of serialized bytes.
        //!
        size_t serialize(DuckContext& duck, ByteBlock& data, size_t max_size = NPOS, bool ignore_empty = false) const;

        //!
        //! Serialize a binary multiple_string_structure with a leading length field.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] data Address of the buffer where to serialize the structure.
        //! On return, it is updated to point after the structure.
        //! @param [in,out] size Size in bytes of the buffer.
        //! On return, it is updated to the remaining size in the buffer.
        //! @param [in] length_bytes Size in bytes of the leading length field (1 byte by default).
        //! @return The number of serialized bytes.
        //!
        size_t lengthSerialize(DuckContext& duck, uint8_t*& data, size_t& size, size_t length_bytes = 1) const;

        //!
        //! Serialize a binary multiple_string_structure and append to a byte block with a leading length field.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] data Byte block where to serialize the structure. The structure is added at the end.
        //! @param [in] length_bytes Size in bytes of the leading length field (1 byte by default).
        //! @return The number of serialized bytes.
        //!
        size_t lengthSerialize(DuckContext& duck, ByteBlock& data, size_t length_bytes = 1) const;

        //!
        //! Deserialize a binary multiple_string_structure.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] buffer Address of the structure to deserialize.
        //! On return, it is updated to point after the structure.
        //! @param [in,out] buffer_size Size in bytes of the data buffer.
        //! On return, it is updated to the remaining size in the buffer.
        //! @param [in] mss_size Size of the multiple_string_structure to deserialize,
        //! possibly lower than the buffer size. If lower than @a buffer_size, adjust
        //! @a data and @a buffer_size to skip @a mss_size bytes.
        //! @param [in] ignore_empty If true and the size is zero, then this is a valid empty multiple_string_structure.
        //! @return True if the structure was successfully deserialized.
        //!
        bool deserialize(DuckContext& duck, const uint8_t*& buffer, size_t& buffer_size, size_t mss_size = NPOS, bool ignore_empty = false);

        //!
        //! Deserialize a binary multiple_string_structure with a leading length field.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] buffer Address of the structure to deserialize.
        //! On return, it is updated to point after the structure.
        //! @param [in,out] buffer_size Size in bytes of the data buffer.
        //! On return, it is updated to the remaining size in the buffer.
        //! @param [in] length_bytes Size in bytes of the leading length field (1 byte by default).
        //! @return True if the structure was successfully deserialized.
        //!
        bool lengthDeserialize(DuckContext& duck, const uint8_t*& buffer, size_t& buffer_size, size_t length_bytes = 1);

        //!
        //! A static method to display a binary multiple_string_structure.
        //! @param [in,out] display Display engine.
        //! @param [in] title Leading title to display. Can be empty.
        //! @param [in] margin Left margin content.
        //! @param [in,out] buffer Address of the binary structure to display.
        //! On return, it is updated to point after the structure.
        //! @param [in,out] buffer_size Size in bytes of the data buffer.
        //! On return, it is updated to the remaining size in the buffer.
        //! @param [in] mss_size Size of the multiple_string_structure to deserialize,
        //! possibly lower than the buffer size. If lower than @a buffer_size, adjust
        //! @a data and @a buffer_size to skip @a mss_size bytes.
        //!
        static void Display(TablesDisplay& display, const UString& title, const UString& margin, const uint8_t*& buffer, size_t& buffer_size, size_t mss_size = NPOS);

    private:
        class StringElement
        {
        public:
            UString language;
            UString text;
            StringElement(const UString& lang = UString(), const UString& txt = UString());
        };

        // Private fields:
        std::vector<StringElement> _strings {};

        // The encoding mode for UTF-16:
        static const uint8_t MODE_UTF16 = 0x3F;

        // Set of encoding modes which directly encode Unicode points.
        // Encoding mode 0xNN encodes Unicode range 0xNN00 to 0xNNFF.
        static const std::set<uint8_t> _unicode_modes;

        // Get the encoding mode for a string.
        // One of the Unicode modes if all characters in the string are in the same 255-code range.
        // Otherwise return MODE_UTF16.
        static uint8_t EncodingMode(const UString& text);

        // Decode a string element.
        // When display is true, replace error strings with a message.
        static bool DecodeString(StringElement& elem, const uint8_t*& data, size_t& size, size_t& max_size, bool display);

        // Decode a segment and append to a string.
        // When display is true, replace error strings with a message.
        static bool DecodeSegment(UString& segment, const uint8_t*& data, size_t& size, size_t& max_size, bool display);
    };
}
