//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        class TSDUCKDLL Field
        {
        public:
            Field() = default;             //!< Constructor.
            bool    field_parity = false;  //!< True for first (odd) field of a frame, false for second (even) field.
            uint8_t line_offset = 0;       //!< Line offset, 5 bits.
        };

        //!
        //! List of field entries.
        //!
        typedef std::list<Field> FieldList;

        //!
        //! A service entry.
        //!
        class TSDUCKDLL Service
        {
        public:
            uint8_t   data_service_id = 0;  //!< VBI service type.
            FieldList fields {};            //!< List of fields.
            ByteBlock reserved {};          //!< Reserved bytes, when data_service_id is not any of 1, 2, 4, 5, 6, 7.
            //!
            //! Default constructor.
            //! @param [in] id VBI service type.
            //!
            Service(uint8_t id = 0) : data_service_id(id) {}
            //!
            //! Check if a service entry has reserved bytes.
            //! @return True if @a reserved is used. Return false if the list of fields are used.
            //!
            bool hasReservedBytes() const { return EntryHasReservedBytes(data_service_id); }
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Service> ServiceList;

        // Public members
        ServiceList services {};  //!< The list of service entries in the descriptor.

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
