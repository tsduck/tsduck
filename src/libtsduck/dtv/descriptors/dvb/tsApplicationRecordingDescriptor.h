//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            UString label {};                //!< Label.
            uint8_t storage_properties = 0;  //!< Storage properties.

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
        bool      scheduled_recording = false;  //!< Accept scheduled recording.
        bool      trick_mode_aware = false;     //!< Accept trick modes.
        bool      time_shift = false;           //!< Accept time shift.
        bool      dynamic = false;              //!< Relies on dynamic broadcast data.
        bool      av_synced = false;            //!< Require streams events.
        bool      initiating_replay = false;    //!< Replay is started by application.
        RecodingLabelList labels {};            //!< List of recording labels.
        ByteBlock component_tags {};            //!< List of component tags (one byte each).
        ByteBlock private_data {};              //!< Private data.
        ByteBlock reserved_future_use {};       //!< Reserved.

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
