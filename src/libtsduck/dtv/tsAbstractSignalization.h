//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  Abstract base class for MPEG PSI/SI tables and descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDefinedByStandards.h"
#include "tsxml.h"

namespace ts {

    class DuckContext;
    class ByteBlock;

    //!
    //! Abstract base class for MPEG PSI/SI tables and descriptors.
    //! @ingroup mpeg
    //!
    //! Some methods are declared as "virtual final". Since these methods are not
    //! inherited, this seems useless. This is in fact a compilation check. These
    //! methods were formerly designed to be overridden by subclasses but the
    //! implementation has changed. They are now defined in this class only and
    //! call a new pure virtual method. The "final" attribute is here to detect
    //! old subclasses which do not yet use the new scheme.
    //!
    class TSDUCKDLL AbstractSignalization : public AbstractDefinedByStandards
    {
    public:
        //!
        //! Check if this object is valid.
        //! @return True if this object is valid.
        //!
        bool isValid() const { return _is_valid; }

        //!
        //! Invalidate this object.
        //! This object must be rebuilt.
        //!
        void invalidate() { _is_valid = false; }

        //!
        //! This method clears the content of the table or descriptor.
        //! Upon return, the object is valid and in the same empty state as after a default constructor.
        //!
        virtual void clear();

        //!
        //! Get the XMl node name representing this table or descriptor.
        //! @return The XML node name.
        //!
        UString xmlName() const;

        //!
        //! This method converts this object to XML.
        //!
        //! When this object is valid, this method creates a root node with the default XML
        //! name and then invokes buildXML() in the subclass to populate the XML node.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @return The new XML element.
        //!
        virtual xml::Element* toXML(DuckContext& duck, xml::Element* parent) const final;

        //!
        //! This method converts an XML structure to a table or descriptor in this object.
        //!
        //! In case of success, this object is replaced with the interpreted content of the XML structure.
        //! In case of error, this object is invalidated.
        //!
        //! This method checks the name of the XML node and then invokes analyzeXML() in the subclass.
        //! Depending on the returned values of analyzeXML(), this object is either validated or invalidated.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element XML element to convert.
        //!
        virtual void fromXML(DuckContext& duck, const xml::Element* element) final;

        // Implementation of AbstractDefinedByStandards
        virtual Standards definingStandards() const override;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractSignalization();

        //!
        //! XML tag name for generic descriptors.
        //!
        static const UChar* const XML_GENERIC_DESCRIPTOR;
        //!
        //! XML tag name for generic short sections.
        //!
        static const UChar* const XML_GENERIC_SHORT_TABLE;
        //!
        //! XML tag name for generic tables with long sections.
        //!
        static const UChar* const XML_GENERIC_LONG_TABLE;

        //!
        //! This static method serializes a DVB string with a required fixed size.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] bb A byte-block where @a str will be appended if its size is correct.
        //! @param [in] str String to serialize.
        //! @param [in] size Required size in bytes of the serialized string.
        //! @return True if the size has the required length and has been serialized.
        //!
        static bool SerializeFixedLength(DuckContext& duck, ByteBlock& bb, const UString& str, const size_t size);

        //!
        //! This static method serializes a 3-byte language or country code.
        //! @param [in,out] bb A byte-block where @a str will be appended if its size is correct.
        //! @param [in] str String to serialize.
        //! @param [in] allow_empty If true, an empty string is allowed and serialized as zeroes.
        //! @return True if the size has the required length and has been serialized.
        //!
        static bool SerializeLanguageCode(ByteBlock& bb, const UString& str, bool allow_empty = false);

        //!
        //! This static method deserializes a 3-byte language or country code.
        //! @param [in] data Address of a 3-byte memory area.
        //! @return Deserialized string.
        //!
        static UString DeserializeLanguageCode(const uint8_t* data);

    protected:
        // Implementation node: Try to make _is_valid private some day.
        // It should not be used outside legacy serialize() / deserialize() implementations.

