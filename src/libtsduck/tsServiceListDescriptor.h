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
//!  Representation of a service_list_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a service_list_descriptor
    //! @see ETSI 300 468, 6.2.35.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ServiceListDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Service entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t service_id;     //!< Service id.
            uint8_t  service_type;   //!< Service type.

            //!
            //! Default constructor.
            //! @param [in] id Service id.
            //! @param [in] type Service type.
            //!
            Entry(uint16_t id = 0, uint8_t type = 0) :
                service_id(id),
                service_type(type)
            {
            }
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static const size_t MAX_ENTRIES = 85;

        // ServiceListDescriptor public members:
        EntryList entries;  //!< The list of service entries.

        //!
        //! Default constructor.
        //!
        ServiceListDescriptor();

        //!
        //! Constructor using a variable-length argument list.
        //! Each entry is described by 2 arguments: @a service_id and @ service_type.
        //! All arguments are int, not uint16_t, since integer literals are int
        //! by default. The end of the argument list must be marked by -1.
        //! @param [in] service_id Service id.
        //! @param [in] service_type Service type.
        //!
        ServiceListDescriptor(int service_id, int service_type, ...);

        //!
        //! Constructor from a binary descriptor
        //! @param [in] bin A binary descriptor to deserialize.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //!
        ServiceListDescriptor(const Descriptor& bin, const DVBCharset* charset = nullptr);

        // Inherited methods
        virtual void serialize(Descriptor&, const DVBCharset* = nullptr) const override;
        virtual void deserialize(const Descriptor&, const DVBCharset* = nullptr) override;
        virtual void buildXML(xml::Element*) const override;
        virtual void fromXML(const xml::Element*) override;
        DeclareDisplayDescriptor();
    };
}
