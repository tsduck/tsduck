//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Packetization of MPEG sections into Transport Stream packets.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsTSPacket.h"
#include "tsSectionProviderInterface.h"

namespace ts {

    class TSDUCKDLL Packetizer
    {
    public:
        // Constructor.
        Packetizer (PID = PID_NULL, SectionProviderInterface* = 0);

        // Destructor
        virtual ~Packetizer() {}

        // Set/get default PID for subsequent MPEG packets.
        void setPID (PID pid) {_pid = pid & 0x1FFF;}
        PID getPID() const {return _pid;}

        // Set/get the object which provides MPEG sections when the
        // packetizer needs a new section
        void setSectionProvider (SectionProviderInterface* sp) {_provider = sp;}
        SectionProviderInterface* sectionProvider() const {return _provider;}

        // Set/get continuity counter value for next MPEG packet.
        // This counter is automatically incremented at each packet.
        // It is usually never a good idea to change this, except
        // maybe before generating the first packet if the continuity
        // must be preserved with the previous content of the PID.
        void setNextContinuityCounter (uint8_t cc) {_continuity = cc & 0x0F;}
        uint8_t nextContinuityCounter() const {return _continuity;}

        // Check if the packet stream is exactly at a section boundary.
        // Return true if the last returned packet contained
        // the end of a section and no unfinished section.
        bool atSectionBoundary() const {return _next_byte == 0;}

        // Build the next MPEG packet for the list of sections.
        // If there is no section to packetize, generate a null packet on PID_NULL.
        void getNextPacket (TSPacket&);

        // Get the number of generated packets so far
        PacketCounter packetCount() const {return _packet_count;}

        // Get the number of completely packetized sections so far
        SectionCounter sectionCount() const {return _section_out_count;}

        // Reset the content of a packetizer. Becomes empty.
        // If the last returned packet contained an unfinished
        // section, this section will be lost.
        virtual void reset();

        // Display the internal state of the packetizer, mainly for debug
        virtual std::ostream& display (std::ostream&) const;

    private:
        // Private members:
        SectionProviderInterface* _provider;
        PID            _pid;
        uint8_t          _continuity;        // Continuity counter for next packet
        SectionPtr     _section;           // Current section to insert 
        size_t         _next_byte;         // Next byte to insert in current section
        PacketCounter  _packet_count;      // Number of generated packets
        SectionCounter _section_out_count; // Number of output (packetized) sections
        SectionCounter _section_in_count;  // Number of input (provided) sections

        // Inaccessible operations
        Packetizer(const Packetizer&) = delete;
        Packetizer& operator= (const Packetizer&) = delete;
    };
}

// Display the internal state of the packetizer, mainly for debug
inline std::ostream& operator<< (std::ostream& strm, const ts::Packetizer& pzer)
{
    return pzer.display (strm);
}
