//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cyclic packetization of MPEG sections into Transport Stream packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPacketizer.h"
#include "tsSectionProviderInterface.h"
#include "tsBinaryTable.h"
#include "tsAbstractTable.h"

namespace ts {
    //!
    //! Cyclic packetization of MPEG sections into Transport Stream packets.
    //! @ingroup mpeg
    //!
    //! A CyclingPacketizer contains various sections to be packetized on one PID.
    //!
    //! All packets are generated on demand. The generated packets have
    //! the right PID and continuity counters and can be directly injected
    //! in a transport stream.
    //!
    //! The "cycle" of the packetizer is defined as the smallest set of TS
    //! packets containing all sections, with respect to the broadcasting
    //! constraints (stuffing, specific repetition rates, etc).
    //!
    //! It is possible to set different repetition rates for sections.
    //! In that case, the target "bitrate" of the PID must be specified.
    //! The sections are inserted on a best effort basis to respect the
    //! minimum repetition rates.
    //!
    //! When the packetizer bitrate is specified as zero (the default), the
    //! target bitrate of the PID is unspecified. The repetition rates of
    //! sections are ignored.
    //!
    //! Note that when sections have different repetition rates, some
    //! sections may be repeated into one cycle of the Packetizer.
    //!
    //! Section stuffing may occur at the end of a section. If the section
    //! ends in the middle of an MPEG packet, the beginning of the next section
    //! can start immediately or can be delayed to the beginning of the next
    //! packet. In the later case, the rest of the current packet is filled
    //! with stuffing bytes (0xFF).
    //!
    //! A bitrate is specified in bits/second. Zero means undefined.
    //! A repetition rate is specified in milliseconds. Zero means undefined.
    //!
    class TSDUCKDLL CyclingPacketizer: public Packetizer, private SectionProviderInterface
    {
        TS_NOBUILD_NOCOPY(CyclingPacketizer);
    public:
        //!
        //! Specify where stuffing applies.
        //!
        enum class StuffingPolicy {
            NEVER,  //!< No stuffing, always pack sections.
            AT_END, //!< Stuffing at end of cycle, pack sections inside cycle.
            ALWAYS  //!< Always stuffing, never pack sections.
        };

        //!
        //! Default constructor.
        //! @param [in] duck TSDuck execution context. The reference is kept inside the packetizer.
        //! @param [in] pid PID for generated TS packets.
        //! @param [in] policy TS packet stuffing policy at end of packet.
        //! @param [in] bitrate Output bitrate, zero if undefined.
        //! Useful only when using specific repetition rates for sections
        //!
        CyclingPacketizer(const DuckContext& duck, PID pid = PID_NULL, StuffingPolicy policy = StuffingPolicy::AT_END, const BitRate& bitrate = 0);

        //!
        //! Destructor
        //!
        virtual ~CyclingPacketizer() override;

        //!
        //! Set the TS packet stuffing policy at end of packet.
        //! @param [in] sp TS packet stuffing policy at end of packet.
        //!
        void setStuffingPolicy(StuffingPolicy sp) { _stuffing = sp; }

        //!
        //! Get the TS packet stuffing policy at end of packet.
        //! @return TS packet stuffing policy at end of packet.
        //!
        StuffingPolicy stuffingPolicy() const {return _stuffing;}

        //!
        //! Set the bitrate of the generated PID.
        //! Useful only when using specific repetition rates for sections
        //! @param [in] bitrate Output bitrate, zero if undefined.
        //!
        void setBitRate(const BitRate& bitrate);

        //!
        //! Get the bitrate of the generated PID.
        //! @return Output bitrate, zero if undefined.
        //!
        BitRate bitRate() const { return _bitrate; }

        //!
        //! Add one section into the packetizer.
        //! The contents of the sections are shared.
        //! @param [in] section A smart pointer to the section to packetize.
        //! @param [in] repetition_rate Repetition rate of the section in milliseconds.
        //! If zero, simply packetize sections one after the other.
        //!
        void addSection(const SectionPtr& section, MilliSecond repetition_rate = 0);

        //!
        //! Add some sections into the packetizer.
        //! The contents of the sections are shared.
        //! @param [in] sections A vector of smart pointer to the sections to packetize.
        //! @param [in] repetition_rate Repetition rate of the sections in milliseconds.
        //! If zero, simply packetize sections one after the other.
        //!
        void addSections(const SectionPtrVector& sections, MilliSecond repetition_rate = 0);

        //!
        //! Add all sections of a binary table into the packetizer.
        //! The contents of the sections are shared. If the table is not complete (there are
        //! missing sections), the sections which are present are individually added.
        //! @param [in] table A binary table to packetize.
        //! @param [in] repetition_rate Repetition rate of the sections in milliseconds.
        //! If zero, simply packetize sections one after the other.
        //!
        void addTable(const BinaryTable& table, MilliSecond repetition_rate = 0);

