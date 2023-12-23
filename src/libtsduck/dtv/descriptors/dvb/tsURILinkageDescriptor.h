//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a URI_linkage_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsUString.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a URI_linkage_descriptor.
    //! @see ETSI EN 300 468, 6.4.15.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL URILinkageDescriptor : public AbstractDescriptor
    {
    public:
        class TSDUCKDLL DVB_I_Info
        {
        //!
        //! DVB-I_info() structure conveyed in private_data when uri_linkage_type = 0x03
        //
        TS_DEFAULT_COPY_MOVE(DVB_I_Info);
        public:
             // DVB_I_Info public members:
            uint8_t                end_point_type = 0;              //!< type of list signalled by the URI
            std::optional<UString> service_list_name {};            //!< name of the service list referenced by the uri
            std::optional<UString> service_list_provider_name {};   //!< name of the provider of the service list referenced by the uri

            //!
            //! Default constructor.
            //!
            DVB_I_Info();
            //!
            //! Constructor from binary descriptor data.
            //! @param [in] buf A binary descriptor to deserialize.
            //!
            DVB_I_Info(PSIBuffer& buf) : DVB_I_Info() { deserialize(buf); }

            //! @cond nodoxygen
            void clearContent(void);
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };
        //
        // URILinkageDescriptor public members:
        uint8_t                   uri_linkage_type = 0;     //!< URI linkage type.
        UString                   uri {};                   //!< The URI.
        uint16_t                  min_polling_interval = 0; //!< Valid when uri_linkage_type == 0x00 or 0x01.
        std::optional<DVB_I_Info> dvb_i_private_data {};    //!< Valid when uri_linkage_type == 0x03.
        ByteBlock                 private_data {};          //!< Private data.

        //!
        //! Default constructor.
        //!
        URILinkageDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        URILinkageDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
