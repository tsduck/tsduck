//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            Point() = default;                      //!< Contructor.
            uint8_t   applicable_temporal_id = 0;   //!< 3 bits.
            uint8_t   num_target_output_views = 0;  //!< 8 bits.
            ByteBlock ES_references {};             //!< List of 6-bit ES references.
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
            Level() = default;              //!< Contructor.
            uint8_t   level_idc = 0;        //!< MVC level.
            PointList operation_points {};  //!< List of operational points.
        };

        //!
        //! List of level entries.
        //!
        typedef std::list<Level> LevelList;

        // MVCOperationPointDescriptor public members:
        uint8_t   profile_idc = 0;           //!< MVC profile.
        bool      constraint_set0 = false;   //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set1 = false;   //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set2 = false;   //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set3 = false;   //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set4 = false;   //!< Defined in H.264, ISO/IEC 14496-10
        bool      constraint_set5 = false;   //!< Defined in H.264, ISO/IEC 14496-10
        uint8_t   AVC_compatible_flags = 0;  //!< 2 bits, defined in H.264, ISO/IEC 14496-10
        LevelList levels {};                 //!< List of level entries.

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
