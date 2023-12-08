//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an audio_preselection_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an audio_preselection_descriptor.
    //! @see ETSI EN 300 468, 6.4.1
    //! @ingroup descriptor
    //!
    class TSDUCKDLL AudioPreselectionDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Pre-selection entry.
        //!
        struct TSDUCKDLL PreSelection
        {
            PreSelection() = default;                               //!< Constructor.
            uint8_t                preselection_id = 0;             //!< 5 bits, pre-selection id.
            uint8_t                audio_rendering_indication = 0;  //!< 3 bits, audio rendering indication.
            bool                   audio_description = false;       //!< Contain audio description.
            bool                   spoken_subtitles = false;        //!< Contain spoken subtitles.
            bool                   dialogue_enhancement = false;    //!< Contain dialogue enhancement.
            bool                   interactivity_enabled = false;   //!< Interactivity is enabled.
            UString                ISO_639_language_code {};        //!< Empty or 3 characters language code.
            std::optional<uint8_t> message_id {};                   //!< Text label, message id.
            ByteBlock              aux_component_tags {};           //!< When multi-stream info is present, list of component tags.
            ByteBlock              future_extension {};             //!< Future extension.
        };

        //!
        //! List of pre-selection entries.
        //!
        typedef std::list<PreSelection> PreSelectionList;

        // AudioPreselectionDescriptor public members:
        PreSelectionList entries {};  //!< List of pre-selection entries.

        //!
        //! Default constructor.
        //!
        AudioPreselectionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        AudioPreselectionDescriptor(DuckContext& duck, const Descriptor& bin);

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

    private:
        // Check if internal data sizes are valid.
        bool hasValidSizes() const;
    };
}
