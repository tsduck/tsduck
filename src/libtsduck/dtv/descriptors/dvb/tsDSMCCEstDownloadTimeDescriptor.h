//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a est_download_time_descriptor (DSM-CC U-N Message DSI/DII specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a est_download_time_descriptor (DSM-CC U-N Message DSI specific).
    //! This descriptor cannot be present in other tables than a DSI (0x3B)
    //!
    //! @see ETSI EN 301 192 V1.7.1 (2021-08), 10.2.8
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DSMCCEstDownloadTimeDescriptor: public AbstractDescriptor {
    public:
        // DSMCCEstDownloadTimeDescriptor public members:
        uint32_t est_download_time = 0;  //!< Estimated download time of data in seconds.

        //!
        //! Default constructor.
        //!
        DSMCCEstDownloadTimeDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DSMCCEstDownloadTimeDescriptor(DuckContext& duck, const Descriptor& bin);

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
}  // namespace ts
