//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB Download Control Table (DCT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"

namespace ts {
    //!
    //! Representation of an ISDB Download Control Table (DCT).
    //! @see ARIB STD-B16, 4.3
    //! @ingroup table
    //!
    //! Note: the ARIB STD-B16 is only in available in Japanese version and not
    //! generally available for download. The following is a summary of the
    //! structure of the DCT for the purpose of its implementation.
    //!
    //! The DCT indicates various information for separating and extracting DLT.
    //! PID is 0x0017. It is transmitted on all transport_streams of the network.
    //! The transmission frequency is at least once per second.
    //!
    //! @code
    //! Syntax                           Bits  Identifier
    //! -------------------------------  ----  -------------
    //! Download_control_section() {
    //!   table_id                          8  uimsbf = 0xC0
    //!   section_syntax_indicator          1  bslbf  = 1
    //!   private_indicator                 1  bslbf  = 1
    //!   reserved                          2  bslbf
    //!   section_length                   12  uimsbf
    //!   network_id                       16  uimsbf
    //!   reserved                          2  bslbf
    //!   version_number                    5  uimsbf
    //!   current_next_indicator            1  bslbf
    //!   section_number                    8  uimsbf
    //!   last_section_number               8  uimsbf
    //!   transmission_rate                 8  uimsbf
    //!   for (i=0;i<N;i++) {
    //!     transport_stream_id            16  uimsbf
    //!     reserved                        3  bslbf
    //!     DL_PID                         13  uimsbf
    //!     reserved                        3  bslbf
    //!     ECM_PID                        13  uimsbf
    //!     reserved                        4  bslbf
    //!     model_info_length              12  uimsbf
    //!     for (j=0;j<M;j++) {
    //!       maker_id                      8  uimsbf
    //!       model_id                      8  uimsbf
    //!       version_id                    8  uimsbf
    //!       DLT_size                      8  uimsbf
    //!     }
    //!   }
    //!   CRC_32                           32  rpchof
    //! }
    //! @endcode
    //!
    //! network_id: This 16-bit field indicates the network to which the switch
    //! is to be made. If it is the same as the network that transmits the DLT,
    //! it means that it is used for the purpose of improving functions and
    //! fixing bugs.
    //!
    //! transmission_rate: This 8-bit field indicates the transmission rate for
    //! each DLT sub-table (each device). It indicates the number of transport
    //! packets sent per second.
    //!
    //! transport_stream_id: For each transport stream represented by this 16-bit
    //! field, this indicates an overview of the DLT transmitted there.
    //!
    //! DL_PID: This 13-bit field indicates the PID of the DLT. A different PID
    //! is assigned for each destination network identification.
    //!
    //! ECM_PID: This 13-bit field indicates the PID of the ECM corresponding to
    //! that DLT.
    //!
    //! model_info_length: This 12-bit field specifies the total number of bytes
    //! in the model information loop that follows.
    //!
    //! maker_id: This 8-bit field indicates the manufacturer identification of
    //! the receiver to which the DLT applies. This value is managed and operated
    //! by the standardization organization.
    //!
    //! model_id: This 8-bit field indicates the model identification, within the
    //! same maker_id, of the receiver to which the DLT applies. This value is
    //! managed and operated by each manufacturer.
    //!
    //! version_id (software version identification): This 8-bit field indicates
    //! the software version identification, within the same maker_id/model_id,
    //! of the receiver to which the DLT applies. This value is managed and
    //! operated by each manufacturer. Only one version_id is transmitted at the
    //! same time.
    //!
    //! DLT_size (DLT size): This 8-bit field indicates the number of sections
    //! in the DLT with the same maker_id/model_id/version_id. It takes the same
    //! value as the middle 8 bits of last_Lsection_number in the DLT.
    //!
    class TSDUCKDLL DCT : public AbstractLongTable
    {
    public:
        //!
        //! Identification of a receiver model and software.
        //!
        class TSDUCKDLL ModelInfo
        {
        public:
            ModelInfo() = default;      //!< Default constructor.
            uint8_t maker_id = 0;       //!< Manufacturer id.
            uint8_t model_id = 0;       //!< Model id within manufacturer.
            uint8_t version_id = 0;     //!< Downloaded software id.
            uint8_t DLT_size = 0;       //!< Number of sections in the DLT with the same maker_id / model_id / version_id.
        };

        //!
        //! Identification of all DLT in a transport stream.
        //!
        class TSDUCKDLL StreamInfo
        {
        public:
            StreamInfo() = default;                        //!< Default constructor.
            uint16_t             transport_stream_id = 0;  //!< Transport stream id.
            PID                  DL_PID = PID_NULL;        //!< Download PID containing DLT.
            PID                  ECM_PID = PID_NULL;       //!< PID containing ECM's for DL_PID scrambling.
            std::list<ModelInfo> models {};                //!< List of downloaded software.
        };

        // DCT public members:
        uint16_t              network_id = 0;         //!< Target network id.
        uint8_t               transmission_rate = 0;  //!< Transmission rate in TS packets per second.
        std::list<StreamInfo> streams {};             //!< Description of all transport streams.

        //!
        //! Default constructor.
        //! @param [in] vers Table version number.
        //! @param [in] cur True if table is current, false if table is next.
        //!
        DCT(uint8_t vers = 0, bool cur = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DCT(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
