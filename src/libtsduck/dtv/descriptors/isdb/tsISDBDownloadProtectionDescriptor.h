//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB download_protection_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsTS.h"

namespace ts {

    //!
    //! Representation of an ISDB download_protection_descriptor.
    //! @see ARIB STD-B61, Volume 2, 4.4.7.1
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL ISDBDownloadProtectionDescriptor : public AbstractDescriptor
    {
    public:
        // ISDBDownloadProtectionDescriptor public members:
        uint8_t   DL_system_ID = 0;             //!< Download protection system identifier.
        PID       DL_program_ID = PID_NULL;     //!< PID of TS packets which transmit associated information.
        uint8_t   encrypt_protocol_number = 0;  //!< Encryption algorithm of secure transmission and associated information.
        ByteBlock encrypt_info {};              //!< Initialization Vector (IV) of block cipher mode used for secure transmission or associated information.

        //!
        //! Default constructor.
        //!
        ISDBDownloadProtectionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBDownloadProtectionDescriptor(DuckContext& duck, const Descriptor& bin);

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
