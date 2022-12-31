//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MVC_operation_point_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an MVC_operation_point_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.82.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL MVCOperationPointDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Operation point entry.
        //!
        struct TSDUCKDLL Point
        {
            Point();                            //!< Contructor.
            uint8_t   applicable_temporal_id;   //!< 3 bits.
            uint8_t   num_target_output_views;  //!< 8 bits.
            ByteBlock ES_references;            //!< List of 6-bit ES references.
        };

        //!
        //! List of operation point entries.
        //!
        typedef std::list<Point> PointList;

        //!
        //! Level entry.
        //!
        struct TSDUCKDLL Level
        {
            Level();                     //!< Contructor.
            uint8_t   level_idc;         //!< MVC level.
            PointList operation_points;  //!< List of operational points.
        };

        //!
        //! List of level entries.
        //!
        typedef std::list<Level> LevelList;

        // MVCOperationPointDescriptor public members:
        uint8_t   profile_idc;           //!< MVC profile.
        bool      constraint_set0;       //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set1;       //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set2;       //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set3;       //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set4;       //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set5;       //!< Defined in H.264, ISO/IEC 14496-10
        uint8_t   AVC_compatible_flags;  //!< 2 bits, defined in H.264, ISO/IEC 14496-10
        LevelList levels;                //!< List of level entries.

        //!
        //! Default constructor.
        //!
        MVCOperationPointDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MVCOperationPointDescriptor(DuckContext& duck, const Descriptor& bin);

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
