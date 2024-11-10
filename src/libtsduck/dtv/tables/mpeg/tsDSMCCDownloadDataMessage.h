//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
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
    //! @see ISO/IEC 13818-6, ITU-T Rec. 9.2.2 and 9.2.7. ETSI TR 101 202 V1.2.1 (2003-01), A.2, A.5, B.
    //! @ingroup table
    //!
    class TSDUCKDLL DSMCCDownloadDataMessage: public AbstractLongTable {
    public:
        // DSMCCDownloadDataMessage public members:
        uint8_t   protocol_discriminator = 0x11;  //!< Indicates that the message is MPEG-2 DSM-CC message.
        uint8_t   dsmcc_type = 0x03;              //!< Indicates type of MPEG-2 DSM-CC message.
        uint16_t  message_id = 0;                 //!< Indicates type of message which is being passed.
        uint32_t  download_id = 0;                //!< Used to associate the download data messages and the download control messages of a single instance of a download scenario.
        uint16_t  module_id = 0;                  //!< Identifies to which module this block belongs.
        uint8_t   module_version = 0;             //!< Identifies the version of the module to which this block belongs.
        uint16_t  block_number = 0;               //!< Identifies the position of this block within the module.
        ByteBlock block_data {};                  //!< Conveys the data of the block.

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
