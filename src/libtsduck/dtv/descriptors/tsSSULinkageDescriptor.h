//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a linkage_descriptor for SSU.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsLinkageDescriptor.h"

namespace ts {
    //!
    //! Representation of a linkage_descriptor for system software update.
    //! SSU uses linkage type 0x09.
    //! @see ETSI EN 300 468, 6.2.19.
    //! @see ETSI TS 102 006, 6.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SSULinkageDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! OUI entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint32_t  oui = 0;      //!< OUI, 24 bits.
            ByteBlock selector {};  //!< Selector bytes.

            //!
            //! Constructor.
            //! @param [in] oui_ OUI, 24 bits.
            //!
            Entry(uint32_t oui_ = 0);
        };

        //!
        //! List of OUI entries.
        //!
        typedef std::list<Entry> EntryList;

        // SSULinkageDescriptor public members:
        uint16_t  ts_id = 0;        //!< Transport stream id.
        uint16_t  onetw_id = 0;     //!< Original network id.
        uint16_t  service_id = 0;   //!< Service id.
        EntryList entries {};       //!< The list of OUI entries.
        ByteBlock private_data {};  //!< Private data.

        //!
        //! Default constructor.
        //! @param [in] ts Transport stream id.
        //! @param [in] onetw Original network id
        //! @param [in] service Service id.
        //!
        SSULinkageDescriptor(uint16_t ts = 0, uint16_t onetw = 0, uint16_t service = 0);

        //!
        //! Constructor with one OUI.
        //! @param [in] ts Transport stream id.
        //! @param [in] onetw Original network id
        //! @param [in] service Service id.
        //! @param [in] oui OUI, 24 bits.
        //!
        SSULinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service, uint32_t oui);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SSULinkageDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Constructor from a linkage_descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] desc A linkage_descriptor to convert.
        //! The data_broadcast_id must be 0x000A.
        //!
        SSULinkageDescriptor(DuckContext& duck, const LinkageDescriptor& desc);

        //!
        //! Convert to a linkage_descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] desc A linkage_descriptor to convert.
        //!
        void toLinkageDescriptor(DuckContext& duck, LinkageDescriptor& desc) const;

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
