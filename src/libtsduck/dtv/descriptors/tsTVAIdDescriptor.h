//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB-defined TVA_id_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a DVB-defined TVA_id_descriptor.
    //! Note: TVA stands for TV-Anytime.
    //! @see ETSI TS 102 323, 11.2.4.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TVAIdDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! TVA_id entry.
        //!
        struct TSDUCKDLL TVAId
        {
            TVAId() = default;            //!< Constructor.
            uint16_t TVA_id = 0;          //!< TV-Anytime id.
            uint8_t  running_status = 0;  //!< 3-bit running status.
        };

        //!
        //! List of TVA_id entries.
        //!
        typedef std::list<TVAId> TVAIdList;

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 85;

        // TVAIdDescriptor public members:
        TVAIdList TVA_ids {};  //!< The list of TVA_id entries.

        //!
        //! Default constructor.
        //!
        TVAIdDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TVAIdDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
