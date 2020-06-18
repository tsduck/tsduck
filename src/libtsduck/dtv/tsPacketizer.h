//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsMPEG.h"
#include "tsSectionProviderInterface.h"
#include "tsReport.h"

namespace ts {

    class TSPacket;
    class DuckContext;

    //!
    //! Packetization of MPEG sections into Transport Stream packets.
    //! @ingroup mpeg
    //!
    //! Sections are provided by an object implementing SectionProviderInterface.
    //!
    class TSDUCKDLL Packetizer
    {
        TS_NOBUILD_NOCOPY(Packetizer);
    public:
        //!
        //! Constructor.
        //! @param [in] duck TSDuck execution context. The reference is kept inside the packetizer.
        //! @param [in] pid PID for generated TS packets.
        //! @param [in] provider An object which will be called each time a section is required.
        //! @param [in] report Optional address of a Report object for debug and trace messages.
        //!
        Packetizer(const DuckContext& duck, PID pid = PID_NULL, SectionProviderInterface* provider = nullptr, Report* report = nullptr);

        //!
        //! Destructor
        //!
        virtual ~Packetizer();

        //!
        //! Set the default PID for subsequent MPEG packets.
        //! @param [in] pid PID for generated TS packets.
        //!
        void setPID(PID pid) { _pid = pid & 0x1FFF; }

        //!
        //! Get the default PID for subsequent MPEG packets.
        //! @return PID for generated TS packets.
        //!
        PID getPID() const { return _pid; }

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
        //! Set the continuity counter value for next MPEG packet.
        //! This counter is automatically incremented at each packet.
        //! It is usually never a good idea to change this, except
        //! maybe before generating the first packet if the continuity
        //! must be preserved with the previous content of the PID.
        //! @param [in] cc Next continuity counter.
        //!
        void setNextContinuityCounter(uint8_t cc) { _continuity = cc & 0x0F; }

        //!
        //! Get the continuity counter value for next MPEG packet.
        //! @return Next continuity counter.
        //!
        uint8_t nextContinuityCounter() const { return _continuity; }

        //!
        //! Check if the packet stream is exactly at a section boundary.
        //! @return True if the last returned packet contained
        //! the end of a section and no unfinished section.
        //!
        bool atSectionBoundary() const { return _next_byte == 0; }

        //!
        //! Build the next MPEG packet for the list of sections.
        //! If there is no section to packetize, generate a null packet on PID_NULL.
        //! @param [out] packet The next TS packet.
        //! @return True if a real packet is returned, false if a null packet was returned.
        //!
        bool getNextPacket(TSPacket& packet);

        //!
        //! Get the number of generated packets so far.
        //! @return The number of generated packets so far.
        //!
        PacketCounter packetCount() const { return _packet_count; }

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

        //!
        //! Get a reference to the debugging report.
        //! @return A reference to the debugging report.
        //!
        Report& report() const { return _report; }

        //!
        //! Reset the content of a packetizer.
        //! The packetizer becomes empty.
        //! If the last returned packet contained an unfinished section, this section will be lost.
        //!
        virtual void reset();

        //!
        //! Display the internal state of the packetizer, mainly for debug.
        //! @param [in,out] strm Output text stream.
        //! @return A reference to @a strm.
        //!
        virtual std::ostream& display(std::ostream& strm) const;

    protected:
        // Protected directly accessible to subclasses.
        const DuckContext& _duck;  //!< The TSDuck execution context is accessible to all subclasses.

    private:
        SectionProviderInterface* _provider;
        Report&        _report;            // Report object for debug.
        PID            _pid;               // PID for injected sections.
        bool           _split_headers;     // Allowed to split section header beetwen TS packets.
        uint8_t        _continuity;        // Continuity counter for next packet
        SectionPtr     _section;           // Current section to insert
        size_t         _next_byte;         // Next byte to insert in current section
        PacketCounter  _packet_count;      // Number of generated packets
        SectionCounter _section_out_count; // Number of output (packetized) sections
        SectionCounter _section_in_count;  // Number of input (provided) sections
    };
}

//!
//! Display the internal state of a packetizer, mainly for debug.
//! @param [in,out] strm Output text stream.
//! @param [in] pzer A packetizer to display.
//! @return A reference to @a strm.
//!
inline std::ostream& operator<<(std::ostream& strm, const ts::Packetizer& pzer)
{
    return pzer.display(strm);
}
