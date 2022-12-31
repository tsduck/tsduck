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
//!
//!  @file
//!  Representation of a VBI_data_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a VBI_data_descriptor.
    //! @see ETSI EN 300 468, 6.2.47.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL VBIDataDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! A field entry.
        //!
        struct TSDUCKDLL Field
        {
            // Public members
            bool    field_parity;  //!< True for first (odd) field of a frame, false for second (even) field.
            uint8_t line_offset;   //!< Line offset, 5 bits.

            //!
            //! Default constructor.
            //! @param [in] parity True for first (odd) field of a frame, false for second (even) field.
            //! @param [in] offset Line offset.
            //!
            Field(bool parity = false, uint8_t offset = 0);
        };

        //!
        //! List of field entries.
        //!
        typedef std::list<Field> FieldList;

        //!
        //! A service entry.
        //!
        struct TSDUCKDLL Service
        {
            // Public members
            uint8_t   data_service_id;  //!< VBI service type.
            FieldList fields;           //!< List of fields.
            ByteBlock reserved;         //!< Reserved bytes, when data_service_id is not any of 1, 2, 4, 5, 6, 7.

            //!
            //! Default constructor.
            //! @param [in] id VBI service type.
            //!
            Service(uint8_t id = 0);

            //!
            //! Check if a service entry has reserved bytes.
            //! @return True if @a reserved is used. Return false if the list of fields are used.
            //!
            bool hasReservedBytes() const
            {
                return EntryHasReservedBytes(data_service_id);
            }
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Service> ServiceList;

        // Public members
        ServiceList services;  //!< The list of service entries in the descriptor.

        //!
        //! Default constructor.
        //!
        VBIDataDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        VBIDataDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Check if an entry has reserved bytes.
        static bool EntryHasReservedBytes(uint8_t data_service_id);
    };
}
