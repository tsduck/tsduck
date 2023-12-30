//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard, Paul Higgs
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
    //! uri_linkage_type in URI_linkage_descriptor
    //! @see ETSI TS 101 162
    //!
    enum {
        URI_LINKAGE_ONLINE_SDT = 0x00,  //!< Online SDT (OSDT) for CI Plus, ETSI TS 102 606-2
        URI_LINKAGE_IPTV_SDnS  = 0x01,  //!< DVB-IPTV SD&S, ETSI TS 102 034
        URI_LINKAGE_MRS        = 0x02,  //!< Material Resolution Server (MRS) for companion screen applications, CENELEC EN 50221
        URI_LINKAGE_DVB_I      = 0x03,  //!< DVB-I, DVB Bluebook A177, ETSI TS 193 770
    };

    //!
    //! end_point_type in DVB-I_Info() in URI_linkage_descriptor
    //!
    enum {
        END_POINT_SERVICE_LIST          = 0x01,           //!< URI is a service list document
        END_POINT_SERVICE_LIST_REGISTRY = 0x02,           //!< URI is a servic list registry query
        END_POINT_SERVICE_LIST_EXTENDED = 0x03,           //!< URI us a service list document with additional information

        END_POINT_MIN = END_POINT_SERVICE_LIST,           //!< First assigned value for end_point_type
        END_POINT_MAX = END_POINT_SERVICE_LIST_EXTENDED,  //!< Last assigned value for end_point_type
    };

    //!
    //! Representation of a URI_linkage_descriptor.
    //! @see ETSI EN 300 468, 6.4.15.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL URILinkageDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! DVB-I_info() structure conveyed in private_data when uri_linkage_type = 0x03
        //!
        class TSDUCKDLL DVB_I_Info
        {
            TS_DEFAULT_COPY_MOVE(DVB_I_Info);
        public:
            // DVB_I_Info public members:
            uint8_t                end_point_type = 0;              //!< type of list signalled by the URI
            std::optional<UString> service_list_name {};            //!< name of the service list referenced by the uri
            std::optional<UString> service_list_provider_name {};   //!< name of the provider of the service list referenced by the uri

            //!
            //! Default constructor.
            //!
            DVB_I_Info() = default;
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
