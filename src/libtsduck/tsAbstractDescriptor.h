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
//!  Abstract base class for MPEG PSI/SI descriptors
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDescriptor.h"
#include "tsTablesPtr.h"
#include "tsTablesDisplay.h"
#include "tsMPEG.h"
#include "tsXML.h"

namespace ts {

    class Descriptor;
    class DescriptorList;
    class TablesDisplay;
    class DVBCharset;

    //!
    //! Abstract base class for MPEG PSI/SI descriptors.
    //!
    class TSDUCKDLL AbstractDescriptor
    {
    public:
        //!
        //! Check if the descriptor is valid.
        //! @return True if the descriptor object is valid, false otherwise.
        //!
        bool isValid() const {return _is_valid;}

        //!
        //! Invalidate the descriptor.
        //! The object must be rebuilt.
        //!
        void invalidate() {_is_valid = false;}

        //!
        //! Get the descriptor tag.
        //! @return The descriptor tag.
        //!
        DID tag() const {return _tag;}

        //!
        //! Get the required private data specifier.
        //! @return The private data specifier which is required to interpret correctly
        //! this descriptor in a section. Return zero if this descriptor is a DVB-defined
        //! or MPEG-defined descriptor, not a private specifier.
        //!
        PDS requiredPDS() const {return _required_pds;}

        //!
        //! Check if this descriptor is a private descriptor.
        //! @return True if this descriptor is a private descriptor,
        //! false if it is a DVB-defined or MPEG-defined descriptor.
        //!
        bool isPrivateDescriptor() const {return _required_pds != 0;}

        //!
        //! Get the XMl node name representing this descriptor.
        //! @return The XML node name.
        //!
        std::string xmlName() const {return _xml_name == 0 ? std::string() : std::string(_xml_name);}

        //!
        //! This abstract method serializes a descriptor.
        //! @param [out] bin A binary descriptor object.
        //! Its content is replaced with a binary representation of this object.
        //! @param [in] charset If not zero, default character set to use.
        //!
        virtual void serialize(Descriptor& bin, const DVBCharset* charset = 0) const = 0;

        //!
        //! This abstract method deserializes a binary descriptor.
        //! In case of success, this object is replaced with the interpreted content of @a bin.
        //! In case of error, this object is invalidated.
        //! @param [in] bin A binary descriptor to interpret according to the descriptor subclass.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //!
        virtual void deserialize(const Descriptor& bin, const DVBCharset* charset = 0) = 0;

        //!
        //! Deserialize a descriptor from a descriptor list.
        //! In case of success, this object is replaced with the interpreted content of the binary descriptor.
        //! In case of error, this object is invalidated.
        //! @param [in] dlist A list of binary descriptors.
        //! @param [in] index Index of the descriptor to deserialize in @a dlist.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //!
        void deserialize(const DescriptorList& dlist, size_t index, const DVBCharset* charset = 0);

        //!
        //! This abstract method converts the descriptor to XML.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @return The new XML element.
        //!
        virtual XML::Element* toXML(XML& xml, XML::Element* parent) const = 0;

        //!
        //! This abstract converts an XML structure to a descriptor.
        //! In case of success, this object is replaced with the interpreted content of the XML structure.
        //! In case of error, this object is invalidated.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] element XML element to convert.
        //!
        virtual void fromXML(XML& xml, const XML::Element* element) = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractDescriptor () {}

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        AbstractDescriptor(const AbstractDescriptor& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        AbstractDescriptor& operator=(const AbstractDescriptor& other);

    protected:
        //!
        //! The descriptor tag can be modified by subclasses only
        //!
        DID _tag;

        //!
        //! XML descriptor name.
        //!
        const char* _xml_name;

        //!
        //! It is the responsibility of the subclasses to set the valid flag
        //!
        bool _is_valid;

        //!
        //! Required private data specified.
        //!
        PDS _required_pds;

        //!
        //! Protected constructor for subclasses.
        //! @param [in] tag Descriptor tag.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] pds Required private data specifier if this is a private descriptor.
        //!
        AbstractDescriptor(DID tag, const char* xml_name, PDS pds = 0);

        //!
        //! Check that an XML element has the right name for this descriptor.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] element XML element to check.
        //! @return True on success, false on error.
        //!
        bool checkXMLName(XML& xml, const XML::Element* element) const;

    private:
        // Unreachable constructors and operators.
        AbstractDescriptor() = delete;
    };
}
