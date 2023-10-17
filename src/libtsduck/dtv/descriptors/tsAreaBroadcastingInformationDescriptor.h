//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB area_broadcasting_information_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {

    //!
    //! Representation of an ISDB area_broadcasting_information_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.55
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AreaBroadcastingInformationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Station entry.
        //!
        struct TSDUCKDLL Station
        {
            Station() = default;                    //!< Constructor.
            uint32_t  station_id = 0;               //!< 24 bits, station id.
            uint16_t  location_code = 0;            //!< Location code.
            uint8_t   broadcast_signal_format = 0;  //!< Broadcast signal format.
            ByteBlock additional_station_info {};   //!< Additional station info.
        };

        typedef std::list<Station> StationList;  //!< List of stations.

        // AreaBroadcastingInformationDescriptor public members:
        StationList stations {};  //!< List of stations.

        //!
        //! Default constructor.
        //!
        AreaBroadcastingInformationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AreaBroadcastingInformationDescriptor(DuckContext& duck, const Descriptor& bin);

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
