//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB CA_startup_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsTS.h"

namespace ts {

    //!
    //! Representation of an ISDB CA_startup_descriptor.
    //! @see ARIB STD-B61, Volume 2, 4.6.1
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL ISDBCAStartupDescriptor : public AbstractDescriptor
    {
    public:
        // ISDBCAStartupDescriptor public members:
        uint16_t               CA_system_ID = 0;            //!< Conditional access system identifier.
        PID                    CA_program_ID = PID_NULL;    //!< CAS program identifier.
        uint8_t                load_indicator = 0;          //!< 7 bits.
        std::optional<PID>     second_CA_program_ID {};     //!< CAS program identifier.
        std::optional<uint8_t> second_load_indicator {};    //!< Optional second conditional access system identifier.
        std::vector<PID>       exclusion_CA_program_ID {};  //!< 7 bits.
        ByteBlock              load_security_info {};       //!< Security information.
        ByteBlock              private_data {};             //!< Private data.

        //!
        //! Default constructor.
        //!
        ISDBCAStartupDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBCAStartupDescriptor(DuckContext& duck, const Descriptor& bin);

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
