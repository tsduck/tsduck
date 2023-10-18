//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSpliceSchedule.h"
#include "tsTablesDisplay.h"
#include "tsDuckContext.h"
#include "tsTS.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"splice_schedule"
#define MY_STD ts::Standards::SCTE


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::SpliceSchedule::SpliceSchedule() :
    AbstractSignalization(MY_XML_NAME, MY_STD)
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
// Full dump of utc_splice_time.
//----------------------------------------------------------------------------

ts::UString ts::SpliceSchedule::DumpSpliceTime(const DuckContext& duck, uint32_t value)
{
    return UString::Format(u"0x%X (%s, leap seconds %s)", {value, ToUTCTime(duck, value).format(Time::DATETIME), duck.useLeapSeconds() ? u"included" : u"ignored"});
}


//----------------------------------------------------------------------------
// Display a SpliceSchedule command.
//----------------------------------------------------------------------------

void ts::SpliceSchedule::display(TablesDisplay& disp, const UString& margin) const
{
    for (auto& ev : events) {
        disp << margin << UString::Format(u"- Splice event id: 0x%X (%<d), cancel: %d", {ev.event_id, ev.canceled}) << std::endl;

        if (!ev.canceled) {
            disp << margin
                 << "  Out of network: " << UString::YesNo(ev.splice_out)
                 << ", program splice: " << UString::YesNo(ev.program_splice)
                 << ", duration set: " << UString::YesNo(ev.use_duration)
                 << std::endl;

            if (ev.program_splice) {
                // The complete program switches at a given time.
                disp << margin << "  UTC: " << DumpSpliceTime(disp.duck(), ev.program_utc) << std::endl;
            }
            if (!ev.program_splice) {
                // Program components switch individually.
                disp << margin << "  Number of components: " << ev.components_utc.size() << std::endl;
                for (auto& it : ev.components_utc) {
                    disp << margin << UString::Format(u"    Component tag: 0x%X (%<d)", {it.first})
                         << ", UTC: " << DumpSpliceTime(disp.duck(), it.second) << std::endl;
                }
            }
            if (ev.use_duration) {
                disp << margin << "  Duration PTS: " << PTSToString(ev.duration_pts) << ", auto return: " << UString::YesNo(ev.auto_return) << std::endl;
            }
            disp << margin << UString::Format(u"  Unique program id: 0x%X (%<d), avail: 0x%X (%<d), avails expected: %d", {ev.program_id, ev.avail_num, ev.avails_expected}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Deserialize a SpliceSchedule command from binary data.
//----------------------------------------------------------------------------

int ts::SpliceSchedule::deserialize(const uint8_t* data, size_t size)
{
    // Clear object content, make it a valid empty object.
    clear();

    const uint8_t* const start = data;
    if (size < 1) {
        invalidate();
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
                invalidate();
                return -1; // too short
            }

            ev.splice_out = (data[0] & 0x80) != 0;
            ev.program_splice = (data[0] & 0x40) != 0;
            ev.use_duration = (data[0] & 0x20) != 0;
            data++; size--;

            if (ev.program_splice) {
                // The complete program switches at a given time.
                if (size < 4) {
                    invalidate();
                    return -1; // too short
                }
                ev.program_utc = GetUInt32(data);
                data += 4; size -= 4;
            }
            else {
                // Program components switch individually.
                if (size < 1) {
                    invalidate();
                    return -1; // too short
                }
                size_t count = data[0];
                data++; size--;
                while (count-- > 0) {
                    if (size < 5) {
                        invalidate();
                        return -1; // too short
                    }
                    ev.components_utc.insert(std::make_pair(GetUInt8(data), GetUInt32(data + 1)));
                    data += 5; size -= 5;
                }
            }
            if (ev.use_duration) {
                if (size < 5) {
                    invalidate();
                    return -1; // too short
                }
                ev.auto_return = (data[0] & 0x80) != 0;
                ev.duration_pts = (uint64_t(data[0] & 0x01) << 32) | uint64_t(GetUInt32(data + 1));
                data += 5; size -= 5;
            }
            if (size < 4) {
                invalidate();
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

    return int(data - start);
}


//----------------------------------------------------------------------------
// Serialize the SpliceSchedule command.
//----------------------------------------------------------------------------

void ts::SpliceSchedule::serialize(ByteBlock& data) const
{
    data.appendUInt8(uint8_t(events.size()));

    for (auto& ev : events) {
        data.appendUInt32(ev.event_id);
        data.appendUInt8(ev.canceled ? 0xFF : 0x7F);

        if (!ev.canceled) {
            data.appendUInt8((ev.splice_out ? 0x80 : 0x00) |
                             (ev.program_splice ? 0x40 : 0x00) |
                             (ev.use_duration ? 0x20 : 0x00) |
                             0x1F);
            if (ev.program_splice) {
                data.appendUInt32(uint32_t(ev.program_utc));
            }
            else {
                data.appendUInt8(uint8_t(ev.components_utc.size()));
                for (auto& it : ev.components_utc) {
                    data.appendUInt8(it.first);
                    data.appendUInt32(uint32_t(it.second));
                }
            }
            if (ev.use_duration) {
                data.appendUInt8((ev.auto_return ? 0xFE : 0x7E) | uint8_t(ev.duration_pts >> 32));
                data.appendUInt32(uint32_t(ev.duration_pts));
            }
            data.appendUInt16(ev.program_id);
            data.appendUInt8(ev.avail_num);
            data.appendUInt8(ev.avails_expected);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SpliceSchedule::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto& ev : events) {
        xml::Element* e = root->addElement(u"splice_event");
        e->setIntAttribute(u"splice_event_id", ev.event_id, true);
        e->setBoolAttribute(u"splice_event_cancel", ev.canceled);
        if (!ev.canceled) {
            e->setBoolAttribute(u"out_of_network", ev.splice_out);
            e->setIntAttribute(u"unique_program_id", ev.program_id, true);
            e->setIntAttribute(u"avail_num", ev.avail_num);
            e->setIntAttribute(u"avails_expected", ev.avails_expected);
            if (ev.use_duration) {
                xml::Element* e1 = e->addElement(u"break_duration");
                e1->setBoolAttribute(u"auto_return", ev.auto_return);
                e1->setIntAttribute(u"duration", ev.duration_pts);
            }
            if (ev.program_splice) {
                e->setDateTimeAttribute(u"utc_splice_time", ToUTCTime(duck, ev.program_utc));
            }
            else {
                for (auto& it : ev.components_utc) {
                    xml::Element* e1 = e->addElement(u"component");
                    e1->setIntAttribute(u"component_tag", it.first);
                    e1->setDateTimeAttribute(u"utc_splice_time", ToUTCTime(duck, it.second));
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Dual interpretation of utc_splice_time XML attributes.
//----------------------------------------------------------------------------

bool ts::SpliceSchedule::GetSpliceTime(const DuckContext& duck, const xml::Element* elem, const UString& attribute, uint32_t& value)
{
    // Get required attribute value as a string.
    UString str;
    if (!elem->getAttribute(str, attribute, true)) {
        return false;
    }

    // If it can be interpreted as a uint32, this is a raw value.
    if (str.toInteger(value, u",")) {
        return true;
    }

    // Now it must be a date-time value.
    Time utc;
    if (!elem->getDateTimeAttribute(utc, attribute, true)) {
        return false;
    }

    // Convert to 32-bit value.
    value = FromUTCTime(duck, utc);
    return true;
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
        ok = xmlEvents[i]->getIntAttribute(ev.event_id, u"splice_event_id", true) &&
             xmlEvents[i]->getBoolAttribute(ev.canceled, u"splice_event_cancel", false, false);

        if (ok && !ev.canceled) {
            xml::ElementVector children;
            ok = xmlEvents[i]->getBoolAttribute(ev.splice_out, u"out_of_network", true) &&
                 xmlEvents[i]->getIntAttribute(ev.program_id, u"unique_program_id", true) &&
                 xmlEvents[i]->getIntAttribute(ev.avail_num, u"avail_num", false, 0) &&
                 xmlEvents[i]->getIntAttribute(ev.avails_expected, u"avails_expected", false, 0) &&
                 xmlEvents[i]->getChildren(children, u"break_duration", 0, 1);
            ev.use_duration = !children.empty();
            if (ok && ev.use_duration) {
                assert(children.size() == 1);
                ok = children[0]->getBoolAttribute(ev.auto_return, u"auto_return", true) &&
                     children[0]->getIntAttribute(ev.duration_pts, u"duration", true);
            }
            ev.program_splice = xmlEvents[i]->hasAttribute(u"utc_splice_time");
            if (ok && ev.program_splice) {
                ok = GetSpliceTime(duck, xmlEvents[i], u"utc_splice_time", ev.program_utc);
            }
            if (ok && !ev.program_splice) {
                ok = xmlEvents[i]->getChildren(children, u"component", 0, 255);
                for (size_t i1 = 0; ok && i1 < children.size(); ++i1) {
                    uint8_t tag = 0;
                    uint32_t utc = 0;
                    ok = children[i1]->getIntAttribute(tag, u"component_tag", true) &&
                         GetSpliceTime(duck, children[i1], u"utc_splice_time", utc);
                    ev.components_utc[tag] = utc;
                }
            }
        }
        events.push_back(ev);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Convert between actual UTC time and 32-bit SCTE 35 utc_splice_time.
//----------------------------------------------------------------------------

ts::Time ts::SpliceSchedule::ToUTCTime(const DuckContext& duck, uint32_t value)
{
    Time utc(Time::GPSEpoch + Second(value) * MilliSecPerSec);
    if (duck.useLeapSeconds()) {
        utc -= Time::GPSEpoch.leapSecondsTo(utc) * MilliSecPerSec;
    }
    return utc;
}

uint32_t ts::SpliceSchedule::FromUTCTime(const DuckContext& duck, const Time& value)
{
    if (value < Time::GPSEpoch) {
        return 0;
    }
    Second utc = (value - Time::GPSEpoch) / MilliSecPerSec;
    if (duck.useLeapSeconds()) {
        utc += Time::GPSEpoch.leapSecondsTo(value);
    }
    return uint32_t(utc);
}
