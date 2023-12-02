//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB target_region_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB target_region_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.27
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ISDBTargetRegionDescriptor : public AbstractDescriptor
    {
        class PrefectureMap {
            TS_DEFAULT_COPY_MOVE(PrefectureMap);

        private:
            static const size_t MAX_PREFECTURES = 56;
        public:
            std::array<bool, MAX_PREFECTURES> prefectures {};
            //!
            //! Default constructor.
            //!
            PrefectureMap() { clear(); }
            //!
            //! Read-in constructor.
            //! @param [in,out] buf Deserialization buffer. Read the descriptor payload from
            //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
            //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
            //!
            PrefectureMap(PSIBuffer& buf) : PrefectureMap() { deserialize(buf); }

            //!
            //! toString
            //! @returns a string depiction of the prefectures included in the target region in a bitmap form where each represented by a 1 or 0 character.
            //! 
            ts::UString toString() const;

            //! @cond nodoxygen
            void clear();
            void serialize(PSIBuffer&) const;
            void deserialize(PSIBuffer&);
            void toXML(xml::Element*) const;
            bool fromXML(const xml::Element*);
            void display(TablesDisplay&, PSIBuffer&, const UString&);
            //! @endcond
        };

    public:
        // ISDBTargetRegionDescriptor public members:
        uint8_t                      region_spec_type = 0;
        std::optional<PrefectureMap> target_region_mask {};

        //!
        //! Default constructor.
        //!
        ISDBTargetRegionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISDBTargetRegionDescriptor(DuckContext& duck, const Descriptor& bin);

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
