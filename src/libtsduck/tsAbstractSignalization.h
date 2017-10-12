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
//!
//!  @file
//!  Abstract base class for MPEG PSI/SI tables and descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsXML.h"

namespace ts {
    //!
    //! Abstract base class for MPEG PSI/SI tables and descriptors.
    //!
    class TSDUCKDLL AbstractSignalization
    {
    public:
        //!
        //! Check if this object is valid.
        //! @return True if this object is valid.
        //!
        bool isValid() const
        {
            return _is_valid;
        }

        //!
        //! Invalidate this object.
        //! This object must be rebuilt.
        //!
        void invalidate()
        {
            _is_valid = false;
        }

        //!
        //! Get the XMl node name representing this table or descriptor.
        //! @return The XML node name.
        //!
        std::string xmlName() const;

        //!
        //! This abstract method converts this object to XML.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @return The new XML element.
        //!
        virtual XML::Element* toXML(XML& xml, XML::Element* parent) const = 0;

        //!
        //! This abstract converts an XML structure to a table or descriptor.
        //! In case of success, this object is replaced with the interpreted content of the XML structure.
        //! In case of error, this object is invalidated.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] element XML element to convert.
        //!
        virtual void fromXML(XML& xml, const XML::Element* element) = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractSignalization() {}

    protected:
        //!
        //! XML table or descriptor name in UTF-8 representation.
        //!
        const char* const _xml_name;

        //!
        //! It is the responsibility of the subclasses to set the valid flag
        //!
        bool _is_valid;

        //!
        //! Protected constructor for subclasses.
        //! @param [in] xml_name Table or descriptor name, as used in XML structures.
        //!
        AbstractSignalization(const char* xml_name);

        //!
        //! Copy constructor.
        //! @param [in] other The other instance to copy.
        //!
        AbstractSignalization(const AbstractSignalization& other);

        //!
        //! Assignment operator.
        //! @param [in] other The other instance to copy.
        //! @return A reference to this object.
        //!
        AbstractSignalization& operator=(const AbstractSignalization& other);

        //!
        //! Check that an XML element has the right name for this table.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] element XML element to check.
        //! @return True on success, false on error.
        //!
        bool checkXMLName(XML& xml, const XML::Element* element) const;

        //!
        //! This static method serializes a DVB string with a required fixed size.
        //! @param [in,out] bb A byte-block where @a str will be appended if its size is correct.
        //! @param [in] str String to serialize.
        //! @param [in] size Required size in bytes of the serialized string.
        //! @param [in] charset If not zero, default character set to use.
        //! @return True if the size has the required length and has been serialized.
        //!
        static bool SerializeFixedLength(ByteBlock& bb, const UString& str, const size_t size, const DVBCharset* charset = 0);

        //!
        //! This abstract method serializes a 3-byte language or country code.
        //! @param [in,out] bb A byte-block where @a str will be appended if its size is correct.
        //! @param [in] str String to serialize.
        //! @param [in] charset If not zero, default character set to use.
        //! @return True if the size has the required length and has been serialized.
        //!
        static bool SerializeLanguageCode(ByteBlock& bb, const UString& str, const DVBCharset* charset = 0)
        {
            return SerializeFixedLength(bb, str, 3, charset);
        }

    private:
        // Unreachable constructors and operators.
        AbstractSignalization() = delete;
    };
}
