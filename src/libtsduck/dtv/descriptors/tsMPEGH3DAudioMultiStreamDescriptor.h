//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG-defined MPEGH_3D_audio_multi_stream_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined MPEGH_3D_audio_multi_stream_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.114.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MPEGH3DAudioMultiStreamDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Definition of an mae_group (as described in ISO/IEC 23008-3, section 15).
        //!
        class TSDUCKDLL Group
        {
        public:
            Group() = default;                  //!< Contructor.
            uint8_t mae_group_id = 0;           //!< 7 bits.
            bool    is_in_main_stream = false;  //!< Audio data in this group is present in the main stream.
            bool    is_in_ts = false;           //!< When is_in_main_stream == false.
            uint8_t auxiliary_stream_id = 0;    //!< 7 bits. When is_in_main_stream == false.
        };

        //!
        //! A list of mae_group (ISO/IEC 23008-3).
        //!
        typedef std::list<Group> GroupList;

        // MPEGH3DAudioMultiStreamDescriptor public members:
        bool      this_is_main_stream = false;  //!< The stream is a main stream, not an auxiliary stream.
        uint8_t   this_stream_id = 0;           //!< 7 bits.
        uint8_t   num_auxiliary_streams = 0;    //!< 7 bits. When this_is_main_stream == true.
        GroupList mae_groups {};                //!< When this_is_main_stream == true.
        ByteBlock reserved {};                  //!< Reserved data.

        //!
        //! Default constructor.
        //!
        MPEGH3DAudioMultiStreamDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MPEGH3DAudioMultiStreamDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
