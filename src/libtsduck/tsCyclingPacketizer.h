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
//  Cyclic packetization of MPEG sections into Transport Stream packets.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPacketizer.h"
#include "tsSectionProviderInterface.h"
#include "tsBinaryTable.h"
#include "tsAbstractTable.h"

namespace ts {

    // A CyclingPacketizer contains various sections to be packetized on one PID.
    //
    // All packets are generated on demand. The generated packets have
    // the right PID and continuity counters and can be directly injected
    // in a transport stream.
    //
    // The "cycle" of the packetizer is defined as the smallest set of TS
    // packets containing all sections, with respect to the broadcasting
    // constraints (stuffing, specific repetition rates, etc).
    //
    // It is possible to set different repetition rates for sections.
    // In that case, the target "bitrate" of the PID must be specified.
    // The sections are inserted on a best effort basis to respect the
    // minimum repetition rates.
    //
    // When the packetizer bitrate is specified as zero (the default), the
    // target bitrate of the PID is unspecified. The repetition rates of
    // sections are ignored.
    //
    // Note that when sections have different repetition rates, some
    // sections may be repeated into one cycle of the Packetizer.
    //
    // Section stuffing may occur at the end of a section. If the section
    // ends in the middle of an MPEG packet, the beginning of the next section
    // can start immediately or can be delayed to the beginning of the next
    // packet. In the later case, the rest of the current packet is filled
    // with stuffing bytes (0xFF).
    //
    // A bitrate is specified in bits/second. Zero means undefined.
    // A repetition rate is specified in milliseconds. Zero means undefined.

    class TSDUCKDLL CyclingPacketizer: public Packetizer, private SectionProviderInterface
    {
    public:
        // The following options specifies where stuffing applies.
        enum StuffingPolicy {
            NEVER,  // No stuffing, always pack sections.
            AT_END, // Stuffing at end of cycle, pack sections inside cycle.
            ALWAYS  // Always stuffing, never pack sections
        };

        // Constructor
        CyclingPacketizer (PID = PID_NULL, StuffingPolicy = AT_END, BitRate = 0);

        // Destructor
        virtual ~CyclingPacketizer();

        // Set/get the stuffing policy.
        void setStuffingPolicy (StuffingPolicy sp) {_stuffing = sp;}
        StuffingPolicy stuffingPolicy() const {return _stuffing;}

        // Set/get the bitrate of the generated PID.
        // Useful only when using specific repetition rates for sections
        void setBitRate (BitRate);
        BitRate bitRate() const {return _bitrate;}

        // Add one or more sections into the packetizer.
        // The contents of the sections are shared.
        void addSection (const SectionPtr&, MilliSecond repetition_rate = 0);
        void addSections (const SectionPtrVector&, MilliSecond repetition_rate = 0);

        // Add all sections of a table into the packetizer.
        void addTable (const BinaryTable&, MilliSecond repetition_rate = 0);
        void addTable (const AbstractTable&, MilliSecond repetition_rate = 0);

        // Remove all sections with the specified table id.
        // If one such section is currently being packetized, the rest of
        // the section will be packetized.
        void removeSections (TID);

        // Remove all sections with the specified table id and table id extension.
        // If one such section is currently being packetized, the rest of
        // the section will be packetized.
        void removeSections (TID, uint16_t);

        // Remove all sections in the packetizer.
        // If a section is currently being packetized, the rest of
        // the section will be packetized.
        void removeAll ();

        // Reset the content of a packetizer. Becomes empty.
        // If the last returned packet contained an unfinished section,
        // this section will be lost. Inherited from Packetizer.
        virtual void reset();

        // Get the number of stored sections to packetize
        SectionCounter storedSectionCount() const {return _section_count;}
        
        // Return true when the last generated packet was the last packet in
        // the cycle. Note that if the stuffing policy is NEVER, this
        // is not reliable since it is unlikely that a packet actually
        // terminates a cycle.
        bool atCycleBoundary() const;

        // Display the internal state of the packetizer, mainly for debug.
        // Inherited from Packetizer.
        virtual std::ostream& display (std::ostream&) const;

    private:
        // Each section is identified by a SectionDesc instance
        class SectionDesc
        {
        public:
            // Public fields
            SectionPtr     section;     // Pointer to section
            MilliSecond    repetition;  // Repetition rate, zero if none
            PacketCounter  last_packet; // Packet index of last time the section was sent
            PacketCounter  due_packet;  // Packet index of next time
            SectionCounter last_cycle;  // Cycle index of last time the section was sent

            // Constructor
            SectionDesc (const SectionPtr& sec, MilliSecond rep) :
                section (sec), repetition (rep), last_packet (0), due_packet (0), last_cycle (0)
            {
            }

            // Display the internal state, mainly for debug.
            std::ostream& display (std::ostream&) const;
        };

        // Safe pointer for SectionDesc (not thread-safe)
        typedef SafePtr <SectionDesc, NullMutex> SectionDescPtr;

        // List of sections
        typedef std::list <SectionDescPtr> SectionDescList;

        // Private members:
        StuffingPolicy  _stuffing;
        BitRate         _bitrate;
        size_t          _section_count;   // Number of sections in the 2 lists
        SectionDescList _sched_sections;  // Scheduled sections, with repetition rates
        SectionDescList _other_sections;  // Unscheduled sections
        PacketCounter   _sched_packets;   // Size in TS packets of all sections in _sched_sections
        SectionCounter  _current_cycle;   // Cycle number (start at 1, always increasing)
        size_t          _remain_in_cycle; // Number of unsent sections in this cycle
        SectionCounter  _cycle_end;       // At end of cycle, contains the index of last section

        static const SectionCounter UNDEFINED = ~SectionCounter(0);

        // Insert a scheduled section in the list, sorted by due_packet,
        // after other sections with the same due_packet.
        void addScheduledSection (const SectionDescPtr&);

        // Remove all sections with the specified tid/tid_ext in the specified list.
        void removeSections (SectionDescList&, TID, uint16_t tid_ext, bool use_tid_ext, bool scheduled);

        // Inherited from SectionProviderInterface
        virtual void provideSection (SectionCounter, SectionPtr&);
        virtual bool doStuffing();

        // Hide this method, we do not want the section provider to be replaced
        void setSectionProvider (SectionProviderInterface*);
    };
}
