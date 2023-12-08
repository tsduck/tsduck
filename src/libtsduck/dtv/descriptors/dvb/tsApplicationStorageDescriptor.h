//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an application_storage_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an application_storage_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 809, 5.3.10.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ApplicationStorageDescriptor : public AbstractDescriptor
    {
    public:
        // ApplicationStorageDescriptor public members:
        uint8_t  storage_property = 0;                      //!< Storage property.
        bool     not_launchable_from_broadcast = false;     //!< Not launchable from broadcast.
        bool     launchable_completely_from_cache = false;  //!< Launchable completely from cache.
        bool     is_launchable_with_older_version = false;  //!< Is launchable with older version.
        uint32_t version = 0;                               //!< 31 bits, application version.
        uint8_t  priority = 0;                              //!< Application priority.

        //!
        //! Default constructor.
        //!
        ApplicationStorageDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ApplicationStorageDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
