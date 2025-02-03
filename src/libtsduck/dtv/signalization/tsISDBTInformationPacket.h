//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  ISDB-T Information Packet (IIP).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"

namespace ts {

    class DuckContext;
    class PSIBuffer;

    //!
    //! ISDB-T Information Packet (IIP).
    //! Contained in specific TS packets in IIP PID (0x1FF0).
    //! @ingroup libtsduck mpeg
    //! @see ARIB STD-B31, section 5.5.3
    //!
    class TSDUCKDLL ISDBTInformationPacket
    {
    public:
        //!
        //! ISDB mode_GI_information in IIP
        //!
        class TSDUCKDLL ModeGI
        {
        public:
            uint8_t initialization_timing_indicator = 0;  //!< 4 bits.
            uint8_t current_mode = 0;                     //!< 2 bits.
            uint8_t current_guard_interval = 0;           //!< 2 bits.
            uint8_t next_mode = 0;                        //!< 2 bits.
            uint8_t next_guard_interval = 0;              //!< 2 bits.

            //! @cond nodoxygen
            ModeGI() = default;
            void deserialize(DuckContext& duck, PSIBuffer& buf);
            void display(DuckContext& duck, std::ostream& strm, const UString& margin) const;
            //! @endcond
        };

        //!
        //! ISDB transmission_parameters in IIP
        //!
        class TSDUCKDLL TransmissionParameters
        {
        public:
            uint8_t modulation_scheme = 0;            //!< 3 bits.
            uint8_t coding_rate_of_inner_code = 0;    //!< 3 bits.
            uint8_t length_of_time_interleaving = 0;  //!< 3 bits.
            uint8_t number_of_segments = 0;           //!< 4 bits.

            //! @cond nodoxygen
            TransmissionParameters() = default;
            void deserialize(DuckContext& duck, PSIBuffer& buf);
            void display(DuckContext& duck, std::ostream& strm, const UString& margin) const;
            //! @endcond
        };

        //!
        //! ISDB configuration_information in IIP
        //!
        class TSDUCKDLL Configuration
        {
        public:
            bool                   partial_reception_flag = false;          //!< 1 bit.
            TransmissionParameters transmission_parameters_for_layer_A {};  //!< Layer A.
            TransmissionParameters transmission_parameters_for_layer_B {};  //!< Layer B.
            TransmissionParameters transmission_parameters_for_layer_C {};  //!< Layer C.

            //! @cond nodoxygen
            Configuration() = default;
            void deserialize(DuckContext& duck, PSIBuffer& buf);
            void display(DuckContext& duck, std::ostream& strm, const UString& margin) const;
            //! @endcond
        };

        //!
        //! ISDB TMCC_information in IIP
        //!
        class TSDUCKDLL TMCC
        {
        public:
            uint8_t       system_identifier = 0;  //!< 2 bits.
            uint8_t       count_down_index = 0;   //!< 4 bits.
            bool          switch_on_control_flag_used_for_alert_broadcasting = false;  //!< 1 bit.
            Configuration current_configuration_information {};                  //!< Current configuration.
            Configuration next_configuration_information {};                     //!< Next configuration.
            uint8_t       phase_correction_of_CP_in_connected_transmission = 0;  //!< 3 bits.
            uint16_t      TMCC_reserved_future_use = 0;                          //!< 12 bits.

            //! @cond nodoxygen
            TMCC() = default;
            void deserialize(DuckContext& duck, PSIBuffer& buf);
            void display(DuckContext& duck, std::ostream& strm, const UString& margin) const;
            //! @endcond
        };

        //!
        //! ISDB modulation_control_configuration_information in IIP.
        //! @see ARIB STD-B31, section 5.5.3
        //!
        class TSDUCKDLL ModulationControlConfiguration
        {
        public:
            uint8_t TMCC_synchronization_word = 0;   //!< 1 bit.
            uint8_t AC_data_effective_position = 0;  //!< 1 bit.
            ModeGI  mode_GI_information {};          //!< Mode GI.
            TMCC    TMCC_information {};             //!< TMCC.

            //! @cond nodoxygen
            ModulationControlConfiguration() = default;
            void deserialize(DuckContext& duck, PSIBuffer& buf);
            void display(DuckContext& duck, std::ostream& strm, const UString& margin) const;
            //! @endcond
        };

        //!
        //! ISDB equipment_control in IIP.
        //! @see ARIB STD-B31, section 5.5.3
        //!
        class TSDUCKDLL EquipmentControl
        {
        public:
            uint16_t equipment_id = 0;              //!< 12 bits.
            bool     renewal_flag = false;          //!< 1 bit.
            bool     static_delay_flag = false;     //!< 1 bit.
            bool     time_offset_polarity = false;  //!< 1 bit.
            uint32_t time_offset = 0;               //!< 24 bits.

            //! @cond nodoxygen
            EquipmentControl() = default;
            void deserialize(DuckContext& duck, PSIBuffer& buf);
            void display(DuckContext& duck, std::ostream& strm, const UString& margin) const;
            //! @endcond
        };

        //!
        //! ISDB network_synchronization in IIP.
        //! @see ARIB STD-B31, section 5.5.3
        //!
        class TSDUCKDLL NetworkSynchronization
        {
        public:
            bool     is_valid = true;                 //!< The structure is optional.
            uint8_t  synchronization_id = 0;          //!< 8 bits.
            uint32_t synchronization_time_stamp = 0;  //!< 24 bits.
            uint32_t maximum_delay = 0;               //!< 24 bits.
            std::vector<EquipmentControl> equipment_control_information {}; //!< Equipment control.

            //! @cond nodoxygen
            NetworkSynchronization() = default;
            void deserialize(DuckContext& duck, PSIBuffer& buf);
            void display(DuckContext& duck, std::ostream& strm, const UString& margin) const;
            //! @endcond
        };

        // Public information, see ARIB STD-B31, section 5.5.3.
        bool                           is_valid = true;                                 //!< Reset by constructor from an invalid data area.
        uint16_t                       IIP_packet_pointer = 0;                          //!< 16 bits.
        ModulationControlConfiguration modulation_control_configuration_information {}; //!< Modulation control.
        uint8_t                        IIP_branch_number = 0;                           //!< 8 bits.
        uint8_t                        last_IIP_branch_number = 0;                      //!< 8 bits.
        NetworkSynchronization         network_synchronization_information {};          //!< Network synchronization.

        //!
        //! Default constructor.
        //!
        ISDBTInformationPacket() = default;

        //!
        //! Constructor from a TS packet.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] pkt TS packet.
        //! @param [in] check_standards If true, do nothing if ISDB is not part of @a duck standards.
        //! When false, we assume ISDB.
        //!
        ISDBTInformationPacket(DuckContext& duck, const TSPacket& pkt, bool check_standards);

        //!
        //! Deserialize a binary area into this object.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] data Address of the binary area to deserialize.
        //! @param [in] size Size in bytes of the binary area to deserialize.
        //! @param [in] check_standards If true, do nothing if ISDB is not part of @a duck standards.
        //! @return True on success, false on error, same value as @a is_valid.
        //!
        bool deserialize(DuckContext& duck, const void* data, size_t size, bool check_standards);

        //!
        //! Display the content of this object.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] strm A standard stream in output mode (text mode).
        //! @param [in] margin Left margin content.
        //!
        void display(DuckContext& duck, std::ostream& strm, const UString& margin) const;
    };
}
