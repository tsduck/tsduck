//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an MPEG-defined virtual_segmentation_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined virtual_segmentation_descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.120.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL VirtualSegmentationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Partition entry.
        //!
        class TSDUCKDLL Partition
        {
        public:
            Partition() = default;                        //!< Constructor.
            uint8_t                 partition_id = 0;     //!< 3 bits
            uint8_t                 SAP_type_max = 0;     //!< 3 bits
            std::optional<PID>      boundary_PID {};      //!< Optional PID.
            std::optional<uint32_t> maximum_duration {};  //!< Up to 29 bits, unit is tick (see ticks_per_second).
        };

        //!
        //! List of partition entries.
        //!
        typedef std::list<Partition> PartitionList;

        //!
        //! Maximum number of partitions (count must fit on 3 bits).
        //!
        static constexpr size_t MAX_PARTITION = 3;

        // VirtualSegmentationDescriptor public members:
        std::optional<uint32_t> ticks_per_second {};  //!< 21 bits, optional number of ticks per second
        PartitionList           partitions {};        //!< List of partitions.

        //!
        //! Default constructor.
        //!
        VirtualSegmentationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        VirtualSegmentationDescriptor(DuckContext& duck, const Descriptor& bin);

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
