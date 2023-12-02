//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a RAR_over_DVB_stream_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an RAR_over_DVB_stream_descriptor
    //!
    //! This descriptor cannot be present in other tables than a RNT
    //! because its tag reuses a DVB-defined one.
    //!
    //! @see ETSI TS 102 323 clause 5.3.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL RARoverDVBstreamDescriptor: public AbstractDescriptor
    {
    public:

        Time                   first_valid_date {};         //!< The first date when this CRID authority reference can be used.
        Time                   last_valid_date {};          //!< The first date when this CRID authority reference cannot be used.
        uint8_t                weighting = 0;               //!< 6 bits. A hint to the PDR as to the order to try multiple records for a single CRID authority from the same resolution provider.
        bool                   complete_flag = false;       //!< This flag indicates if the referenced CRI data is complete.
        uint16_t               transport_stream_id = 0;     //!< Set to the transport_stream_id of the DVB service in which the referenced CRI is carried.
        uint16_t               original_network_id = 0;     //!< Set to the original_network_id of the DVB service in which the referenced CRI is carried.
        uint16_t               service_id = 0;              //!< Set to the service_id of the DVB service in which the referenced CRI is carried.
        uint8_t                component_tag = 0;           //!< Identifies the elementary stream on which the referenced CRI is carried.
        std::optional<Time>    download_start_time {};      //!< Date and time at which the CRI service will start to be available.
        std::optional<uint8_t> download_period_duration {}; //!< The length of time from the start time during which the CRI service will be available.
        std::optional<uint8_t> download_cycle_time {};      //!< the minimum time required for one complete repetition of all data in the CRI service, measured in minutes.

        //!
        //! Default constructor.
        //!
        RARoverDVBstreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        RARoverDVBstreamDescriptor(DuckContext& duck, const Descriptor& bin);

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
