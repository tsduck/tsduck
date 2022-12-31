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
//!  Representation of an SCTE 35 SpliceInsert command.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractSignalization.h"
#include "tsSCTE35.h"

namespace ts {
    //!
    //! Representation of an SCTE 35 SpliceInsert command.
    //!
    //! @see ANSI/SCTE 35, 9.3.3.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL SpliceInsert : public AbstractSignalization
    {
    public:
        //!
        //! A map of 64-bit PTS time values, indexed by 8-bit component tags.
        //! Used when the program is splice component by component, not as a whole.
        //!
        typedef std::map<uint8_t, SpliceTime> SpliceByComponent;

        //!
        //! An invalid value for event id, can be used as place-holder.
        //!
        static constexpr uint32_t INVALID_EVENT_ID = 0xFFFFFFFF;

        // Public members, derived from SCTE 35 standard.
        uint32_t          event_id;        //!< Splice event id.
        bool              canceled;        //!< When true, event is canceled, other fields are ignored.
        bool              splice_out;      //!< When true, this is a "splice out" event, "splice in" otherwise.
        bool              immediate;       //!< When true, should splice asap, time fields are ignored.
        bool              program_splice;  //!< When true, all components are spliced.
        bool              use_duration;    //!< When true, the duration of the splice out / splice in sequence is given.
        SpliceTime        program_pts;     //!< PTS time value of the event (valid if !canceled && program_splice && !immediate).
        SpliceByComponent components_pts;  //!< PTS time value of the event by component (valid if !canceled && !program_splice && !immediate).
        uint64_t          duration_pts;    //!< Duration of the splice out / splice in sequence (valid if !canceled && use_duration).
        bool              auto_return;     //!< When true, there won't be an explicit "splice in" event, use duration_pts (valid if !canceled && use_duration).
        uint16_t          program_id;      //!< Unique program id.
        uint8_t           avail_num;       //!< Identification for a specific avail within one program_id.
        uint8_t           avails_expected; //!< Expected number of individual avails within the current viewing event.

        //!
        //! Default constructor.
        //!
        SpliceInsert();

        //!
        //! Adjust PTS time values using the "PTS adjustment" field from a splice information section.
        //! @param adjustment PTS adjustment value.
        //!
        void adjustPTS(uint64_t adjustment);

        //!
        //! Get the highest PTS value in the command.
        //! @return The highest PTS value in the command or INVALID_PTS if none found.
        //!
        uint64_t highestPTS() const;

        //!
        //! Get the lowest PTS value in the command.
        //! @return The lowest PTS value in the command or INVALID_PTS if none found.
        //!
        uint64_t lowestPTS() const;

        //!
        //! Display the splice insert command.
        //! @param [in,out] display Display engine.
        //! @param [in] margin Left margin content.
        //!
        void display(TablesDisplay& display, const UString& margin) const;

        //!
        //! Deserialize a SpliceInsert command from binary data.
        //! @param [in] data Address of data to deserialize.
        //! @param [in] size Size of data buffer, possibly larger than the SpliceInsert command.
        //! @return Deserialized size, -1 on incorrect data.
        //!
        int deserialize(const uint8_t* data, size_t size);

        //!
        //! Serialize the SpliceInsert command.
        //! @param [in,out] data The SpliceInsert command is serialized at the end of this byte block.
        //!
        void serialize(ByteBlock& data) const;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
