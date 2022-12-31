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
//!  Packetization of MPEG sections into Transport Stream packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractPacketizer.h"
#include "tsSectionProviderInterface.h"

namespace ts {
    //!
    //! Packetization of MPEG sections into Transport Stream packets.
    //! @ingroup mpeg
    //!
    //! Sections are provided by an object implementing SectionProviderInterface.
    //!
    class TSDUCKDLL Packetizer: public AbstractPacketizer
    {
        TS_NOBUILD_NOCOPY(Packetizer);
    public:
        //!
        //! Constructor.
        //! @param [in] duck TSDuck execution context. The reference is kept inside the packetizer.
        //! @param [in] pid PID for generated TS packets.
        //! @param [in] provider An object which will be called each time a section is required.
        //!
        Packetizer(const DuckContext& duck, PID pid = PID_NULL, SectionProviderInterface* provider = nullptr);

        //!
        //! Destructor
        //!
        virtual ~Packetizer() override;

        //!
        //! Set the object which provides MPEG sections when the packetizer needs a new section.
        //! @param [in] provider An object which will be called each time a section is required.
        //!
        void setSectionProvider(SectionProviderInterface* provider) { _provider = provider; }

        //!
        //! Get the object which provides MPEG sections when the packetizer needs a new section.
        //! @return The object which will be called each time a section is required.
        //!
        SectionProviderInterface* sectionProvider() const { return _provider; }

        //!
        //! Check if the packet stream is exactly at a section boundary.
        //! @return True if the last returned packet contained
        //! the end of a section and no unfinished section.
        //!
        bool atSectionBoundary() const { return _next_byte == 0; }

        //!
        //! Get the number of completely packetized sections so far.
        //! @return The number of completely packetized sections so far.
        //!
        SectionCounter sectionCount() const { return _section_out_count; }

        //!
        //! Allow or disallow splitting section headers across TS packets.
        //! By default, a Packetizer never splits a section header between two TS packets.
        //! This is not required by the MPEG standard but some STB are known to have problems with that.
        //! @param [in] allow If true, splitting section headers across TS packets is allowed.
        //!
        void allowHeaderSplit(bool allow) { _split_headers = allow; }

        //!
        //! Check if splitting section headers across TS packets is allowed.
        //! @return True if splitting section headers across TS packets is allowed.
        //!
        bool headerSplitAllowed() const { return _split_headers; }

        // Inherited methods.
        virtual void reset() override;
        virtual bool getNextPacket(TSPacket& packet) override;
        virtual std::ostream& display(std::ostream& strm) const override;

    private:
        SectionProviderInterface* _provider;
        bool           _split_headers;     // Allowed to split section header beetwen TS packets.
        SectionPtr     _section;           // Current section to insert
        size_t         _next_byte;         // Next byte to insert in current section
        SectionCounter _section_out_count; // Number of output (packetized) sections
        SectionCounter _section_in_count;  // Number of input (provided) sections
    };
}
