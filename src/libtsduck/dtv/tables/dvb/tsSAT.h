//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Satellite Access Table (SAT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
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
        //!
        //! Base capabilities to be defined/extended by Satellite Access Table processing functions
        //!
        class SAT_base
        {
            TS_INTERFACE(SAT_base);
        public:
            //!
            //! This method populates this object from XML attributes and sub-element
            //! @param [in] element  The element whose attributes and sub-elements are parsed to construct this object
            //! @return  true if the requisite attributes and sub-elements are available and correct
            //!
            virtual bool fromXML(const xml::Element* element) = 0;

            //!
            //! This method converts this object to XML by populating necessary attributes and sub-elements into the provided element
            //! @param [in,out] root The element whose attributes and sub-elements are added to to represent values in this object
            //!
            virtual void toXML(xml::Element* root) = 0;

            //!
            //! This method serializes the attributes of a geostationary satellite.
            //! @param [in,out] buf A PSIBuffer with the appropriate size for the section payload.
            //!
            virtual void serialize(PSIBuffer& buf) const = 0;

            //!
            //! This method deserializes (populates) the attributes of a geostationary satellite.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            virtual void deserialize(PSIBuffer& buf) = 0;
        };

        //!
        //! Representation of a satellite position
        //! @see ETSI EN 300 648, 5.2.11.2
        //!
        class satellite_position_v2_info_type : public SAT_base
        {
        public:
            //!
            //! Representation of a geostationary satellite position
            //! @see ETSI EN 300 648, 5.2.11.2
            //!
            class geostationary_position_type : SAT_base
            {
                TS_DEFAULT_COPY_MOVE(geostationary_position_type);
            public:
                uint16_t orbital_position = 0;  //!< Orbital position, unit is 0.1 degree.
                int      west_east_flag = 0;    //!< True (1) for East, false (0) for West.
                //!
                //! Default constructor.
                //!
                geostationary_position_type() = default;
                //!
                //! Read-in constructor.
                //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
                //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
                //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
                //!
                geostationary_position_type(PSIBuffer& buf) : geostationary_position_type() { deserialize(buf); }

                // Inherited methods
                virtual bool fromXML(const xml::Element* element) override;
                virtual void toXML(xml::Element* root) override;
                virtual void serialize(PSIBuffer& buf) const override;
                virtual void deserialize(PSIBuffer& buf) override;
            };

            //!
            //! Representation of an earth orbiting satellite position
            //! @see ETSI EN 300 648, 5.2.11.2
            //!
            class earth_orbiting_satallite_type : public SAT_base
            {
                TS_DEFAULT_COPY_MOVE(earth_orbiting_satallite_type);
            public:
                uint8_t        epoch_year = 0;                               //!< 8 bits. lLast 2 digits of the epoch year.
                uint16_t       day_of_the_year = 0;                          //!< 16 bits. Epoch day of the year.
                ieee_float32_t day_fraction = 0;                             //!< Epoch day fraction.
                ieee_float32_t mean_motion_first_derivative = 0;             //!< Mean motion derivative divided by 2 in revolutions per day-squared.
                ieee_float32_t mean_motion_second_derivative = 0;            //!< The mean motion second derivative divided by 6 in revolutions per day-cubed.
                ieee_float32_t drag_term = 0;                                //!< Drag term (or radiation pressure coefficient or BSTAR) in 1/EarthRadii.
                ieee_float32_t inclination = 0;                              //!< Angle between the equator and the orbit plane in degrees.
                ieee_float32_t right_ascension_of_the_ascending_node = 0;    //!< Right ascension of the ascension node in degrees.
                ieee_float32_t eccentricity = 0;                             /**< Shape of the orbit(0 = circular, Less than 1 = elliptical).
                                                                                  The value provided is the mean eccentricity. */
                ieee_float32_t argument_of_perigree = 0;                     //!< Argument of perigee in degrees.
                ieee_float32_t mean_anomaly = 0;                             //!< Mean anomaly in degrees.
                ieee_float32_t mean_motion = 0;                              //!< Mean number of orbits per day the object completes in revolutions/day.

                //!
                //! Default constructor.
                //!
                earth_orbiting_satallite_type() = default;
                //!
                //! Read-in constructor.
                //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
                //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
                //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
                //!
                earth_orbiting_satallite_type(PSIBuffer& buf) : earth_orbiting_satallite_type() { deserialize(buf); }

                // Inherited methods
                virtual bool fromXML(const xml::Element* element) override;
                virtual void toXML(xml::Element* root) override;
                virtual void serialize(PSIBuffer& buf) const override;
                virtual void deserialize(PSIBuffer& buf) override;
            };

            uint32_t satellite_id = 0;      //!< 24 bits, A label to identify the satellite that is detailed here.
            uint8_t  position_system = 0;   /**< 1 bit.The positioning system that is used  for this satellite.
                                                 The value '0' can be used for a satellite in geostationary orbit and the value '1'
                                                 can be used for any earth-orbiting satellite. */

            // for position_system==POS_GEOSTATIONARY
            std::optional<geostationary_position_type> geostationaryPosition {};  //!< attributes of a geostationary satellite.

            // for positon_system==POS_EARTH_ORBITING
            std::optional<earth_orbiting_satallite_type> earthOrbiting {};        //!< attributes of an earth orbiting satellite

            //!
            //! Default constructor
            //!
            satellite_position_v2_info_type() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            satellite_position_v2_info_type(PSIBuffer& buf) : satellite_position_v2_info_type() { deserialize(buf); }

            // Inherited methods
            virtual bool fromXML(const xml::Element* element) override;
            virtual void toXML(xml::Element* root) override;
            virtual void serialize(PSIBuffer& buf) const override;
            virtual void deserialize(PSIBuffer& buf) override;
        };

        //!
        //! Network Clock Reference
        //!
        class NCR_type : public SAT_base
        {
            TS_DEFAULT_COPY_MOVE(NCR_type);
        public:
            uint64_t base = 0;  //!< 33 bits. NCR time div 300, as specified in ETSI EN 301 790  and ISO/IEC 13818-1.
            uint16_t ext = 0;   //!< 9 bits. NCR time mod 300, as specified in ETSI EN 301 790 and ISO/IEC 13818-1.

            //!
            //! Default constructor.
            //!
            NCR_type() = default;
            //!
            //! Clear values.
            //!
            void clear();
            //!
            //! The length (in bytes) of a network clock reference when serialized.
            //! @returns the length in bytes of a serialized network clock reference.
            //!
            static uint16_t serialized_length() { return 6; }
            //!
            //! This method populates this object from XML attributes and sub-element
            //! @param [in] parent  The element whose attributes and sub-elements are parsed to construct this object
            //! @param [in] element_name  The name of the new element that contaiuning the attributes associated with this clock reference.
            //! @return  true if the requisite attributes and sub-elements are available and correct
            //!
            bool fromXML(const xml::Element* parent, const UString element_name);
            //!
            //! This method converts this object to XML by populating necessary attributes and sub-elements into the provided element
            //! @param [in,out] parent  The element whose attributes and sub-elements are added to to represent values in this object
            //! @param [in] element_name  The name of the element that will contain the serialized values as attributes..
            //!
            void toXML(xml::Element* parent, const UString element_name);

            // Inherited methods
            virtual bool fromXML(const xml::Element* element) override;
            virtual void toXML(xml::Element* root) override;
            virtual void serialize(PSIBuffer& buf) const override;
            virtual void deserialize(PSIBuffer& buf) override;
        };

        //!
        //! Representation of a cell fragment
        //! An area on earth identified by a center (center_lattitude and center_longitude) and max_distance from the center,
        //! that is associated with none, one or multiple delivery systems
        //! @see ETSI EN 300 648, 5.2.11.3
        //!
        class cell_fragment_info_type : public SAT_base
        {
            TS_DEFAULT_COPY_MOVE(cell_fragment_info_type);
        public:
            //!
            //! Representation of a new delivery system
            //! A new delivery system that is soon serving this cell fragment.
            //! @see ETSI EN 300 648, 5.2.11.3
            //!
            class new_delivery_system_id_type : public SAT_base
            {
                TS_DEFAULT_COPY_MOVE(new_delivery_system_id_type);
            public:
                uint32_t new_delivery_system_id = 0;  //!< The identifier of a new delivery system that is soon serving this cell fragment.
                NCR_type time_of_application {};      //!< The network clock reference of the time when the specified delivery system will be serving the cell fragment.

                //!
                //! Default constructor.
                //!
                new_delivery_system_id_type() = default;
                //!
                //! Read-in constructor.
                //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
                //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
                //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
                //!
                new_delivery_system_id_type(PSIBuffer& buf) : new_delivery_system_id_type() { deserialize(buf); }

                // Inherited methods
                virtual bool fromXML(const xml::Element* element) override;
                virtual void toXML(xml::Element* root) override;
                virtual void serialize(PSIBuffer& buf) const override;
                virtual void deserialize(PSIBuffer& buf) override;
            };

            //!
            //! Representation of am obsolescent delivery system
            //! A delivery system that will soon stop serving this cell fragment.
            //! @see ETSI EN 300 648, 5.2.11.3
            //!
            class obsolescent_delivery_system_id_type : public SAT_base
            {
                TS_DEFAULT_COPY_MOVE(obsolescent_delivery_system_id_type);
            public:
                uint32_t obsolescent_delivery_system_id = 0;  //!< The delivery system id of a delivery system that is soon no longer serving this cell fragment.
                NCR_type time_of_obsolescence {};             //!< The network clock reference of the time when the specified delivery system will no longer be serving the cell fragment.

                //!
                //! Default constructor.
                //!
                obsolescent_delivery_system_id_type() = default;
                //!
                //! Read-in constructor.
                //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
                //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
                //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
                //!
                obsolescent_delivery_system_id_type(PSIBuffer& buf) : obsolescent_delivery_system_id_type() { deserialize(buf); }

                // Inherited methods
                virtual bool fromXML(const xml::Element* element) override;
                virtual void toXML(xml::Element* root) override;
                virtual void serialize(PSIBuffer& buf) const override;
                virtual void deserialize(PSIBuffer& buf) override;
            };

            uint32_t          cell_fragment_id = 0;     /**< The identifier of this cell fragment. It can occur multiple times in consecutive
                                                             sections if the signalling is spanned over multiple sections.     */
            bool              first_occurence = false;  /**< Indicates that this section is the first section in a sequence of 1 or more sections
                                                             that contain the information for the cell fragment. When this bit is set to 1, it is
                                                             the first section of the sequence. When it is set to 0, it is not the first section. */
            bool              last_occurence = false;   /**< Indicates that this section is the last section in a sequence of 1 or more sections that
                                                             contain the information for the cell fragment. When this bit is set to 1, it is the
                                                             last section of the sequence. When it is set to 0, it is not the last section. */
            std::optional<int32_t> center_latitude {};  /**< 18 bits - tcimsbf. The current center of the cell fragment on earth in the WGS84 datum
                                                             in units of 0.001 degrees. Northern latitudes shall be stated as a positive number,
                                                             southern latitudes as negative. The range is evidently +/- 90 degrees. */
            std::optional<int32_t> center_longitude {}; /**< 19 bits - tcimsbf. The current center of the cell fragment on earth in the WGS84 datum,
                                                             in units of 0.001 degrees. Eastern longitudes shall be stated as a positive number,
                                                             western longitudes as negative. The range is evidently +/- 180 degrees. */
            std::optional<uint32_t> max_distance {};    /**< 24 bits. Indicates the maximum distance on the surface of the earth from the center
                                                             of the cell-fragment in units of 1 meter that is still considered to be part of the cell fragment */

            std::vector<uint32_t>                            delivery_system_ids {};                //!< Identifiers of delivery systems currently serving this cell fragment.
            std::vector<new_delivery_system_id_type>         new_delivery_system_ids {};            //!< Delivery systems that are soon serving this cell fragment.
            std::vector<obsolescent_delivery_system_id_type> obsolescent_delivery_system_ids {};    //!< Delivery systems that wil soon stopm serving this cell fragment.

            //!
            //! Default constructor.
            //!
            cell_fragment_info_type() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            cell_fragment_info_type(PSIBuffer& buf) : cell_fragment_info_type() { deserialize(buf); }

            // Inherited methods
            virtual bool fromXML(const xml::Element* element) override;
            virtual void toXML(xml::Element* root) override;
            virtual void serialize(PSIBuffer& buf) const override;
            virtual void deserialize(PSIBuffer& buf) override;
        };

        //!
        //! Representation of a time assocition between NCR and UTC
        //! @see ETSI EN 300 648, 5.2.11.4
        //!
        class time_association_info_type : public SAT_base
        {
            TS_DEFAULT_COPY_MOVE(time_association_info_type);
        public:
            uint8_t  association_type = 0;                  //!< 4 bits. Indicates how the association_timestamp is to be interpreted (valid: 0 or 1).

            NCR_type ncr {};                                /**< NCR time as specified in ETSI EN 301 790 and ISO/IEC 13818-1. The NCR time is associated
                                                                 with the association_timestamp. The NCR time used in the association shall be between
                                                                 648 000 000 (2 hours) in the past and 7 776 000 000 (24 hours) in the future. Typically,
                                                                 it will be very close to the current NCR. */

            uint64_t association_timestamp_seconds = 0;     //!< The number of seconds of the association_timestamp since January 1st, 1970 00:00:00.

            uint32_t association_timestamp_nanoseconds = 0; /**< The number of nanoseconds of the association timestamp on top of the
                                                                 association_timestamp_seconds since January 1st, 1970 00:00 : 00. (maximum : 1 000 000 000). */

            bool     leap59 = false;                        /**< Set to '1' to announce that a leap second will be skipped at the end of the quarter of the
                                                                 year to which the association timestamp belongs.Otherwise, this field shall be set to '0'. */

            bool     leap61 = false;                        /**< Set to '1' to announce that a leap second will be added at the end of the quarter of a year
                                                                 to which the association timestamp belongs. In case that the association timestamp refers to
                                                                 a time within such an added leap second, and the leap second is already added to the
                                                                 association timestamp, then this flag shall be set to '0'. In all other cases this field
                                                                 shall be set to '0'. */

            bool     past_leap59 = false;                   /**< Set to '1' to announce that a leap second was skipped at the end of the quarter of a year
                                                                 previous to the quarter to which the association timestamp belongs. Otherwise, this field
                                                                 shall be set to '0'. */

            bool     past_leap61 = false;                   /**< Set to '1' to announce that a leap second is currently being added, when the association
                                                                 timestamp refers to the last second in a quarter of a year. This field may also be set to '1'
                                                                 to announce that a leap second was added at the end of the quarter of a year previous to
                                                                 the quarter to which the association timestamp belongs except when it belongs to the last
                                                                 second in that quarter. Otherwise, this field shall be set to '0'. */
            //!
            //! Default constructor.
            //!
            time_association_info_type() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            time_association_info_type(PSIBuffer& buf) : time_association_info_type() { deserialize(buf); }
            //!
            //! Clear values.
            //!
            void clear();

            // Inherited methods
            virtual bool fromXML(const xml::Element* element) override;
            virtual void toXML(xml::Element* root) override;
            virtual void serialize(PSIBuffer& buf) const override;
            virtual void deserialize(PSIBuffer& buf) override;
        };

        //!
        //! Representation of a beam hopping time plan, identified by the beamhopping_time_plan_id with information relating to the
        //! period(s) in time that the beam will illuminate the cell each beamhopping cycle.
        //! @see ETSI EN 300 648, 5.2.11.3
        //!
        class beam_hopping_time_plan_info_type : public SAT_base
        {
            TS_DEFAULT_COPY_MOVE(beam_hopping_time_plan_info_type);
        public:
            //!
            //! Indicates if there is a transmission in the respective timeslot.
            //!
            class slot : public SAT_base
            {
                TS_DEFAULT_COPY_MOVE(slot);
            public:
                uint16_t number = 0;  //!< The beam number.
                bool     on = false;  //!< Illuminatiom state of the beam.

                //!
                //! Default constructor.
                //!
                slot() = default;
                //!
                //! Constructor.
                //! @param [in] slot_num Slot number.
                //! @param [in] on_ slot illumination state.
                //!
                slot(uint16_t slot_num, bool on_) : number(slot_num), on(on_) {}
                //!
                //! Read-in constructor.
                //! @param [in] slot_num Slot number.
                //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
                //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
                //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
                //!
                slot(uint16_t slot_num, PSIBuffer& buf) : slot() { deserialize(slot_num, buf); }
                //!
                //! Equivelance operator.
                //! @param [in] rhs  other oblect to compare to.
                //! @return  true if an illumination status has already been signalled for the slot number
                //!
                bool operator==(const slot& rhs) const { return this->number == rhs.number; }
                //!
                //! This method deserializes (populates) the attributes of an illumination time slot.
                //! @param [in] slot_num the time slot for which illimination is being specified.
                //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
                //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
                //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
                //!
                void deserialize(uint16_t slot_num, PSIBuffer& buf);

                // Inherited methods
                virtual bool fromXML(const xml::Element* element) override;
                virtual void toXML(xml::Element* root) override;
                virtual void serialize(PSIBuffer& buf) const override;
                virtual void deserialize(PSIBuffer& buf) override;
            };

            uint32_t           beamhopping_time_plan_id = 0; //!< Label to identify the beamhopping time plan that is detailed in this loop.
            NCR_type           time_of_application {};       //!< NCR of time of application.
            NCR_type           cycle_duration {};            //!< Duration in NCR of cycle duration.

            // time_plan_mode == HOP_1_TRANSMISSION
            std::optional<NCR_type> dwell_duration {};       //!< Duration in NCR of dwell duration.
            std::optional<NCR_type> on_time {};              //!< NCR of on time.

            // time_plan_mode == HOP_MULTI_TRANSMISSION
            std::optional<uint16_t> current_slot {};         /**< 15 bits.The slot in which the transmission of this table started.
                                                                  32767 (all bits set to '1'), denotes that the current slot is not communicated for this entry. */
            std::vector<slot>  slot_transmission_on {};      //!< Indicates if there is a transmission in the respective timeslot.

            // time_plan_mode == HOP_GRID
            std::optional<NCR_type> grid_size {};            //!< The duration in NCR of grid size.
            std::optional<NCR_type> revisit_duration {};     //!< The maximal duration of time by which a cell is not illuminated, when not in sleep mode.
            std::optional<NCR_type> sleep_time {};           /**< The NCR time when the sleep mode will be entered. Starting from this NCR time for the
                                                                  duration given by sleep_time_duration, receivers can expect not to be illuminated. */
            std::optional<NCR_type> sleep_duration {};       //!< Duration in NCR of sleep duration.

            //!
            //! Default constructor.
            //!
            beam_hopping_time_plan_info_type() = default;
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            beam_hopping_time_plan_info_type(PSIBuffer& buf) : beam_hopping_time_plan_info_type() { deserialize(buf); }
            //!
            //! Determines the size of this iteration of a beam hopping time plan to allow quick jumping to the next iteration.
            //! @return The size, in bytes (12 bits), of this iteration in the loop, starting with the beamhopping_time_plan_id
            //! and ending at the end of the loop.
            //!
            uint16_t plan_length(void) const;
            //!
            //! Determines the time plan mode for this beam hopping time plan.
            //! @return (2 bits), indicating the time plan mode or -1 plan mode cannot be determined
            //!
            uint8_t time_plan_mode(void) const;

            // Inherited methods
            virtual bool fromXML(const xml::Element* element) override;
            virtual void toXML(xml::Element* root) override;
            virtual void serialize(PSIBuffer& buf) const override;
            virtual void deserialize(PSIBuffer& buf) override;
        };


        // SAT public members:
        std::vector<satellite_position_v2_info_type>  satellite_position_v2_info {};      //!< Satellite ephemeris data for DVB - S2Xv2 delivery.
        std::vector<cell_fragment_info_type>          cell_fragment_info {};              /**< Cell fragments, areas on earth identified by a center
                                                                                               (center_lattitude and center_longitude) and max_distance from the
                                                                                               center, that are associated with none, one or multiple delivery_system_ids. */
        time_association_info_type                    time_association_fragment_info {};  /**< Time association between NCRand UTC.It allows the receiver to
                                                                                               translate the received NCRs in UTC time. */
        std::vector<beam_hopping_time_plan_info_type> beam_hopping_time_plan_info {};     /**< Beamhopping time plan, identified by the beamhopping_time_plan_id
                                                                                               with information relating to the period(s) in time that the beam will
                                                                                               illuminate the cell each beamhopping cycle. */

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

        // Inherited methods.
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    private:
        uint16_t satellite_table_id = 0;  //!< Identifies each table type.
        uint16_t table_count = 0;         //!< Allows different sub_tables to be distinguished.

        //!
        //! Format an 18-bit 2s compliment value
        //! @param [in] bin_val   A 2s compliment 18 bit representation of a floating point value
        //! @return A string representing the signed floating point value
        //!
        static UString degrees18(const uint32_t bin_val);

        //!
        //! Format an 19-bit 2s compliment value
        //! @param [in] bin_val   A 2s compliment 19 bit representation of a floating point value
        //! @return A string representing the signed floating point value
        //!
        static UString degrees19(const uint32_t bin_val);

        //!
        //! Format a network clock reference being deserialized.
        //! @param [in] buf   Input buffer currently pointing to a serialized network clock reference.
        //! @return A string representing the components and value of the network clock reference
        //!
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
    constexpr auto SATELLITE_POSITION_V2_INFO = 0;      //!< satellite access table contains satellite positioning information
    constexpr auto CELL_FRAGMENT_INFO = 1;              //!< satellite access table contains cell fragment definitions
    constexpr auto TIME_ASSOCIATION_INFO = 2;           //!< satellite access table contains time association information
    constexpr auto BEAMHOPPING_TIME_PLAN_INFO = 3;      //!< satellite access table contains beam hopping timeplans

    // for satellite position v2 info
    constexpr auto POSITION_SYSTEM_GEOSTATIONARY = 0;   //!< satellite is geostationary
    constexpr auto POSITION_SYSTEM_EARTH_ORBITING = 1;  //!< satellite is earth orbiting

    // for beam hopping time plan mode
    constexpr auto HOP_1_TRANSMISSION = 0;              //!< 1 transmission each cycle
    constexpr auto HOP_MULTI_TRANSMISSION = 1;          //!< multiple transmissions in each cycle
    constexpr auto HOP_GRID = 2;                        //!< grid_size will be signalled, but when the illumination will take place is not signalled
}
