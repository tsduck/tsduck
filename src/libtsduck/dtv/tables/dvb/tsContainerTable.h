//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB Container Table (TV-Anytime).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"

namespace ts {
    //!
    //! Representation of DVB Container Table (TV-Anytime).
    //! @see ETSI TS 102 323, 7.3.1.4
    //! @ingroup libtsduck table
    //!
    class TSDUCKDLL ContainerTable : public AbstractLongTable
    {
    public:
        // ContainerTable public members:
        uint16_t  container_id = 0;        //!< Container id.
        ByteBlock compression_wrapper {};  //!< Complete compression_wrapper (see ETSI TS 102 323, 7.3.1.5).

        //!
        //! Maximum size of container_data per section.
        //!
        static constexpr size_t MAX_CONTAINER_DATA = MAX_PRIVATE_SECTION_SIZE - LONG_SECTION_HEADER_SIZE - SECTION_CRC32_SIZE;

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //!
        ContainerTable(uint8_t version = 0, bool is_current = true);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        ContainerTable(DuckContext& duck, const BinaryTable& table);

        //!
        //! Extract the container binary data block from the compression wrapper.
        //! @param [out] container Extracted container.
        //! @return True on success, false on error (invalid compression).
        //!
        bool getContainer(ByteBlock& container) const;

        //!
        //! Store the container binary data block into the compression wrapper.
        //! @param [in] container Container data block.
        //! @param [in] compress If true, the data is zlib-compressed.
        //! @return True on success, false on error (invalid compression).
        //!
        bool setContainer(const ByteBlock& container, bool compress);

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
