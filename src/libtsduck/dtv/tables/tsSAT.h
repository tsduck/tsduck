//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-, Paul Higgs
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Satellite Access Table (SAT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsVariable.h"
#include "tsFloatUtils.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"

namespace ts {
    //!
    //! Representation of a Satellite Access Table (SAT).
    //! @see ETSI EN 300 468, 5.2.11
    //! @ingroup table
    //!
    class TSDUCKDLL SAT : public AbstractLongTable
    {
    public:
        class satellite_position_v2_info_type {
        public:
            class geostationary_position_type {
            public:
                uint16_t  orbital_position;   //!< Orbital position, unit is 0.1 degree.
                int       west_east_flag;     //!< True (1) for East, false (0) for West.

                geostationary_position_type();
                geostationary_position_type(const geostationary_position_type& other);
                geostationary_position_type& operator=(const geostationary_position_type& other);

                void serialize(BinaryTable&, PSIBuffer&) const;
                void deserialize(PSIBuffer&, const Section&);
            };

            class earth_orbiting_satallite_type {
            public:
                uint8_t           epoch_year;         //!< 8 bits. last 2 digits of the epoch year
                uint16_t          day_of_the_year;    //!< 16 bits. epoch day of the year
                ieee_float32_t    day_fraction;       //!< epoch day fraction.
                ieee_float32_t    mean_motion_first_derivative;
                ieee_float32_t    mean_motion_second_derivative;
                ieee_float32_t    drag_term;
                ieee_float32_t    inclination;
                ieee_float32_t    right_ascension_of_the_ascending_node;
                ieee_float32_t    eccentricity;
                ieee_float32_t    argument_of_perigree;
                ieee_float32_t    mean_anomaly;
                ieee_float32_t    mean_motion;
                
                earth_orbiting_satallite_type();
                earth_orbiting_satallite_type(const earth_orbiting_satallite_type& other);
                earth_orbiting_satallite_type& operator=(const earth_orbiting_satallite_type& other);

                void serialize(BinaryTable&, PSIBuffer&) const;
                void deserialize(PSIBuffer&, const Section&);
            };

            uint32_t        satellite_id;           //!< 24 bits
            uint8_t         position_system;        //!< 1 bit

            //!<for position_system==POS_GEOSTATIONARY
            Variable<geostationary_position_type>   geostationaryPosition;

            //!< for positon_system==POS_EARTH_ORBITING
            Variable<earth_orbiting_satallite_type>  earthOrbiting;

            satellite_position_v2_info_type();
            satellite_position_v2_info_type(const satellite_position_v2_info_type& other);

            bool fromXML(const xml::Element*);
            void toXML(xml::Element*);
            void serialize(BinaryTable&, PSIBuffer&) const;
            void deserialize(PSIBuffer&, const Section&);
        };

        class NCR_type {
        public:
            uint64_t        base;       //!< 33 bits. NCR time div 300, as specified in ETSI EN 301 790  and ISO/IEC 13818-1. 
            uint16_t        ext;        //!< 9 bits. NCR time mod 300, as specified in ETSI EN 301 790 and ISO/IEC 13818-1. 

            NCR_type();
            NCR_type(const NCR_type& other);

            void clear();
            bool fromXML(const xml::Element*, const UString);
            void toXML(xml::Element*, const UString);
            void serialize(BinaryTable&, PSIBuffer&) const;
            void deserialize(PSIBuffer&, const Section&);

            static uint16_t serialized_length() { return 6; }
        };

        class cell_fragment_info_type {
        public:
            class new_delivery_system_id_type {
            public:
                uint32_t    new_delivery_system_id;
                NCR_type    time_of_application;

                new_delivery_system_id_type();
                new_delivery_system_id_type(const new_delivery_system_id_type& other);
                void serialize(BinaryTable&, PSIBuffer&) const;
                void deserialize(PSIBuffer&, const Section&);
            };

            class obsolescent_delivery_system_id_type {
            public:
                uint32_t    obsolescent_delivery_system_id;
                NCR_type    time_of_obsolescence;

                obsolescent_delivery_system_id_type();
                obsolescent_delivery_system_id_type(const obsolescent_delivery_system_id_type& other);
                void serialize(BinaryTable&, PSIBuffer&) const;
                void deserialize(PSIBuffer&, const Section&);
            };

            uint32_t                cell_fragment_id;           //!< 32 bits
            bool                    first_occurence;
            bool                    last_occurence;
            Variable<int32_t>       center_latitude;            //!< 18 bits - tcimsbf
            Variable<int32_t>       center_longitude;           //!< 19 bits - tcimsbf
            Variable<uint32_t>      max_distance;               //!< 24 bits
            std::vector<uint32_t>                               delivery_system_ids;                          // list of 32 bit values
            std::vector<new_delivery_system_id_type>            new_delivery_system_ids;
            std::vector<obsolescent_delivery_system_id_type>    obsolescent_delivery_system_ids;

            cell_fragment_info_type();
            cell_fragment_info_type(const cell_fragment_info_type& other);

            bool fromXML(const xml::Element*);
            void toXML(xml::Element*);
            void serialize(BinaryTable&, PSIBuffer&) const;
            void deserialize(PSIBuffer&, const Section&);
        };

        class time_association_info_type {
        public:
            uint8_t     association_type;                   //!< 4 bits. Indicates how the association_timestamp is to be interpreted (valid: 0 or 1).

            NCR_type    ncr;                                //!< NCR time as specified in ETSI EN 301 790 and ISO/IEC 13818-1. The NCR time is associated with the association_timestamp. The NCR time used in the association shall be between 648 000 000 (2 hours) in the past and 7 776 000 000 (24 hours) in the future. Typically, it will be very close to the current NCR.

            uint64_t    association_timestamp_seconds;      //!< The number of seconds of the association_timestamp since January 1st, 1970 00:00:00.

            uint32_t    association_timestamp_nanoseconds;  //!< The number of nanoseconds of the association timestamp on top of the association_timestamp_seconds since January 1st, 1970 00:00:00. (maximum: 1 000 000 000 )

            bool        leap59;                     //!< Set to '1' to announce that a leap second will be skipped at the end of the quarter of the year to which the association timestamp belongs. Otherwise, this field shall be set to '0'

            bool        leap61;                     //!< Set to '1' to announce that a leap second will be added at the end of the quarter of a year to which the association timestamp belongs. In case that the association timestamp refers to a time within such an added leap second, and the leap second is already added to the association timestamp, then this flag shall be set to '0'. In all other cases this field shall be set to '0'.

            bool        past_leap59;                //!< Set to '1' to announce that a leap second was skipped at the end of the quarter of a year previous to the quarter to which the association timestamp belongs. Otherwise, this field shall be set to '0'.

            bool        past_leap61;                //!< Set to '1' to announce that a leap second is currently being added, when the association timestamp refers to the last second in a quarter of a year. This field may also be set to '1' to announce that a leap second was added at the end of the quarter of a year previous to the quarter to which the association timestamp belongs except when it belongs to the last second in that quarter. Otherwise, this field shall be set to '0'.

            time_association_info_type();
            time_association_info_type(const time_association_info_type& other);

            void clear();
            bool fromXML(const xml::Element*);
            void toXML(xml::Element*);
            void serialize(BinaryTable&, PSIBuffer&) const;
            void deserialize(PSIBuffer&, const Section&);
        };

        class beam_hopping_time_plan_info_type {
        public:
            class slot {
            public:
                uint16_t    number;
                bool        on;

                slot() : number(0), on(false) {}
                slot(uint16_t number_, bool on_) : number(number_), on(on_) {}
                slot(const slot& other) : number(other.number), on(other.on) {}
                bool operator==(const slot& rhs) const { return this->number == rhs.number; }
            };

            uint32_t                beamhopping_time_plan_id; 
            NCR_type                time_of_application;
            NCR_type                cycle_duration;

            // time_plan_mode == HOP_1_TRANSMISSION
            Variable<NCR_type>      dwell_duration;
            Variable<NCR_type>      on_time;

            // time_plan_mode == HOP_MULTI_TRANSMISSION
            Variable<uint16_t>      current_slot;  //!<15 bits
            std::vector<slot>       slot_duration_on;

            // time_plan_mode == HOP_GRID
            Variable<NCR_type>      grid_size;
            Variable<NCR_type>      revisit_duration;
            Variable<NCR_type>      sleep_time;
            Variable<NCR_type>      sleep_duration;

            beam_hopping_time_plan_info_type();
            beam_hopping_time_plan_info_type(const beam_hopping_time_plan_info_type& other);
       
            //!
            //! Determines the size of this iteration of a beam hopping time plan to allow quick jumping to the next iteration.
            //! @return The size, in bytes (12 bits), of this iteration in the loop, starting with the beamhopping_time_plan_id and ending at the end of the loop.
            //! 
            uint16_t plan_length(void) const;
            //!
            //! Determines the time plan mode for this beam hopping time plan.
            //! @return (2 bits), indicating the time plan mode or -1 plan mode cannot be determined
            //! 
            uint8_t time_plan_mode(void) const;

            bool fromXML(const xml::Element*);
            void toXML(xml::Element*);
            void serialize(BinaryTable&, PSIBuffer&) const;
            void deserialize(PSIBuffer&, const Section&);
        };


        // SAT public members:
        std::vector<satellite_position_v2_info_type>    satellite_position_v2_info;
        std::vector<cell_fragment_info_type>            cell_fragment_info;
        time_association_info_type                      time_association_fragment_info;
        std::vector<beam_hopping_time_plan_info_type>   beam_hopping_time_plan_info;

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //! @param [in] satellite_table_id_ The table type 
        //! @param [in] table_count_ depends on table type, see ETSI EN 300 468 clause 5.2.11.1 
        //!
        SAT(uint8_t vers = 0, bool cur = true, uint16_t satellite_table_id_ = 0, uint16_t table_count_ = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        SAT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        SAT(const SAT& other);

         //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        SAT& operator=(const SAT& other);

        // Inherited methods.
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    private:
        uint16_t satellite_table_id;
        uint16_t table_count;

        static UString degrees18(uint32_t bin_val);
        static UString degrees19(uint32_t bin_val);
        static UString ncr(PSIBuffer& buf);

    protected:
        // Inherited methods.
        virtual void clearContent() override;
        virtual size_t maxPayloadSize() const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };

    // for satellite access table
    constexpr auto SATELLITE_POSITION_V2_INFO = 0;
    constexpr auto CELL_FRAGMENT_INFO = 1;
    constexpr auto TIME_ASSOCIATION_INFO = 2;
    constexpr auto BEAMHOPPING_TIME_PLAN_INFO = 3;

    // for satellite position v2 info
    constexpr auto POSITION_SYSTEM_GEOSTATIONARY = 0;
    constexpr auto POSITION_SYSTEM_EARTH_ORBITING = 1;

    // for beam hopping time plan
    constexpr auto HOP_1_TRANSMISSION = 0;
    constexpr auto HOP_MULTI_TRANSMISSION = 1;
    constexpr auto HOP_GRID = 2;
}
