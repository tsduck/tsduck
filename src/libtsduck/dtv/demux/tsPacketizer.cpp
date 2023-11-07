//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPacketizer.h"
#include "tsTSPacket.h"
#include "tsSection.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::Packetizer::Packetizer(const DuckContext& duck, PID pid, SectionProviderInterface* provider) :
    AbstractPacketizer(duck, pid),
    _provider(provider)
{
}

ts::Packetizer::~Packetizer()
{
}


//----------------------------------------------------------------------------
// Reset the content of a packetizer. Becomes empty.
//----------------------------------------------------------------------------

void ts::Packetizer::reset()
{
    AbstractPacketizer::reset();
    _section.clear();
    _next_byte = 0;
}


//----------------------------------------------------------------------------
// Build the next MPEG packet for the list of sections.
//----------------------------------------------------------------------------

bool ts::Packetizer::getNextPacket(TSPacket& pkt)
{
    // If there is no current section, get the next one.
    if (_section.isNull() && _provider != nullptr) {
        _provider->provideSection(_section_in_count, _section);
        _next_byte = 0;
        if (!_section.isNull()) {
            _section_in_count++;
        }
    }

    // If there is still no current section, return a null packet
    if (_section.isNull()) {
        configurePacket(pkt, true);
        return false;
    }

    // Various values to build the MPEG header.
    uint16_t pusi = 0x0000;         // payload_unit_start_indicator (set: 0x4000)
    uint8_t pointer_field = 0x00;   // pointer_field (used only if pusi is set)
    size_t remain_in_section = _section->size() - _next_byte;
    bool do_stuffing = true;        // do we need to insert stuffing at end of packet?
    SectionPtr next_section;        // next section after current one

    // Check if it is possible that a new section may start in the middle
    // of the packet. We check that after adding the remaining of the
    // current section, there is room for a pointer field (5 = 4-byte TS header
    // + 1-byte pointer field) and at least a short section header if we can't
    // split section headers.

    if (remain_in_section <= PKT_SIZE - 5 - (_split_headers ? 0 : SHORT_SECTION_HEADER_SIZE)) {
        // Check if next section requires stuffing before it.
        do_stuffing = _provider == nullptr ? true : _provider->doStuffing();
        if (!do_stuffing) {
            // No stuffing before next section => get next section.
            // Note that _provider cannot be null here.
            _provider->provideSection(_section_in_count, next_section);
            if (next_section.isNull()) {
                // If no next section, do stuffing anyway.
                do_stuffing = true;
            }
            else {
                // Now that we know the actual header size of the next section, recheck if it fits in packet
                _section_in_count++;
                do_stuffing = remain_in_section > PKT_SIZE - 5 - (_split_headers ? 0 : next_section->headerSize());
            }
        }
    }

    // Do we need to insert a pointer_field?
    if (_next_byte == 0) {
        // We are at the beginning of a section
        pusi = 0x4000;
        pointer_field = 0x00; // section starts immediately after pointer field
    }
    else if (!do_stuffing) {
        // A new section will start in the middle of the packet
        pusi = 0x4000;
        pointer_field = uint8_t(remain_in_section);  // point after current section
    }

    // Build the header
    pkt.b[0] = SYNC_BYTE;
    PutUInt16(pkt.b + 1, pusi);
    pkt.b[3] = 0x10;  // no adaptation field, has payload
    configurePacket(pkt, false);  // PID, continuity, count packets.

    // Remaining bytes in the packet.
    uint8_t* data = pkt.b + 4;
    size_t remain_in_packet = PKT_SIZE - 4;

    // Insert the pointer field if required.
    if (pusi) {
        *data++ = pointer_field;
        remain_in_packet--;
    }

    // Fill the packet payload
    while (remain_in_packet > 0) {
        // Copy a part of the current section in the packet
        size_t length = remain_in_section < remain_in_packet ? remain_in_section : remain_in_packet;
        std::memcpy(data, _section->content() + _next_byte, length);  // Flawfinder: ignore: memcpy()
        // Advance pointers
        data += length;
        remain_in_packet -= length;
        remain_in_section -= length;
        _next_byte += length;
        // If end of current section reached...
        if (remain_in_section == 0) {
            // Count sections
            _section_out_count++;
            // Remember next section if known
            _section = next_section;
            _next_byte = 0;
            next_section.clear();
            // If stuffing required at the end of packet, don't use next section
            if (do_stuffing) {
                break;
            }
            // If next section unknown, get it now
            if (_section.isNull()) {
                // If stuffing required before this section, give up
                if (_provider == nullptr || _provider->doStuffing()) {
                    break;
                }
                _provider->provideSection(_section_in_count, _section);
                // If no next section, stuff the end of packet
                if (_section.isNull()) {
                    break;
                }
                else {
                    _section_in_count++;
                }
            }
            // We no longer know about stuffing after current section
            do_stuffing = false;
            // If no room for new section header, stuff the end of packet
            if (!_split_headers && remain_in_packet < _section->headerSize()) {
                break;
            }
            // Get characteristcs of new section.
            remain_in_section = _section->size();
        }
    }

    // Do packet stuffing if necessary.
    // Note: the following test is normally useless since memset() works fine when
    // the size is zero. However, GCC 4.8.x erroneously deduces that remain_in_packet
    // is always zero (which is not true) and complains that memset() is always useless.
    // The test fixes this GCC error. However, it has not yet been tested if the behaviour
    // of the compiled code is correct with this version of GCC.
    if (remain_in_packet > 0) {
        std::memset(data, 0xFF, remain_in_packet);
    }
    return true;
}


//----------------------------------------------------------------------------
// Display the internal state of the packetizer, mainly for debug
//----------------------------------------------------------------------------

std::ostream& ts::Packetizer::display(std::ostream& strm) const
{
    return AbstractPacketizer::display(strm)
        << UString::Format(u"  Output sections: %'d", {_section_out_count}) << std::endl
        << UString::Format(u"  Provided sections: %'d", {_section_in_count}) << std::endl
        << "  Current section: "
        << (_section.isNull() ? UString(u"none") : UString::Format(u"%s, offset %d", {names::TID(duck(), _section->tableId()), _next_byte}))
        << std::endl;
}
