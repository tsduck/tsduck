//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Service Guide Table (SGT), as defined by SES Astra.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsServiceIdTriplet.h"

namespace ts {
    //!
    //! Representation of a Service Guide Table (SGT), as defined by SES Astra.
    //! @see Astra LCN Technical Specification, 2.1
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL SGT : public AbstractLongTable
    {
    public:
        //!
        //! Description of a service.
        //! Note: by inheriting from EntryWithDescriptors, there is a public field "DescriptorList descs".
        //!
        class TSDUCKDLL Service : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Service);
            TS_DEFAULT_ASSIGMENTS(Service);
        public:
            uint16_t logical_channel_number = 0;   //!< LCN, 14 bits.
            bool     visible_service_flag = true;  //!< Service is visible.
            bool     new_service_flag = false;     //!< Service is new.
            uint16_t genre_code = 0xFFFF;          //!< Genre code, unused as documented in Astra spec.

            //!
            //! Constructor.
            //! @param [in] table Parent table.
            //!
            explicit Service(const AbstractTable* table);
        };

        //!
        //! List of Service's, indexed by ServiceIdTriplet.
        //!
        using ServiceMap = AttachedEntryMap<ServiceIdTriplet, Service>;

        // SGT public members:
        uint16_t       service_list_id = 0xFFFF;  //!< Service list id.
        DescriptorList descs;                     //!< Top-level descriptor list.
        ServiceMap     services;                  //!< Map of service descriptions, key=onid/tsid/srvid, value=service.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //! @param [in] id Service list identifier.
        //!
        SGT(uint8_t vers = 0, bool cur = true, uint16_t id = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        SGT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        SGT(const SGT& other);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        SGT& operator=(const SGT& other) = default;

        // Inherited methods.
        virtual uint16_t tableIdExtension() const override;
        virtual DescriptorList* topLevelDescriptorList() override;
        virtual const DescriptorList* topLevelDescriptorList() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods.
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
