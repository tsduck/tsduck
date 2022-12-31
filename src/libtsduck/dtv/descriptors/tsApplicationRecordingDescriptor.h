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
//!  Representation of an application_recording_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an application_recording_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 809, 5.3.5.4.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ApplicationRecordingDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Description of a recording label.
        //!
        class TSDUCKDLL RecodingLabel
        {
        public:
            UString label;               //!< Label.
            uint8_t storage_properties;  //!< Storage properties.

            //!
            //! Constructor.
            //! @param [in] l Label.
            //! @param [in] p Storage properties.
            //!
            RecodingLabel(const UString& l = UString(), uint8_t p = 0) : label(l), storage_properties(p) {}
        };

        //!
        //! List of recording labels.
        //!
        typedef std::list<RecodingLabel> RecodingLabelList;

        // ApplicationRecordingDescriptor public members:
        bool      scheduled_recording;  //!< Accept scheduled recording.
        bool      trick_mode_aware;     //!< Accept trick modes.
        bool      time_shift;           //!< Accept time shift.
        bool      dynamic;              //!< Relies on dynamic broadcast data.
        bool      av_synced;            //!< Require streams events.
        bool      initiating_replay;    //!< Replay is started by application.
        RecodingLabelList labels;       //!< List of recording labels.
        ByteBlock component_tags;       //!< List of component tags (one byte each).
        ByteBlock private_data;         //!< Private data.
        ByteBlock reserved_future_use;  //!< Reserved.

        //!
        //! Default constructor.
        //!
        ApplicationRecordingDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ApplicationRecordingDescriptor(DuckContext& duck, const Descriptor& bin);

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