        //!
        //! Add all sections of a typed table into the packetizer.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table A table to packetize.
        //! @param [in] repetition_rate Repetition rate of the sections in milliseconds.
        //! If zero, simply packetize sections one after the other.
        //!
        void addTable(DuckContext& duck, const AbstractTable& table, MilliSecond repetition_rate = 0);

        //!
        //! Remove all sections with the specified table id.
        //! If one such section is currently being packetized, the rest of the section will be packetized.
        //! @param [in] tid The table id of the sections to remove.
        //!
        void removeSections(TID tid);

        //!
        //! Remove all sections with the specified table id and table id extension.
        //! If one such section is currently being packetized, the rest of the section will be packetized.
        //! @param [in] tid The table id of the sections to remove.
        //! @param [in] tid_ext The table id extension of the sections to remove.
        //!
        void removeSections(TID tid, uint16_t tid_ext);

        //!
        //! Remove all sections with the specified table id, table id extension and section number.
        //! If one such section is currently being packetized, the rest of the section will be packetized.
        //! @param [in] tid The table id of the sections to remove.
        //! @param [in] tid_ext The table id extension of the sections to remove.
        //! @param [in] sec_number The section number of the sections to remove.
        //!
        void removeSections(TID tid, uint16_t tid_ext, uint8_t sec_number);

        //!
        //! Remove all sections in the packetizer.
        //! If a section is currently being packetized, the rest of the section will be packetized.
        //!
        void removeAll();

        //!
        //! Get the number of stored sections to packetize.
        //! @return The number of stored sections to packetize.
        //!
        SectionCounter storedSectionCount() const
        {
            return _section_count;
        }

        //!
        //! Check if the last generated packet was the last packet in the cycle.
        //! Note that if the stuffing policy is NEVER, this is not reliable since it is
        //! unlikely that a packet actually terminates a cycle.
        //! @return True when the last generated packet was the last packet in the cycle.
        //!
        bool atCycleBoundary() const;

        // Inherited from Packetizer.
        virtual void reset() override;
        virtual std::ostream& display(std::ostream& strm) const override;

    private:
        // Each section is identified by a SectionDesc instance
        class SectionDesc
        {
        public:
            // Public fields
            SectionPtr     section {};      // Pointer to section
            MilliSecond    repetition = 0;  // Repetition rate, zero if none
            PacketCounter  last_packet = 0; // Packet index of last time the section was sent
            PacketCounter  due_packet = 0;  // Packet index of next time
            SectionCounter last_cycle = 0;  // Cycle index of last time the section was sent

            // Constructor
            SectionDesc(const SectionPtr& sec, MilliSecond rep);

            // Check if this section shall be inserted after some other one.
            bool insertAfter(const SectionDesc& other) const;

            // Display the internal state, mainly for debug.
            std::ostream& display(const DuckContext&, std::ostream&) const;
        };

        // Safe pointer for SectionDesc (not thread-safe)
        typedef SafePtr <SectionDesc, ts::null_mutex> SectionDescPtr;

        // List of sections
        typedef std::list <SectionDescPtr> SectionDescList;

        // Private members:
        StuffingPolicy  _stuffing {StuffingPolicy::NEVER};
        BitRate         _bitrate = 0;
        size_t          _section_count = 0;      // Number of sections in the 2 lists
        SectionDescList _sched_sections {};      // Scheduled sections, with repetition rates
        SectionDescList _other_sections {};      // Unscheduled sections
        PacketCounter   _sched_packets = 0;      // Size in TS packets of all sections in _sched_sections
        SectionCounter  _current_cycle {1};      // Cycle number (start at 1, always increasing)
        size_t          _remain_in_cycle = 0;    // Number of unsent sections in this cycle
        SectionCounter  _cycle_end {UNDEFINED};  // At end of cycle, contains the index of last section

        static constexpr SectionCounter UNDEFINED = ~SectionCounter(0);

        // Insert a scheduled section in the list, sorted by due_packet.
        void addScheduledSection(const SectionDescPtr&);

        // Remove all sections with the specified tid/tid_ext in the specified list.
        void removeSections(SectionDescList&, TID tid, uint16_t tid_ext, uint8_t sec_number, bool use_tid_ext, bool use_sec_number, bool scheduled);

        // Inherited from SectionProviderInterface
        virtual void provideSection(SectionCounter, SectionPtr&) override;
        virtual bool doStuffing() override;

        // Hide this method, we do not want the section provider to be replaced
        void setSectionProvider(SectionProviderInterface*) = delete;
    };
}
