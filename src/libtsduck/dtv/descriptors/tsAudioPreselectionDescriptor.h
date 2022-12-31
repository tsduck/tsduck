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
//!  Representation of an audio_preselection_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"
#include "tsVariable.h"

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
            // Public members
            uint8_t            preselection_id;             //!< 5 bits, pre-selection id.
            uint8_t            audio_rendering_indication;  //!< 3 bits, audio rendering indication.
            bool               audio_description;           //!< Contain audio description.
            bool               spoken_subtitles;            //!< Contain spoken subtitles.
            bool               dialogue_enhancement;        //!< Contain dialogue enhancement.
            bool               interactivity_enabled;       //!< Interactivity is enabled.
            UString            ISO_639_language_code;       //!< Empty or 3 characters language code.
            Variable<uint8_t>  message_id;                  //!< Text label, message id.
            ByteBlock          aux_component_tags;          //!< When multi-stream info is present, list of component tags.
            ByteBlock          future_extension;            //!< Future extension.

            //!
            //! Default constructor.
            //!
            PreSelection();
        };

        //!
        //! List of pre-selection entries.
        //!
        typedef std::list<PreSelection> PreSelectionList;

        // AudioPreselectionDescriptor public members:
        PreSelectionList entries;  //!< List of pre-selection entries.

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
