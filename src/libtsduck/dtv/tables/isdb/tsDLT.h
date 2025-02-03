//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB DownLoad Table (DLT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractTable.h"

namespace ts {
    //!
    //! Representation of an ISDB DownLoad Table (DLT).
    //! @see ARIB STD-B16, 4.4
    //! @ingroup libtsduck table
    //!
    //! Note: the ARIB STD-B16 is only in available in Japanese version and not
    //! generally available for download. The following is a summary of the
    //! structure of the DLT for the purpose of its implementation.
    //!
    //! The DLT indicates the software to be downloaded. PID is operated by the
    //! operator and is indicated by DCT. It is transmitted in part (or all) of
    //! the transport_stream of the network. It is scrambled to ensure security.
    //! The frequency of transmission is set arbitrarily by the operator.
    //!
    //! The DLT is a short section. However, the ARIB standard recreates 16-bit
    //! extended versions of @a section_number and @a last_section_number in the
    //! form of @a Lsection_number and @a last_Lsection_number. A CRC-32 is also
    //! explicitly added at the end of the section (just like a TOT).
    //!
    //! @code
    //! Syntax                           Bits  Identifier
    //! -------------------------------  ----  -------------
    //! Download_section() {
    //!   table_id                          8  uimsbf = 0xC1
    //!   section_syntax_indicator          1  bslbf  = 0
    //!   private_indicator                 1  bslbf  = 1
    //!   reserved                          2  bslbf
    //!   section_length                   12  uimsbf
    //!   maker_id                          8  uimsbf
    //!   model_id                          8  uimsbf
    //!   version_id                        8  uimsbf
    //!   Lsection_number                  16  uimsbf
    //!   last_Lsection_number             16  uimsbf
    //!   for (i=0;i<145;i++) {
    //!     model_info                      8  bslbf
    //!   }
    //!   for (i=0;i<2048;i++) {
    //!     code_data_byte                  8  bslbf
    //!   }
    //!   CRC_32                           32  rpchof
    //! }
    //! @endcode
    //!
    //! maker_id: This 8-bit field indicates the manufacturer identification of
    //! the receiver to which this section applies. This value is managed and
    //! operated by the standardization organization.
    //!
    //! model_id: This 8-bit field indicates the model identification of the
    //! receiver to which this section applies, within the same maker_id.
    //! This value is managed and operated by each manufacturer.
    //!
    //! version_id (software version identification): This 8-bit field indicates
    //! the software version identification of the receiver to which this section
    //! applies, within the same maker_id/model_id. This value is managed and
    //! operated by each manufacturer. Only one version_id is transmitted at
    //! the same time.
    //!
    //! Lsection_number (extended section number): This 16-bit field is an
    //! extension of section_number to 16 bits, and indicates the number of the
    //! extended section. The extended section number of the first section in a
    //! subtable is 0x00. The extended section number is incremented by 1 for each
    //! additional section with the same table identification/manufacturer
    //! identification/model identification/software version identification.
    //!
    //! last_Lsection_number (last extended section number): This 16-bit field
    //! is an extension of last_section_number to 16 bits, and specifies the
    //! number of the last section of the subtable to which the section belongs
    //! (i.e., the section with the highest extended section number).
    //!
    //! model_info: This 145-byte field can contain any information related to
    //! the software.
    //!
    //! code_data_byte (downloaded software): This 2048-byte field indicates the
    //! software to be downloaded.
    //!
    //! CRC_32 (CRC): This 32-bit field is generated according to the CRC generation
    //! method section of the ISO/IEC13818-1 standard. It indicates the CRC value
    //! for the entire section.
    //!
    class TSDUCKDLL DLT : public AbstractTable
    {
    public:
        // DLT public members:
        uint8_t   maker_id = 0;              //!< Manufacturer id.
        uint8_t   model_id = 0;              //!< Model id within manufacturer.
        uint8_t   version_id = 0;            //!< Downloaded software id.
        uint16_t  Lsection_number = 0;       //!< DLT section number (recreated on 16 bits).
        uint16_t  last_Lsection_number = 0;  //!< DLT last section number (recreated on 16 bits).
        ByteBlock model_info {};             //!< Model information. Padded with FF to 145 bytes in binary section.
        ByteBlock code_data {};              //!< Downloaded software fragment. Padded with FF to 2048 bytes in binary section.

        static constexpr size_t MODEL_INFO_SIZE = 145;  //!< Fixed size in bytes of @a model_info in binary section.
        static constexpr size_t CODE_DATA_SIZE = 2048;  //!< Fixed size in bytes of @a code_data in binary section.

        //!
        //! Default constructor.
        //!
        DLT();

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        DLT(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual bool useTrailingCRC32() const override;
        virtual void clearContent() override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
