//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an DSM-CC Download Data Block Table (DSMCCDownloadDataMessage)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"

namespace ts {
    //!
    //! Representation of an DSM-CC Download Data Message Table (DSMCCDownloadDataMessage)
    //!
    //! @see ISO/IEC 13818-6, ITU-T Rec. 9.2.2 and 9.2.7. ETSI TR 101 202 V1.2.1 (2003-01), A.2, A.5, B
    //! @ingroup table
    //!
    class TSDUCKDLL DSMCCDownloadDataMessage: public AbstractLongTable {
    public:
        //!
        //! Representation of Download Data Header
        //! @see ETSI TR 101 202 V1.2.1 (2003-01), A.2
        //!
        class TSDUCKDLL DownloadDataHeader {
            TS_DEFAULT_COPY_MOVE(DownloadDataHeader);

        public:
            // DSMCCDownloadDataMessage public members:
            uint8_t  protocol_discriminator = DSMCC_PROTOCOL_DISCRIMINATOR;  //!< Indicates that the message is MPEG-2 DSM-CC message.
            uint8_t  dsmcc_type = DSMCC_TYPE_DOWNLOAD_MESSAGE;               //!< Indicates type of MPEG-2 DSM-CC message.
            uint16_t message_id = DSMCC_MESSAGE_ID_DDB;                      //!< Indicates type of message which is being passed.
            uint32_t download_id = 0;                                        //!< Used to associate the download data messages and the download control messages of a single instance of a download scenario.

            //!
            //! Default constructor.
            //!
            DownloadDataHeader() = default;

            //!
            //! Clear values.
            //!
            void clear();
        };

        uint16_t           table_id_ext = 0;    //!< Module Id where block belongs.
        DownloadDataHeader header {};           //!< DSM-CC Download Data Header.
        uint16_t           module_id = 0;       //!< Identifies to which module this block belongs.
        uint8_t            module_version = 0;  //!< Identifies the version of the module to which this block belongs.
        ByteBlock          block_data {};       //!< Conveys the data of the block.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        DSMCCDownloadDataMessage(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DSMCCDownloadDataMessage(DuckContext& duck, const BinaryTable& table);

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
        static constexpr size_t DOWNLOAD_DATA_HEADER_SIZE = 12;  //!< DSM-CC Download Data Header size w/o adaptation header.

        static constexpr uint8_t  DSMCC_PROTOCOL_DISCRIMINATOR = 0x11;  //!< Protocol Discriminator for DSM-CC.
        static constexpr uint8_t  DSMCC_TYPE_DOWNLOAD_MESSAGE = 0x03;   //!< MPEG-2 DSM-CC Download Message.
        static constexpr uint16_t DSMCC_MESSAGE_ID_DDB = 0x1003;        //!< DownloadDataMessage.
    };
}  // namespace ts