        //!
        //! It is the responsibility of the subclasses to set the valid flag
        //!
        bool _is_valid;

        //!
        //! Protected constructor for subclasses.
        //! @param [in] xml_name Table or descriptor name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractSignalization(const UChar* xml_name, Standards standards, const UChar* xml_legacy_name = nullptr);

        //!
        //! Copy constructor.
        //! Use default implementation, just tell the compiler we understand
        //! the consequences of copying a pointer member.
        //! @param [in] other The other instance to copy.
        //!
        AbstractSignalization(const AbstractSignalization& other) = default;

        //!
        //! Assignment operator.
        //! @param [in] other The other instance to copy.
        //! @return A reference to this object.
        //!
        AbstractSignalization& operator=(const AbstractSignalization& other);

        //!
        //! Helper method to clear the content of the table or descriptor.
        //!
        //! It is called by clear(). In clearContent(), the subclass shall simply
        //! revert the value of all fields to their original values in the default
        //! constructor.
        //!
        virtual void clearContent() = 0;

        //!
        //! Helper method to convert this object to XML.
        //!
        //! It is called by toXML() only when the object is valid. The @a root element
        //! is already built with the appropriate XML node name. In buildXML(), the
        //! subclass shall simply populate the XML node.
        //!
        //! @param [in,out] root The root node for the new XML tree.
        //! @param [in,out] duck TSDuck execution context.
        //!
        virtual void buildXML(DuckContext& duck, xml::Element* root) const = 0;

        //!
        //! Helper method to convert this object from XML.
        //!
        //! It is called by fromXML() after checking the validity of the XML
        //! node name. In analyzeXML(), the subclass shall populate the C++
        //! object from the content of the XML node. If analyzeXML() returns false,
        //! this table or descriptor object is then invalidated and cleared.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] element XML element to convert.
        //! @return True if the analysis is correct, false otherwise.
        //!
        virtual bool analyzeXML(DuckContext& duck, const xml::Element* element) = 0;

        //!
        //! Deserialize a 3-byte language or country code.
        //! @param [out] lang Deserialized language code.
        //! @param [in,out] data Address of memory area. Adjusted to point after the deserialized data.
        //! @param [in,out] size Remaining size in bytes of memory area. Adjusted remove the deserialized data.
        //! @return True on success, false on error. On error, the object is invalidated.
        //!
        bool deserializeLanguageCode(UString& lang, const uint8_t*& data, size_t& size);

        //!
        //! Deserialize an integer.
        //! @tparam INT Some integer type.
        //! @param [out] value Deserialized integer value.
        //! @param [in,out] data Address of memory area. Adjusted to point after the deserialized data.
        //! @param [in,out] size Remaining size in bytes of memory area. Adjusted remove the deserialized data.
        //! @return True on success, false on error. On error, the object is invalidated.
        //!
        template<typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        bool deserializeInt(INT& value, const uint8_t*& data, size_t& size);

        //!
        //! Deserialize a one-bit boolean inside one byte.
        //! @param [out] value Deserialized bool value.
        //! @param [in,out] data Address of memory area. Adjusted to point after the deserialized data (one byte).
        //! @param [in,out] size Remaining size in bytes of memory area. Adjusted remove the deserialized data.
        //! @param [in] bit Bit number of the boolean in the deserialized byte, from 0 (LSB) to 7 (MSB).
        //! @return True on success, false on error. On error, the object is invalidated.
        //!
        bool deserializeBool(bool& value, const uint8_t*& data, size_t& size, size_t bit = 0);

    private:
        const UChar* const _xml_name;         // XML table or descriptor name.
        const UChar* const _xml_legacy_name;  // Optional XML table or descriptor legacy name. Ignored if null pointer.
        const Standards    _standards;        // Defining standards (usually only one).

        // Check that an XML element has the right name for this table or descriptor.
        bool checkXMLName(const xml::Element* element) const;

        // Unreachable constructors and operators.
        AbstractSignalization() = delete;
    };
}

#include "tsAbstractSignalizationTemplate.h"
