//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2018-2024, Tristan Claverie
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

namespace ts {
    //!
    //! Representation of an DSM-CC User-to-Network Message Table (DownloadServerInitiate, DownloadInfoIndication)
    //!
    //! @see ISO/IEC 13818-6, ITU-T Rec. 9.2.2 and 9.2.7. ETSI TR 101 202 V1.2.1 (2003-01), A.1, A.3, A.4, B.
    //! @ingroup table
    //!
    class TSDUCKDLL DSMCCUserToNetworkMessage: public AbstractLongTable {
    public:
        // DSMCCUserToNetworkMessage public members:
        uint8_t  protocol_discriminator = 0x11;  //!< Indicates that the message is MPEG-2 DSM-CC message.
        uint8_t  dsmcc_type = 0x03;              //!< Indicates type of MPEG-2 DSM-CC message.
        uint16_t message_id = 0;                 //!< Indicates type of message which is being passed.
        uint32_t transaction_id = 0;             //!< Used for session integrity and error processing.

        // DownloadServerInitiate
        static constexpr size_t SERVER_ID_SIZE = 20;  //!< Fixed size in bytes of server_id.
        ByteBlock               server_id {};         //!< Conveys the data of the block.
        ByteBlock               private_data {};      //!< Private data.

        // DownloadInfoIndication
        class TSDUCKDLL Module {
        public:
            Module() = default;  //!< Default constructor.
            uint16_t  module_id = 0;
            uint32_t  module_size = 0;
            uint8_t   module_version = 0;
            ByteBlock module_info {};
        };

        uint32_t download_id = 0;
        uint16_t block_size = 0;

        std::list<Module> modules {};  //!< Description of all transport streams.

        ByteBlock dii_private_data {};  //!< Private data.

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
        virtual uint16_t tableIdExtension() const override;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext& duck, const xml::Element* element) override;
    };
}  // namespace ts
