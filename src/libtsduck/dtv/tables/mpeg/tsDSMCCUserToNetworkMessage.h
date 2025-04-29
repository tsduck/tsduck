//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an DSM-CC User-to-Network Message Table (DownloadServerInitiate, DownloadInfoIndication)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDSMCC.h"

namespace ts {
    //!
    //! Representation of an DSM-CC User-to-Network Message Table (DownloadServerInitiate, DownloadInfoIndication)
    //!
    //! @see ISO/IEC 13818-6, ITU-T Rec. 9.2.2 and 9.2.7. ETSI TR 101 202 V1.2.1 (2003-01), A.1, A.3, A.4, B.
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL DSMCCUserToNetworkMessage: public AbstractLongTable
    {
    public:
        //!
        //! Representation of DSM-CC Message Header structure
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), A.1
        //!
        class TSDUCKDLL MessageHeader
        {
            TS_DEFAULT_COPY_MOVE(MessageHeader);
        public:
            // DSMCCUserToNetworkMessage public members:
            uint8_t  protocol_discriminator = DSMCC_PROTOCOL_DISCRIMINATOR;  //!< Indicates that the message is MPEG-2 DSM-CC message.
            uint8_t  dsmcc_type = DSMCC_TYPE_DOWNLOAD_MESSAGE;               //!< Indicates type of MPEG-2 DSM-CC message.
            uint16_t message_id = 0;                                         //!< Indicates type of message which is being passed.
            uint32_t transaction_id = 0;                                     //!< Used for session integrity and error processing.

            //!
            //! Default constructor.
            //!
            MessageHeader() = default;

            //!
            //! Clear values.
            //!
            void clear();
        };

        //!
        //! Representation of Tap structure
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.2.5
        //!
        class TSDUCKDLL Tap
        {
        public:
            Tap() = default;                    //!< Default constructor.
            uint16_t id = 0x0000;               //!< This field is for private use (shall be set to zero if not used).
            uint16_t use = 0x0016;              //!< Field indicating the usage of the Tap.
            uint16_t association_tag = 0x0000;  //!< Field to associate the Tap with a particular (Elementary) Stream.
            uint16_t selector_type = 0x0001;    //!< Optional selector, to select the associated data from the associated (Elementary) Stream.
            uint32_t transaction_id = 0;        //!< Used for session integrity and error processing.
            uint32_t timeout = 0;               //!< Defined in units of µs, specific to the construction of a particular carousel.
        };

        //  *****************************************
        //  *** DownloadServerInitiate Structures ***
        //  *****************************************

        //!
        //! Representation of LiteComponent structure (BIOP::Object Location, DSM::ConnBinder)
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), Table 4.5
        //!
        class TSDUCKDLL LiteComponent
        {
        public:
            LiteComponent() = default;      //!< Default constructor.
            uint32_t component_id_tag = 0;  //!< Component idenfitier tag (eg. TAG_ObjectLocation, TAG_ConnBinder).

            // BIOPObjectLocation context
            uint32_t  carousel_id = 0;       //!< The carouselId field provides a context for the moduleId field.
            uint16_t  module_id = 0;         //!< Identifies the module in which the object is conveyed within the carousel.
            uint8_t   version_major = 0x01;  //!< Fixed, BIOP protocol major version 1.
            uint8_t   version_minor = 0x00;  //!< Fixed, BIOP protocol minor version 0.
            ByteBlock object_key_data {};    //!< Identifies the object within the module in which it is broadcast.

            // DSMConnBinder context
            Tap tap {};  //!< Tap structure

            // UnknownComponent context
            std::optional<ByteBlock> component_data {};  //!< Optional component data, for UnknownComponent.
        };

        //!
        //! Representation of TaggedProfile structure (BIOP Profile Body, Lite Options Profile Body)
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.3.2, 4.7.3.3
        //!
        class TSDUCKDLL TaggedProfile
        {
        public:
            TaggedProfile() = default;             //!< Default constructor.
            uint32_t profile_id_tag = 0;           //!< Profile identifier tag (eg. TAG_BIOP, TAG_LITE_OPTIONS).
            uint8_t  profile_data_byte_order = 0;  //!< Fixed 0x00, big endian byte order.

            // BIOP Profile Body context
            std::list<LiteComponent> liteComponents {};  //!< List of LiteComponent.

            // Any other profile context for now
            std::optional<ByteBlock> profile_data {};  //!< Optional profile data, for UnknownProfile.
        };

        //!
        //! Representation of Interoperable Object Reference (IOR) structure
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), 4.7.3.1
        //!
        class TSDUCKDLL IOR
        {
        public:
            IOR() = default;                              //!< Default constructor.
            ByteBlock                type_id {};          //!< U-U Objects type_id.
            std::list<TaggedProfile> tagged_profiles {};  //!< List of tagged profiles.
        };

        //  *****************************************
        //  *** DownloadInfoIndication Structures ***
        //  *****************************************

        //!
        //! Representation of BIOP::ModuleInfo structure
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), Table 4.14
        //!
        class TSDUCKDLL Module: public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Module);
            TS_DEFAULT_ASSIGMENTS(Module);
        public:
            uint16_t       module_id = 0;       //!< Identifies the module.
            uint32_t       module_size = 0;     //!< Length of the module in bytes.
            uint8_t        module_version = 0;  //!< Identifies the version of the module.
            uint32_t       module_timeout = 0;  //!< Time out value in microseconds that may be used to time out the acquisition of all Blocks of the Module.
            uint32_t       block_timeout = 0;   //!< Time out value in microseconds that may be used to time out the reception of the next Block after a Block has been acquired.
            uint32_t       min_block_time = 0;  //!< Indicates the minimum time period that exists between the delivery of two subsequent Blocks of the Module.
            std::list<Tap> taps {};             //!< List of Taps.
                                                //
            //!
            //! Constructor.
            //! @param [in] table Parent DSMCCUserToNetworkMessage Table.
            //!
            explicit Module(const AbstractTable* table);
        };

        MessageHeader header {};     //!< DSM-CC Message Header.
        ByteBlock     server_id {};  //!< Field shall be set to 20 bytes with the value 0xFF.
        IOR           ior {};        //!< Interoperable Object Reference (IOR) structure.

        //!
        //! List of Modules
        //!
        using ModuleList = AttachedEntryList<Module>;

        uint32_t   download_id = 0;  //!< Same value as the downloadId field of the DownloadDataBlock() messages which carry the Blocks of the Module.
        uint16_t   block_size = 0;   //!< Block size of all the DownloadDataBlock() messages which convey the Blocks of the Modules.
        ModuleList modules;          //!< List of modules structures.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        DSMCCUserToNetworkMessage(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DSMCCUserToNetworkMessage(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        DeclareDisplaySection();
        virtual bool     isPrivate() const override;
        virtual uint16_t tableIdExtension() const override;

    protected:
        // Inherited methods
        virtual void   clearContent() override;
        virtual size_t maxPayloadSize() const override;
        virtual void   serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void   deserializePayload(PSIBuffer&, const Section&) override;
        virtual void   buildXML(DuckContext&, xml::Element*) const override;
        virtual bool   analyzeXML(DuckContext& duck, const xml::Element* element) override;

    private:
        static constexpr size_t MESSAGE_HEADER_SIZE = 12;  //!< DSM-CC Message Header size w/o adaptation header.
        static constexpr size_t SERVER_ID_SIZE = 20;       //!< Fixed size in bytes of server_id.
    };
}
