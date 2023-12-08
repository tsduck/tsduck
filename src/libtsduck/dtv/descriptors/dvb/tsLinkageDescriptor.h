//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a generic linkage_descriptor.
//!  Specialized classes exist, depending on the linkage type.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Linkage type values (in linkage_descriptor)
    //!
    enum : uint8_t {
        LINKAGE_INFO            = 0x01, //!< Information service
        LINKAGE_EPG             = 0x02, //!< EPG service
        LINKAGE_CA_REPLACE      = 0x03, //!< CA replacement service
        LINKAGE_TS_NIT_BAT      = 0x04, //!< TS containing complet network/bouquet SI
        LINKAGE_SERVICE_REPLACE = 0x05, //!< Service replacement service
        LINKAGE_DATA_BROADCAST  = 0x06, //!< Data broadcast service
        LINKAGE_RCS_MAP         = 0x07, //!< RCS map
        LINKAGE_HAND_OVER       = 0x08, //!< Mobile hand-over
        LINKAGE_SSU             = 0x09, //!< System software update service
        LINKAGE_SSU_TABLE       = 0x0A, //!< TS containing SSU BAT or NIT
        LINKAGE_IP_NOTIFY       = 0x0B, //!< IP/MAC notification service
        LINKAGE_INT_BAT_NIT     = 0x0C, //!< TS containing INT BAT or NIT
        LINKAGE_EVENT           = 0x0D, //!< Event linkage
        LINKAGE_EXT_EVENT_MIN   = 0x0E, //!< Extented event linkage, first value
        LINKAGE_EXT_EVENT_MAX   = 0x1F, //!< Extented event linkage, last value
    };

    //!
    //! Representation of a generic linkage_descriptor.
    //! Specialized classes exist, depending on the linkage type.
    //! @see ETSI EN 300 468, 6.2.19.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL LinkageDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Definition of mobile_hand-over_info when linkage_type == LINKAGE_HAND_OVER
        //!
        class TSDUCKDLL MobileHandoverInfo
        {
        public:
            MobileHandoverInfo() = default;   //!< Default constructor.
            void clear();                     //!< Clear object content.

            uint8_t  handover_type = 0;       //!< Hand-over type, 4 bits.
            uint8_t  origin_type = 0;         //!< Origin type, 0 = NIT, 1 = SDT.
            uint16_t network_id = 0;          //!< Network when handover_type == 0x01, 0x02, 0x03.
            uint16_t initial_service_id = 0;  //!< Initial service when origin_type == 0x00
        };

        //!
        //! Definition of event_linkage_info when linkage_type == LINKAGE_EVENT
        //!
        class TSDUCKDLL EventLinkageInfo
        {
        public:
            EventLinkageInfo() = default;      //!< Default constructor.
            void clear();                      //!< Clear object content.

            uint16_t target_event_id = 0;      //!< Target event.
            bool     target_listed = false;    //!< Service is listed in SDT.
            bool     event_simulcast = false;  //!< Target and source event are simulcast.
        };

        //!
        //! Definition of extended_event_linkage_info when linkage_type in LINKAGE_EXT_EVENT_MIN .. LINKAGE_EXT_EVENT_MAX
        //!
        class TSDUCKDLL ExtendedEventLinkageInfo
        {
        public:
            ExtendedEventLinkageInfo() = default;      //!< Default constructor.
            void clear();                              //!< Clear object content.

            uint16_t  target_event_id = 0;             //!< Target event.
            bool      target_listed = false;           //!< Service is listed in SDT.
            bool      event_simulcast = false;         //!< Target and source event are simulcast.
            uint8_t   link_type = 0;                   //!< Link type, 2 bits.
            uint8_t   target_id_type = 0;              //!< Target type, 2 bits.
            uint16_t  user_defined_id = 0;             //!< User-defined id when target_id_type == 3
            uint16_t  target_transport_stream_id = 0;  //!< Target TS when target_id_type == 1
            std::optional<uint16_t> target_original_network_id {};   //!< Optional target original network.
            std::optional<uint16_t> target_service_id {};            //!< Optional target service.
        };

        //!
        //! List of extended event info.
        //!
        typedef std::list<ExtendedEventLinkageInfo> ExtendedEventLinkageList;

        // LinkageDescriptor public members:
        uint16_t                 ts_id = 0;                       //!< Transport stream id.
        uint16_t                 onetw_id = 0;                    //!< Original network id.
        uint16_t                 service_id = 0;                  //!< Service id.
        uint8_t                  linkage_type = 0;                //!< Linkage type, LINKAGE_* constants, eg ts::LINKAGE_INFO.
        MobileHandoverInfo       mobile_handover_info {};         //!< mobile_hand-over_info when linkage_type == LINKAGE_HAND_OVER.
        EventLinkageInfo         event_linkage_info {};           //!< event_linkage_info when linkage_type == LINKAGE_EVENT.
        ExtendedEventLinkageList extended_event_linkage_info {};  //!< extended_event_linkage_info when linkage_type in LINKAGE_EXT_EVENT_MIN .. LINKAGE_EXT_EVENT_MAX.
        ByteBlock                private_data {};                 //!< Private data, depends on linkage type.

        //!
        //! Default constructor
        //! @param [in] ts Transport stream id.
        //! @param [in] onetw Original network id.
        //! @param [in] service Service id.
        //! @param [in] ltype Linkage type.
        //!
        LinkageDescriptor(uint16_t ts = 0, uint16_t onetw = 0, uint16_t service = 0, uint8_t ltype = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        LinkageDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

        // The specific cases of linkage_descriptor reuse buildXML().
        friend class SSULinkageDescriptor;

    private:
        // Display linkage private data of various types.
        // Fields data and size are updated.
        static void DisplayPrivateMobileHandover(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
        static void DisplayPrivateSSU(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
        static void DisplayPrivateTableSSU(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
        static void DisplayPrivateINT(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
        static void DisplayPrivateDeferredINT(TablesDisplay& display, PSIBuffer& buf, const UString& margin, uint8_t ltype);
    };
}
