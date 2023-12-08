//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a satellite_delivery_system_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDeliverySystemDescriptor.h"
#include "tsDeliverySystem.h"

namespace ts {
    //!
    //! Representation of a satellite_delivery_system_descriptor.
    //!
    //! This descriptor was originally defined by DVB. It has been reused by ISDB
    //! with a slightly different binary layout and different semantics for the
    //! modulation and FEC fields. But the same descriptor tag is used in both
    //! standards.
    //!
    //! The delivery system, as returned by SatelliteDeliverySystemDescriptor::deliverySystem(),
    //! is one of DS_DVB_S, DS_DVB_S2, DS_ISDB_S.
    //!
    //! - When manipulated as a C++ object, the delivery system can be set by the method
    //!   SatelliteDeliverySystemDescriptor::setDeliverySystem().
    //! - When deserialized from a binary table, the delivery system is DS_ISDB_S when the
    //!   execution context contains ISDB as a standard and DS_DVB_S or DS_DVB_S2 otherwise.
    //! - When deserialized from XML, the attribute @a modulation_system is used.
    //!
    //! @see ETSI EN 300 468, 6.2.13.2.
    //! @see ARIB STD-B10, Part 2, 6.2.6.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SatelliteDeliverySystemDescriptor : public AbstractDeliverySystemDescriptor
    {
    public:
        // SatelliteDeliverySystemDescriptor public members:
        uint64_t frequency = 0;          //!< Frequency in Hz (warning: coded in 10 kHz units in descriptor).
        uint16_t orbital_position = 0;   //!< Orbital position, unit is 0.1 degree.
        bool     east_not_west = false;  //!< True for East, false for West.
        uint8_t  polarization = 0;       //!< Polarization, 2 bits.
        uint64_t symbol_rate = 0;        //!< Symbol rate (warning: coded in 100 symbol/s units in descriptor).
        uint8_t  modulation = 0;         //!< Modulation type, 2 bits with DVB, 5 bits with ISDB.
        uint8_t  roll_off = 0;           //!< Roll-off factor, 2 bits (DVB-S2 only).
        uint8_t  FEC_inner = 0;          //!< FEC inner, 4 bits, value depends on DVB vs. ISDB.

        //!
        //! Default constructor.
        //!
        SatelliteDeliverySystemDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Check if this is a DVB descriptor (ie. not ISDB).
        //! @param [in] duck TSDuck execution context.
        //! @return True if this is a DVB descriptor.
        //!
        bool isDVB(const DuckContext& duck) const { return deliverySystem(duck) != DS_ISDB_S; }

        //!
        //! Check if this is a ISDB descriptor (ie. not DVB).
        //! @param [in] duck TSDuck execution context.
        //! @return True if this is a ISDB descriptor.
        //!
        bool isISDB(const DuckContext& duck) const { return deliverySystem(duck) == DS_ISDB_S; }

        //!
        //! Set the delivery system.
        //! @param [in] duck TSDuck execution context.
        //! @param [in] system The delivery system to set. Must be one of DS_DVB_S, DS_DVB_S2, DS_ISDB_S.
        //! Otherwise, if ISDB is listed in the current standards in the context, the delivery system is set
        //! to DS_ISDB_S. Otherwise, it is set to DS_DVB_S.
        //! @see deliverySystem()
        //!
        void setDeliverySystem(const DuckContext& duck, DeliverySystem system);

        // Inherited methods
        virtual DeliverySystem deliverySystem(const DuckContext&) const override;
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        friend class S2XSatelliteDeliverySystemDescriptor;
        friend class S2Xv2SatelliteDeliverySystemDescriptor;
        friend class SAT;
        static DeliverySystem ResolveDeliverySystem(const DuckContext&, DeliverySystem);

        // Enumerations for XML.
        static const Enumeration DirectionNames;
        static const Enumeration PolarizationNames;
        static const Enumeration RollOffNames;
        static const Enumeration ModulationNamesDVB;
        static const Enumeration ModulationNamesISDB;
        static const Enumeration CodeRateNamesDVB;
        static const Enumeration CodeRateNamesISDB;
    };
}
