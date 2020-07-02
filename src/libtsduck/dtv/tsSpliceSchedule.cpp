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

#include "tsSpliceSchedule.h"
#include "tsTablesDisplay.h"
#include "tsDuckContext.h"
#include "tsNames.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"splice_schedule"
#define MY_STD ts::Standards::SCTE


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::SpliceSchedule::SpliceSchedule() :
    AbstractSignalization(MY_XML_NAME, MY_STD),
    events()
{
}

ts::SpliceSchedule::Event::Event() :
    event_id(0),
    canceled(false),
    splice_out(false),
    program_splice(false),
    use_duration(false),
    program_utc(0),
    components_utc(),
    duration_pts(0),
    auto_return(false),
    program_id(0),
    avail_num(0),
    avails_expected(0)
{
}


//----------------------------------------------------------------------------
// Reset all fields to default initial values.
//----------------------------------------------------------------------------

void ts::SpliceSchedule::clearContent()
{
    events.clear();
}


//----------------------------------------------------------------------------
// Display a SpliceSchedule command.
//----------------------------------------------------------------------------

void ts::SpliceSchedule::display(TablesDisplay& display, int indent) const
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    for (EventList::const_iterator ev = events.begin(); ev != events.end(); ++ev) {
        strm << margin << UString::Format(u"- Splice event id: 0x%X, cancel: %d", {ev->event_id, ev->canceled}) << std::endl;

        if (!ev->canceled) {
            strm << margin
                 << "  Out of network: " << UString::YesNo(ev->splice_out)
                 << ", program splice: " << UString::YesNo(ev->program_splice)
                 << ", duration set: " << UString::YesNo(ev->use_duration)
                 << std::endl;

            if (ev->program_splice) {
                // The complete program switches at a given time.
                strm << margin << UString::Format(u"  UTC: %s", {Time::UnixTimeToUTC(ev->program_utc).format(Time::DATE | Time::TIME)}) << std::endl;
            }
            if (!ev->program_splice) {
                // Program components switch individually.
                strm << margin << "  Number of components: " << ev->components_utc.size() << std::endl;
                for (UTCByComponent::const_iterator it = ev->components_utc.begin(); it != ev->components_utc.end(); ++it) {
                    strm << margin
                         << UString::Format(u"    Component tag: 0x%X (%d)", {it->first, it->first})
                         << UString::Format(u", UTC: %s", {Time::UnixTimeToUTC(it->second).format(Time::DATE | Time::TIME)})
                         << std::endl;
                }
            }
            if (ev->use_duration) {
                strm << margin
                     << UString::Format(u"  Duration PTS: 0x%09X (%d), auto return: %s", {ev->duration_pts, ev->duration_pts, UString::YesNo(ev->auto_return)})
                     << std::endl;
            }
            strm << margin
                 << UString::Format(u"  Unique program id: 0x%X (%d), avail: 0x%X (%d), avails expected: %d", {ev->program_id, ev->program_id, ev->avail_num, ev->avail_num, ev->avails_expected})
                 << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Deserialize a SpliceSchedule command from binary data.
//----------------------------------------------------------------------------

int ts::SpliceSchedule::deserialize(const uint8_t* data, size_t size)
{
    const uint8_t* const start = data;
    clear();

    if (size < 1) {
        return -1; // too short
    }

    // Numbere of splice events
    uint8_t spliceCount = data[0];
    data += 1; size -= 1;

    while (spliceCount > 0) {
        // Decode one event.
        Event ev;

        ev.event_id = GetUInt32(data);
        ev.canceled = (data[4] & 0x80) != 0;
        data += 5; size -= 5;

        if (!ev.canceled) {
            if (size < 1) {
                return -1; // too short
            }

            ev.splice_out = (data[0] & 0x80) != 0;
            ev.program_splice = (data[0] & 0x40) != 0;
            ev.use_duration = (data[0] & 0x20) != 0;
            data++; size--;

            if (ev.program_splice) {
                // The complete program switches at a given time.
                if (size < 4) {
                    return -1; // too short
                }
                ev.program_utc = GetUInt32(data);
                data += 4; size -= 4;
            }
            else {
                // Program components switch individually.
                if (size < 1) {
                    return -1; // too short
                }
                size_t count = data[0];
                data++; size--;
                while (count-- > 0) {
                    if (size < 5) {
                        return -1; // too short
                    }
                    ev.components_utc.insert(std::make_pair(GetUInt8(data), GetUInt32(data + 1)));
                    data += 5; size -= 5;
                }
            }
            if (ev.use_duration) {
                if (size < 5) {
                    return -1; // too short
                }
                ev.auto_return = (data[0] & 0x80) != 0;
                ev.duration_pts = (uint64_t(data[0] & 0x01) << 32) | uint64_t(GetUInt32(data + 1));
                data += 5; size -= 5;
            }
            if (size < 4) {
                return -1; // too short
            }
            ev.program_id = GetUInt16(data);
            ev.avail_num = data[2];
            ev.avails_expected = data[3];
            data += 4; size -= 4;
        }

        // Finally add the deserialized event in the list.
        events.push_back(ev);
        spliceCount--;
    }

    _is_valid = true;
    return int(data - start);
}


//----------------------------------------------------------------------------
// Serialize the SpliceSchedule command.
//----------------------------------------------------------------------------

void ts::SpliceSchedule::serialize(ByteBlock& data) const
{
    data.appendUInt8(uint8_t(events.size()));

    for (auto ev = events.begin(); ev != events.end(); ++ev) {
        data.appendUInt32(ev->event_id);
        data.appendUInt8(ev->canceled ? 0xFF : 0x7F);

        if (!ev->canceled) {
            data.appendUInt8((ev->splice_out ? 0x80 : 0x00) |
                             (ev->program_splice ? 0x40 : 0x00) |
                             (ev->use_duration ? 0x20 : 0x00) |
                             0x1F);
            if (ev->program_splice) {
                data.appendUInt32(uint32_t(ev->program_utc));
            }
            else {
                data.appendUInt8(uint8_t(ev->components_utc.size()));
                for (auto it = ev->components_utc.begin(); it != ev->components_utc.end(); ++it) {
                    data.appendUInt8(it->first);
                    data.appendUInt32(uint32_t(it->second));
                }
            }
            if (ev->use_duration) {
                data.appendUInt8((ev->auto_return ? 0xFE : 0x7E) | uint8_t(ev->duration_pts >> 32));
                data.appendUInt32(uint32_t(ev->duration_pts));
            }
            data.appendUInt16(ev->program_id);
            data.appendUInt8(ev->avail_num);
            data.appendUInt8(ev->avails_expected);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SpliceSchedule::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto ev = events.begin(); ev != events.end(); ++ev) {
        xml::Element* e = root->addElement(u"splice_event");
        e->setIntAttribute(u"splice_event_id", ev->event_id, true);
        e->setBoolAttribute(u"splice_event_cancel", ev->canceled);
        if (!ev->canceled) {
            e->setBoolAttribute(u"out_of_network", ev->splice_out);
            e->setIntAttribute(u"unique_program_id", ev->program_id, true);
            e->setIntAttribute(u"avail_num", ev->avail_num);
            e->setIntAttribute(u"avails_expected", ev->avails_expected);
            if (ev->use_duration) {
                xml::Element* e1 = e->addElement(u"break_duration");
                e1->setBoolAttribute(u"auto_return", ev->auto_return);
                e1->setIntAttribute(u"duration", ev->duration_pts);
            }
            if (ev->program_splice) {
                e->setIntAttribute(u"utc_splice_time", ev->program_utc);
            }
            else {
                for (auto it = ev->components_utc.begin(); it != ev->components_utc.end(); ++it) {
                    xml::Element* e1 = e->addElement(u"component");
                    e1->setIntAttribute(u"component_tag", it->first);
                    e1->setIntAttribute(u"utc_splice_time", it->second);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SpliceSchedule::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xmlEvents;
    bool ok = element->getChildren(xmlEvents, u"splice_event", 0, 255);

    for (size_t i = 0; ok && i < xmlEvents.size(); ++i) {
        Event ev;
        ok = xmlEvents[i]->getIntAttribute<uint32_t>(ev.event_id, u"splice_event_id", true) &&
             xmlEvents[i]->getBoolAttribute(ev.canceled, u"splice_event_cancel", false, false);

        if (ok && !ev.canceled) {
            xml::ElementVector children;
            ok = xmlEvents[i]->getBoolAttribute(ev.splice_out, u"out_of_network", true) &&
                 xmlEvents[i]->getIntAttribute<uint16_t>(ev.program_id, u"unique_program_id", true) &&
                 xmlEvents[i]->getIntAttribute<uint8_t>(ev.avail_num, u"avail_num", false, 0) &&
                 xmlEvents[i]->getIntAttribute<uint8_t>(ev.avails_expected, u"avails_expected", false, 0) &&
                 xmlEvents[i]->getChildren(children, u"break_duration", 0, 1);
            ev.use_duration = !children.empty();
            if (ok && ev.use_duration) {
                assert(children.size() == 1);
                ok = children[0]->getBoolAttribute(ev.auto_return, u"auto_return", true) &&
                     children[0]->getIntAttribute<uint64_t>(ev.duration_pts, u"duration", true);
            }
            ev.program_splice = xmlEvents[i]->hasAttribute(u"utc_splice_time");
            if (ok && ev.program_splice) {
                ok = xmlEvents[i]->getIntAttribute<uint32_t>(ev.program_utc, u"utc_splice_time", true);
            }
            if (ok && !ev.program_splice) {
                ok = xmlEvents[i]->getChildren(children, u"component", 0, 255);
                for (size_t i1 = 0; ok && i1 < children.size(); ++i1) {
                    uint8_t tag = 0;
                    uint32_t utc = 0;
                    ok = children[i1]->getIntAttribute<uint8_t>(tag, u"component_tag", true) &&
                         children[i1]->getIntAttribute<uint32_t>(utc, u"utc_splice_time", true);
                    ev.components_utc[tag] = utc;
                }
            }
        }
        events.push_back(ev);
    }
    return ok;
}
