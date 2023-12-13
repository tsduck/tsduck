//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Program Map Table (PMT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsCodecType.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Representation of a Program Map Table (PMT).
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.4.4.8
    //! @ingroup table
    //!
    class TSDUCKDLL PMT : public AbstractLongTable
    {
    public:
        //!
        //! Description of an elementary stream.
        //!
        //! Note: by inheriting from EntryWithDescriptors, there is a
        //! public field "DescriptorList descs".
        //!
        class TSDUCKDLL Stream : public EntryWithDescriptors
        {
            TS_NO_DEFAULT_CONSTRUCTORS(Stream);
            TS_DEFAULT_ASSIGMENTS(Stream);
        public:
            uint8_t stream_type = 0;  //!< Stream type, one of ST_* (eg ts::ST_MPEG2_VIDEO).

            //!
            //! Constructor.
            //! @param [in] table Parent PMT.
            //! @param [in] type Stream type.
            //!
            explicit Stream(const AbstractTable* table, uint8_t type = 0);

            //!
            //! Check if an elementary stream carries audio.
            //! Does not just look at the stream type.
            //! Also analyzes the descriptor list for additional information.
            //! @param [in] duck TSDuck execution context.
            //! @return True if the elementary stream carries audio.
            //!
            bool isAudio(const DuckContext& duck) const;

            //!
            //! Check if an elementary stream carries video.
            //! Does not just look at the stream type.
            //! Also analyzes the descriptor list for additional information.
            //! @param [in] duck TSDuck execution context.
            //! @return True if the elementary stream carries video.
            //!
            bool isVideo(const DuckContext& duck) const;

            //!
            //! Check if an elementary stream carries subtitles.
            //! Does not just look at the stream type.
            //! Also analyzes the descriptor list for additional information.
            //! @param [in] duck TSDuck execution context.
            //! @return True if the elementary stream carries subtitles.
            //!
            bool isSubtitles(const DuckContext& duck) const;

            //!
            //! Get the PID class of the stream.
            //! Look at the stream type and the descriptor list.
            //! @param [in] duck TSDuck execution context.
            //! @return The PID class (PIDClass::DATA if unknown component type).
            //!
            PIDClass getClass(const DuckContext& duck) const;

            //!
            //! Try to determine the codec which is used in the stream.
            //! Look at the stream type and the descriptor list.
            //! @param [in] duck TSDuck execution context.
            //! @return The codec type (CodecType::UNDEFINED if unknown).
            //!
            CodecType getCodec(const DuckContext& duck) const;

            //!
            //! Look for a component tag in a stream_identifier_descriptor.
            //! @param [out] tag First component tag found, unmodified if none found.
            //! @return True if a component tag was found.
            //!
            bool getComponentTag(uint8_t& tag) const;
        };

        //!
        //! List of elementary streams, indexed by PID.
        //!
        typedef EntryWithDescriptorsMap<PID, Stream> StreamMap;

        // PMT public members:
        uint16_t       service_id;  //!< Service id aka "program_number".
        PID            pcr_pid;     //!< PID for PCR data.
        DescriptorList descs;       //!< Program-level descriptor list.
        StreamMap      streams;     //!< Map of stream descriptions: key=PID, value=stream_description.

        //!
        //! Default constructor.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //! @param [in] service_id Service identifier.
        //! @param [in] pcr_pid PID of the PCR. Default: none.
        //!
        PMT(uint8_t  version = 0,
            bool     is_current = true,
            uint16_t service_id = 0,
            PID      pcr_pid = PID_NULL);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        PMT(const PMT& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        PMT(DuckContext& duck, const BinaryTable& table);

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this object.
        //!
        PMT& operator=(const PMT& other) = default;

        //!
        //! Search the component PID for a given component tag.
        //! @param [in] tag Component tag to search.
        //! @return The PID of the corresponding component or PID_NULL if not found.
        //!
        PID componentTagToPID(uint8_t tag) const;

        //!
        //! Search the first video PID in the service.
        //! @param [in] duck TSDuck execution context.
        //! @return The first video PID or PID_NULL if none is found.
        //!
        PID firstVideoPID(const DuckContext& duck) const;

        //!
        //! Search the first format identifier in a registration descriptor.
        //! When @a pid is specified, the registration descriptor is first search in the corresponding
        //! component-level descriptor list and, if no registration descriptor was found, in the
        //! program-level descriptor list.
        //! @param [in] pid The PID to search or PID_NULL to search in program-level descriptor list only.
        //! @return The first format identifier in a registration descriptor or REGID_NULL if none was found.
        //!
        uint32_t registrationId(PID pid = PID_NULL) const;

        // Inherited methods
        virtual bool isPrivate() const override;
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
