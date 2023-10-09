//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a transport_protocol_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of a transport_protocol_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 101 812, 10.8.1.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TransportProtocolDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Selector byte layout when protocol_id == MHP_PROTO_CAROUSEL.
        //!
        struct Carousel
        {
            Carousel();                              //!< Default constructor.
            void clear();                            //!< Reset content.
            Variable<uint16_t> original_network_id;  //!< Optional original network id.
            Variable<uint16_t> transport_stream_id;  //!< Optional transport stream id.
            Variable<uint16_t> service_id;           //!< Optional service id.
            uint8_t            component_tag;        //!< Component tag.
        };

        //!
        //! Selector byte layout when protocol_id == MHP_PROTO_MPE.
        //!
        struct MPE
        {
            MPE();                                   //!< Default constructor.
            void clear();                            //!< Reset content.
            Variable<uint16_t> original_network_id;  //!< Optional original network id.
            Variable<uint16_t> transport_stream_id;  //!< Optional transport stream id.
            Variable<uint16_t> service_id;           //!< Optional service id.
            bool               alignment_indicator;  //!< Alignment indicator.
            UStringList        urls;                 //!< List of URL's.
        };

        //!
        //! One entry in selector bytes when protocol_id == MHP_PROTO_HTTP.
        //!
        struct HTTPEntry
        {
            HTTPEntry();                             //!< Default constructor.
            UString URL_base;                        //!< URL base.
            UStringList URL_extensions;              //!< List of URL extensions.
        };

        //!
        //! Selector byte layout when protocol_id == MHP_PROTO_HTTP.
        //!
        typedef std::list<HTTPEntry> HTTP;

        // TransportProtocolDescriptor public members:
        uint16_t  protocol_id;               //!< Transport protocol id, one of MHP_PROTO_* values.
        uint8_t   transport_protocol_label;  //!< Transport protocol label.
        Carousel  carousel;                  //!< Selector when protocol_id == MHP_PROTO_CAROUSEL.
        MPE       mpe;                       //!< Selector when protocol_id == MHP_PROTO_MPE.
        HTTP      http;                      //!< Selector when protocol_id == MHP_PROTO_HTTP.
        ByteBlock selector;                  //!< Selector for other protocol ids.

        //!
        //! Default constructor.
        //!
        TransportProtocolDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TransportProtocolDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! When the protocol id is a known one, try to transfer the selector bytes into the appropriate structure.
        //! @param [in,out] duck TSDuck execution context.
        //! @return True on success, false on invalid selector bytes.
        //!
        bool transferSelectorBytes(DuckContext& duck);

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
