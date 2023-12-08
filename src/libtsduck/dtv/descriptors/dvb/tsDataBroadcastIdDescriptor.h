//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a generic data_broadcast_id_descriptor.
//!  Specialized classes exist, depending on the data_broadcast_id.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a generic data_broadcast_id_descriptor.
    //! Specialized classes exist, depending on the data_broadcast_id.
    //! @see ETSI EN 300 468, 6.2.12.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DataBroadcastIdDescriptor : public AbstractDescriptor
    {
    public:
        // DataBroadcastIdDescriptor public members:
        uint16_t  data_broadcast_id = 0; //!< Data broadcast id.
        ByteBlock private_data {};       //!< Id selector bytes.

        //!
        //! Default constructor.
        //! @param [in] id Data broadcast id.
        //!
        DataBroadcastIdDescriptor(uint16_t id = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DataBroadcastIdDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Static method to display a data broadcast selector bytes.
        //! @param [in,out] display Display engine.
        //! @param [in] buf Buffer from which the selector bytes are read.
        //! @param [in] margin Left margin content.
        //! @param [in] dbid Data broadcast id.
        //!
        static void DisplaySelectorBytes(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint16_t dbid);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

        // These specific cases of data_broadcast_id_descriptor reuse buildXML().
        friend class SSUDataBroadcastIdDescriptor;

    private:
        // Display selector bytes of various types.
        // Fields data and size are updated.
        static void DisplaySelectorSSU(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint16_t dbid);
        static void DisplaySelectorINT(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint16_t dbid);
        static void DisplaySelectorMPE(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint16_t dbid);
        static void DisplaySelectorGeneric(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint16_t dbid);
    };
}
