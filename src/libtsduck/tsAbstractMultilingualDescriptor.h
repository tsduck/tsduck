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
//!  Abstract base class for DVB descriptors with a multilingual name.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTunerParameters.h"
#include "tsMPEG.h"

namespace ts {
    //!
    //! Abstract base class for DVB descriptors with a multilingual name.
    //! Subclasses may have a "prolog" between the descriptor header and
    //! the multilingual names loop.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AbstractMultilingualDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Language entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            UString language;  //!< ISO-639 language code, 3 characters.
            UString name;      //!< Name in this language.

            //!
            //! Default constructor.
            //! @param [in] lang ISO-639 language code, 3 characters.
            //! @param [in] name Name for this language.
            //!
            Entry(const UString& lang = UString(), const UString& name = UString());
        };

        //!
        //! List of language entries.
        //!
        typedef std::list<Entry> EntryList;

        // Multiligual descriptor public members:
        EntryList entries;  //!< List of language entries.

        // Inherited methods
        virtual void serialize(Descriptor&, const DVBCharset* = nullptr) const override;
        virtual void deserialize(const Descriptor&, const DVBCharset* = nullptr) override;
        virtual void buildXML(xml::Element*) const override;
        virtual void fromXML(const xml::Element*) override;
        DeclareDisplayDescriptor();

    protected:
        //!
        //! Protected constructor for subclasses.
        //! @param [in] tag Descriptor tag.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] xml_attribute XML attribute name for the "name" fields.
        //! @param [in] pds Required private data specifier if this is a private descriptor.
        //!
        AbstractMultilingualDescriptor(DID tag, const UChar* xml_name, const UChar* xml_attribute, PDS pds = 0);

        //!
        //! The subclass serializes the "prolog".
        //! The prolog is between the descriptor header and the multilingual names loop.
        //! @param [in,out] bbp Serialize the prolog in this byte block.
        //! @param [in] charset If not zero, default character set to use.
        //!
        virtual void serializeProlog(const ByteBlockPtr& bbp, const DVBCharset* charset) const {}

        //!
        //! The subclass deserializes the "prolog".
        //! The prolog is between the descriptor header and the multilingual names loop.
        //! The subclass shall set _is_valid to false on error.
        //! @param [in,out] data Address of prolog. Updated after deserialization.
        //! @param [in,out] size Remaining descriptor size, including prolog. Updated after deserialization.
        //! @param [in] charset If not zero, default character set to use.
        //!
        virtual void deserializeProlog(const uint8_t*& data, size_t& size, const DVBCharset* charset) {}

    private:
        const UChar* _xml_attribute;

        // Use default assignment but declare it to make sure the compiler knows
        // that we have understood the consequences of a pointer member.
        AbstractMultilingualDescriptor(const AbstractMultilingualDescriptor&) = default;
        AbstractMultilingualDescriptor& operator=(const AbstractMultilingualDescriptor&) = default;

        // Unreachable constructors and operators.
        AbstractMultilingualDescriptor() = delete;
    };
}
