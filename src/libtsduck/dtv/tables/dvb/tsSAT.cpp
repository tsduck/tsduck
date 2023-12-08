//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSAT.h"
#include "tsPSIRepository.h"
#include "tsSatelliteDeliverySystemDescriptor.h"

#include <algorithm>

#define MY_XML_NAME u"SAT"
#define MY_CLASS ts::SAT
#define MY_TID ts::TID_SAT
#define MY_PID ts::PID_SAT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});

typedef ts::FloatingPoint<double> Double;


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::SAT::SAT(uint8_t vers, bool cur, uint16_t satellite_table_id_, uint16_t table_count_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    satellite_table_id(satellite_table_id_),
    table_count(table_count_)
{
}

ts::SAT::SAT(DuckContext& duck, const BinaryTable& table) :
    SAT()
{
    deserialize(duck, table);
}

ts::SAT::SAT(const SAT& other) :
    AbstractLongTable(other),
    satellite_position_v2_info(other.satellite_position_v2_info),
    cell_fragment_info(other.cell_fragment_info),
    time_association_fragment_info(other.time_association_fragment_info),
    beam_hopping_time_plan_info(other.beam_hopping_time_plan_info),
    satellite_table_id(other.satellite_table_id),
    table_count(other.table_count)
{
}

void ts::SAT::clearContent()
{
    // satellite_table_id = 0;
    satellite_position_v2_info.clear();
    cell_fragment_info.clear();
    time_association_fragment_info.clear();
    beam_hopping_time_plan_info.clear();
}


//----------------------------------------------------------------------------
// Desructors.
//----------------------------------------------------------------------------

ts::SAT::SAT_base::~SAT_base()
{
}


//----------------------------------------------------------------------------
// Helpers.
//----------------------------------------------------------------------------

//!
//! The number of bits needed after the slot map for byte alignment
//! @return The number of bits needed for byte aligment
//!
static uint8_t padding_size_K(size_t map_size)
{
    return 7 - (( map_size - 1) % 8);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::SAT::tableIdExtension() const
{
    return uint16_t((satellite_table_id & 0x3f) << 10) | uint16_t(table_count & 0x3FF);
}

//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::SAT::maxPayloadSize() const
{
    // Although a "private section" in the MPEG sense, the SAT section is limited to 4096 bytes in ETSI EN 300 468.
    return MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE;
}

//----------------------------------------------------------------------------
// Geostationary satellite
//----------------------------------------------------------------------------

void ts::SAT::satellite_position_v2_info_type::geostationary_position_type::serialize(PSIBuffer& buf) const
{
    buf.putBCD(orbital_position, 4);
    buf.putBits(west_east_flag, 1);
    buf.putReservedZero(7);
}

void ts::SAT::satellite_position_v2_info_type::geostationary_position_type::deserialize(PSIBuffer& buf)
{
    buf.getBCD(orbital_position, 4);
    west_east_flag = buf.getBit();
    buf.skipBits(7);
}

void ts::SAT::satellite_position_v2_info_type::geostationary_position_type::toXML(xml::Element* root)
{
    root->setAttribute(u"orbital_position", UString::Format(u"%d.%d", { orbital_position / 10, orbital_position % 10 }));
    root->setEnumAttribute(SatelliteDeliverySystemDescriptor::DirectionNames, u"west_east_flag", west_east_flag);
}

bool ts::SAT::satellite_position_v2_info_type::geostationary_position_type::fromXML(const xml::Element* element)
{
    UString orbit;
    bool ok = element->getAttribute(orbit, u"orbital_position", true) &&
        element->getEnumAttribute(west_east_flag, SatelliteDeliverySystemDescriptor::DirectionNames, u"west_east_flag", true);

    if (ok) {
        // Expected orbital position is "XX.X" as in "19.2".
        UStringVector fields;
        uint16_t p1 = 0;
        uint16_t p2 = 0;
        orbit.split(fields, u'.');
        ok = fields.size() == 2 && fields[0].toInteger(p1) && fields[1].toInteger(p2) && p2 < 10;
        if (ok) {
            orbital_position = (p1 * 10) + p2;
        }
        else {
            element->report().error(u"Invalid value '%s' for attribute 'orbital_position' in <%s> at line %d, use 'nn.n'", { orbit, element->name(), element->lineNumber() });
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Earth orbiting satellite
//----------------------------------------------------------------------------

void ts::SAT::satellite_position_v2_info_type::earth_orbiting_satallite_type::serialize(PSIBuffer& buf) const
{
    buf.putUInt8(epoch_year);
    buf.putUInt16(day_of_the_year);
    buf.putFloat32(day_fraction);
    buf.putFloat32(mean_motion_first_derivative);
    buf.putFloat32(mean_motion_second_derivative);
    buf.putFloat32(drag_term);
    buf.putFloat32(inclination);
    buf.putFloat32(right_ascension_of_the_ascending_node);
    buf.putFloat32(eccentricity);
    buf.putFloat32(argument_of_perigree);
    buf.putFloat32(mean_anomaly);
    buf.putFloat32(mean_motion);
}

void ts::SAT::satellite_position_v2_info_type::earth_orbiting_satallite_type::deserialize(PSIBuffer& buf)
{
    epoch_year = buf.getUInt8();
    day_of_the_year = buf.getUInt16();
    day_fraction = buf.getFloat32();
    mean_motion_first_derivative = buf.getFloat32();
    mean_motion_second_derivative = buf.getFloat32();
    drag_term = buf.getFloat32();
    inclination = buf.getFloat32();
    right_ascension_of_the_ascending_node = buf.getFloat32();
    eccentricity = buf.getFloat32();
    argument_of_perigree = buf.getFloat32();
    mean_anomaly = buf.getFloat32();
    mean_motion = buf.getFloat32();
}

void ts::SAT::satellite_position_v2_info_type::earth_orbiting_satallite_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"epoch_year", epoch_year);
    root->setIntAttribute(u"day_of_the_year", day_of_the_year);
    root->setFloatAttribute(u"day_fraction", day_fraction);
    root->setFloatAttribute(u"mean_motion_first_derivative", mean_motion_first_derivative);
    root->setFloatAttribute(u"mean_motion_second_derivative", mean_motion_second_derivative);
    root->setFloatAttribute(u"drag_term", drag_term);
    root->setFloatAttribute(u"inclination", inclination);
    root->setFloatAttribute(u"right_ascension_of_the_ascending_node", right_ascension_of_the_ascending_node);
    root->setFloatAttribute(u"eccentricity", eccentricity);
    root->setFloatAttribute(u"argument_of_perigree", argument_of_perigree);
    root->setFloatAttribute(u"mean_anomaly", mean_anomaly);
    root->setFloatAttribute(u"mean_motion", mean_motion);
}

bool ts::SAT::satellite_position_v2_info_type::earth_orbiting_satallite_type::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(epoch_year, u"epoch_year", true) &&
        element->getIntAttribute(day_of_the_year, u"day_of_the_year", true) &&
        element->getFloatAttribute(day_fraction, u"day_fraction", true) &&
        element->getFloatAttribute(mean_motion_first_derivative, u"mean_motion_first_derivative", true) &&
        element->getFloatAttribute(mean_motion_second_derivative, u"mean_motion_second_derivative", true) &&
        element->getFloatAttribute(drag_term, u"drag_term", true) &&
        element->getFloatAttribute(inclination, u"inclination", true) &&
        element->getFloatAttribute(right_ascension_of_the_ascending_node, u"right_ascension_of_the_ascending_node", true) &&
        element->getFloatAttribute(eccentricity, u"eccentricity", true) &&
        element->getFloatAttribute(argument_of_perigree, u"argument_of_perigree", true) &&
        element->getFloatAttribute(mean_anomaly, u"mean_anomaly", true) &&
        element->getFloatAttribute(mean_motion, u"mean_motion", true);
}


//----------------------------------------------------------------------------
// Satellite position v2
//----------------------------------------------------------------------------

void ts::SAT::satellite_position_v2_info_type::serialize(PSIBuffer& buf) const
{
    buf.putBits(satellite_id, 24);
    buf.putReservedZero(7);
    buf.putBits(position_system, 1);
    if ((position_system == POSITION_SYSTEM_GEOSTATIONARY) && geostationaryPosition.has_value()) {
        geostationaryPosition.value().serialize(buf);
    }
    else if ((position_system == POSITION_SYSTEM_EARTH_ORBITING) && earthOrbiting.has_value()) {
        earthOrbiting.value().serialize(buf);
    }
}

void ts::SAT::satellite_position_v2_info_type::deserialize(PSIBuffer& buf)
{
    buf.getBits(satellite_id, 24);
    buf.skipBits(7);
    buf.getBits(position_system, 1);
    if (position_system == POSITION_SYSTEM_GEOSTATIONARY) {
        geostationary_position_type gPos(buf);
        geostationaryPosition = gPos;
    }
    if (position_system == POSITION_SYSTEM_EARTH_ORBITING) {
        earth_orbiting_satallite_type eos(buf);
        earthOrbiting = eos;
    }
}

void ts::SAT::satellite_position_v2_info_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"satellite_id", satellite_id, true);
    if ((position_system == POSITION_SYSTEM_GEOSTATIONARY) && geostationaryPosition.has_value()) {
        geostationaryPosition.value().toXML(root->addElement(u"geostationary"));
    }
    else if ((position_system == POSITION_SYSTEM_EARTH_ORBITING) && earthOrbiting.has_value()) {
        earthOrbiting.value().toXML(root->addElement(u"earth_orbiting"));
    }
}

bool ts::SAT::satellite_position_v2_info_type::fromXML(const xml::Element* element)
{
    bool ok = element->getIntAttribute(satellite_id, u"satellite_id", true, 0, 0, 0xFFFFFF);

    if (ok) {
        xml::ElementVector geos, eos;
        ok = element->getChildren(geos, u"geostationary", 0, 1) &&
            element->getChildren(eos, u"earth_orbiting", 0, 1);
        if (ok && (geos.size() + eos.size() == 0)) {
            element->report().error(u"either <geostationary> or <earth_orbiting> must be provided in <%s>, line %d", { element->name(), element->lineNumber() });
            ok = false;
        }
        if (ok && (geos.size() + eos.size() != 1)) {
            element->report().error(u"only one of <geostationary> or <earth_orbiting> is permitted in <%s>, line %d", { element->name(), element->lineNumber() });
            ok = false;
        }
        if (ok) {
            if (!geos.empty()) {
                position_system = POSITION_SYSTEM_GEOSTATIONARY;
                satellite_position_v2_info_type::geostationary_position_type newGeos;
                ok = newGeos.fromXML(geos[0]);
                if (ok) {
                    geostationaryPosition = newGeos;
                }
            }
            if (!eos.empty()) {
                position_system = POSITION_SYSTEM_EARTH_ORBITING;
                satellite_position_v2_info_type::earth_orbiting_satallite_type newEos;
                ok = newEos.fromXML(eos[0]);
                if (ok) {
                    earthOrbiting = newEos;
                }
            }
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Network Clock Reference
//----------------------------------------------------------------------------

void ts::SAT::NCR_type::clear()
{
    base = 0;
    ext = 0;
}

void ts::SAT::NCR_type::serialize(PSIBuffer& buf) const
{
    buf.putBits(base, 33);
    buf.putReservedZero(6);
    buf.putBits(ext, 9);
}

void ts::SAT::NCR_type::deserialize(PSIBuffer& buf)
{
    buf.getBits(base, 33);
    buf.skipBits(6);
    buf.getBits(ext, 9);
}

void ts::SAT::NCR_type::toXML(xml::Element* parent, const UString element_name)
{
    toXML(parent->addElement(element_name));
}

void ts::SAT::NCR_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"base", base);
    root->setIntAttribute(u"ext", ext);
}


bool ts::SAT::NCR_type::fromXML(const xml::Element* parent, const UString element_name)
{
    xml::ElementVector children;
    bool ok = parent->getChildren(children, element_name, 1, 1);

    return ok && fromXML(children[0]);
}

bool ts::SAT::NCR_type::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(base, u"base", true, 0, 0, 0x1FFFFFFFF) &&
        element->getIntAttribute(ext, u"ext", true, 0, 0, 0x1FF);
}

//----------------------------------------------------------------------------
// New Delivery System
//----------------------------------------------------------------------------

void ts::SAT::cell_fragment_info_type::new_delivery_system_id_type::serialize(PSIBuffer& buf) const
{
    buf.putUInt32(new_delivery_system_id);
    time_of_application.serialize(buf);
}

void ts::SAT::cell_fragment_info_type::new_delivery_system_id_type::deserialize(PSIBuffer& buf)
{
    new_delivery_system_id = buf.getUInt32();
    time_of_application.deserialize(buf);
}

void ts::SAT::cell_fragment_info_type::new_delivery_system_id_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"id", new_delivery_system_id, true);
    time_of_application.toXML(root, u"time_of_application");
}

bool ts::SAT::cell_fragment_info_type::new_delivery_system_id_type::fromXML(const xml::Element* root)
{
    return root->getIntAttribute(new_delivery_system_id, u"id", true) &&
        time_of_application.fromXML(root, u"time_of_application");
}

//----------------------------------------------------------------------------
// Obsolescent Delivery System
//----------------------------------------------------------------------------

void ts::SAT::cell_fragment_info_type::obsolescent_delivery_system_id_type::serialize(PSIBuffer& buf) const
{
    buf.putUInt32(obsolescent_delivery_system_id);
    time_of_obsolescence.serialize(buf);
}

void ts::SAT::cell_fragment_info_type::obsolescent_delivery_system_id_type::deserialize(PSIBuffer& buf)
{
    obsolescent_delivery_system_id = buf.getUInt32();
    time_of_obsolescence.deserialize(buf);
}

void ts::SAT::cell_fragment_info_type::obsolescent_delivery_system_id_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"id", obsolescent_delivery_system_id, true);
    time_of_obsolescence.toXML(root, u"time_of_obsolescence");
}

bool ts::SAT::cell_fragment_info_type::obsolescent_delivery_system_id_type::fromXML(const xml::Element* root)
{
    return root->getIntAttribute(obsolescent_delivery_system_id, u"id", true) &&
        time_of_obsolescence.fromXML(root, u"time_of_obsolescence");
}


//----------------------------------------------------------------------------
// Cell Fragment
//----------------------------------------------------------------------------

void ts::SAT::cell_fragment_info_type::serialize(PSIBuffer& buf) const
{
    buf.putUInt32(cell_fragment_id);
    buf.putBit(first_occurence);
    buf.putBit(last_occurence);
    if (first_occurence) {
        buf.putReservedZero(4);
        buf.putBits(center_latitude.value(), 18);
        buf.putReservedZero(5);
        buf.putBits(center_longitude.value(), 19);
        buf.putBits(max_distance.value(), 24);
        buf.putReservedZero(6);
    }
    else {
        buf.putReservedZero(4);
    }
    buf.putBits(delivery_system_ids.size(), 10);
    for (auto it : delivery_system_ids) {
        buf.putUInt32(it);
    }
    buf.putReservedZero(6);
    buf.putBits(new_delivery_system_ids.size(), 10);
    for (auto it : new_delivery_system_ids) {
        it.serialize(buf);
    }
    buf.putReservedZero(6);
    buf.putBits(obsolescent_delivery_system_ids.size(), 10);
    for (auto it : obsolescent_delivery_system_ids) {
        it.serialize(buf);
    }
}

void ts::SAT::cell_fragment_info_type::deserialize(PSIBuffer& buf)
{
    cell_fragment_id = buf.getUInt32();
    first_occurence = buf.getBit();
    last_occurence = buf.getBit();
    if (first_occurence) {
        buf.skipBits(4);
        buf.getBits(center_latitude, 18);
        buf.skipBits(5);
        buf.getBits(center_longitude, 19);
        max_distance = buf.getUInt24();
        buf.skipBits(6);
    }
    else {
        buf.skipBits(4);
    }
    uint16_t delivery_system_id_loop_count;
    buf.getBits(delivery_system_id_loop_count, 10);
    for (uint16_t i = 0; i < delivery_system_id_loop_count; i++) {
        uint32_t delivery_system_id = buf.getUInt32();
        delivery_system_ids.push_back(delivery_system_id);
    }
    buf.skipBits(6);
    uint16_t new_delivery_system_id_loop_count;
    buf.getBits(new_delivery_system_id_loop_count, 10);
    for (uint16_t i = 0; i < new_delivery_system_id_loop_count; i++) {
        new_delivery_system_id_type newDS(buf);
        new_delivery_system_ids.push_back(newDS);
    }
    buf.skipBits(6);
    uint16_t obsolescent_delivery_system_id_loop_count;
    buf.getBits(obsolescent_delivery_system_id_loop_count, 10);
    for (uint16_t i = 0; i < obsolescent_delivery_system_id_loop_count; i++) {
        obsolescent_delivery_system_id_type obsDS(buf);
        obsolescent_delivery_system_ids.push_back(obsDS);
    }
}

void ts::SAT::cell_fragment_info_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"cell_fragment_id", cell_fragment_id, true);
    root->setBoolAttribute(u"first_occurence", first_occurence);
    root->setBoolAttribute(u"last_occurence", last_occurence);
    root->setOptionalIntAttribute(u"center_latitude", center_latitude);
    root->setOptionalIntAttribute(u"center_longitude", center_longitude);
    root->setOptionalIntAttribute(u"max_distance", max_distance);

    for (auto it : delivery_system_ids) {
        xml::Element* deliverySystem = root->addElement(u"delivery_system");
        deliverySystem->setIntAttribute(u"id", it, true);
    }
    for (auto it : new_delivery_system_ids) {
        it.toXML(root->addElement(u"new_delivery_system"));
    }
    for (auto it : obsolescent_delivery_system_ids) {
        it.toXML(root->addElement(u"obsolescent_delivery_system"));
    }
}

bool ts::SAT::cell_fragment_info_type::fromXML(const xml::Element* element)
{
    bool ok = element->getIntAttribute(cell_fragment_id, u"cell_fragment_id", true) &&
        element->getBoolAttribute(first_occurence, u"first_occurence", true) &&
        element->getBoolAttribute(last_occurence, u"last_occurence", true) &&
        element->getOptionalIntAttribute(center_latitude, u"center_latitude", -90000, 90000) &&
        element->getOptionalIntAttribute(center_longitude, u"center_longitude", -180000, 180000) &&
        element->getOptionalIntAttribute(max_distance, u"max_distance", 0, 0xFFFFFF);

    xml::ElementVector delivery_systems, new_delivery_systems, obsolescent_delivery_systems;
    if (ok) {
        ok = element->getChildren(delivery_systems, u"delivery_system") &&
            element->getChildren(new_delivery_systems, u"new_delivery_system") &&
            element->getChildren(obsolescent_delivery_systems, u"obsolescent_delivery_system");
    }
    if (ok) {
        for (size_t j = 0; ok && j < delivery_systems.size(); j++) {
            uint32_t delivery_system_id;
            ok = delivery_systems[j]->getIntAttribute(delivery_system_id, u"id", true);
            if (ok) {
                delivery_system_ids.push_back(delivery_system_id);
            }
        }
        for (size_t j = 0; ok && j < new_delivery_systems.size(); j++) {
            cell_fragment_info_type::new_delivery_system_id_type newDS;
            ok = newDS.fromXML(new_delivery_systems[j]);
            if (ok) {
                new_delivery_system_ids.push_back(newDS);
            }
        }
        for (size_t j = 0; ok && j < obsolescent_delivery_systems.size(); j++) {
            cell_fragment_info_type::obsolescent_delivery_system_id_type obsDS;
            ok = obsDS.fromXML(obsolescent_delivery_systems[j]);
            if (ok) {
                obsolescent_delivery_system_ids.push_back(obsDS);
            }
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Time Association
//----------------------------------------------------------------------------

void ts::SAT::time_association_info_type::clear()
{
    association_type = 0;
    ncr.clear();
    association_timestamp_seconds = 0;
    association_timestamp_nanoseconds = 0;
    leap59 = leap61 = false;
    past_leap59 = past_leap61 = false;
}

void ts::SAT::time_association_info_type::serialize(PSIBuffer& buf) const
{
    buf.putBits(association_type, 4);
    if (association_type == 1) {
        buf.putBit(leap59);
        buf.putBit(leap61);
        buf.putBit(past_leap59);
        buf.putBit(past_leap61);
    }
    else {
        buf.putReservedZero(4);
    }
    ncr.serialize(buf);
    buf.putUInt64(association_timestamp_seconds);
    buf.putUInt32(association_timestamp_nanoseconds);
}

void ts::SAT::time_association_info_type::deserialize(PSIBuffer& buf)
{
    buf.getBits(association_type, 4);
    if (association_type == 1) {
        leap59 = buf.getBool();
        leap61 = buf.getBool();
        past_leap59 = buf.getBool();
        past_leap61 = buf.getBool();
    }
    else {
        buf.skipBits(4);
    }
    ncr.deserialize(buf);
    association_timestamp_seconds = buf.getUInt64();
    association_timestamp_nanoseconds = buf.getUInt32();
}

void ts::SAT::time_association_info_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"association_type", association_type);
    ncr.toXML(root, u"ncr");
    root->setIntAttribute(u"association_timestamp_seconds", association_timestamp_seconds);
    root->setIntAttribute(u"association_timestamp_nanoseconds", association_timestamp_nanoseconds);
    if (association_type == 1) {
        root->setBoolAttribute(u"leap59", leap59);
        root->setBoolAttribute(u"leap61", leap61);
        root->setBoolAttribute(u"past_leap59", past_leap59);
        root->setBoolAttribute(u"past_leap61", past_leap61);
    }
}

bool ts::SAT::time_association_info_type::fromXML(const xml::Element* element)
{
    bool ok = element->getIntAttribute(association_type, u"association_type", true, 0, 0, 1) &&
        ncr.fromXML(element, u"ncr") &&
        element->getIntAttribute(association_timestamp_seconds, u"association_timestamp_seconds", true) &&
        element->getIntAttribute(association_timestamp_nanoseconds, u"association_timestamp_nanoseconds", true);
    if (ok && association_type == 1) {
        ok = element->getBoolAttribute(leap59, u"leap59", true, false) &&
            element->getBoolAttribute(leap61, u"leap61", true, false) &&
            element->getBoolAttribute(past_leap59, u"past_leap59", true, false) &&
            element->getBoolAttribute(past_leap61, u"past_leap61", true, false);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Beam Hopping Illumination
//----------------------------------------------------------------------------

void ts::SAT::beam_hopping_time_plan_info_type::slot::serialize(PSIBuffer& buf) const
{
    buf.putBit(on);
}

void ts::SAT::beam_hopping_time_plan_info_type::slot::deserialize(uint16_t slot_num, PSIBuffer& buf)
{
    number = slot_num;
    deserialize(buf);
}

void ts::SAT::beam_hopping_time_plan_info_type::slot::deserialize(PSIBuffer& buf)
{
    on = buf.getBit();
}

void ts::SAT::beam_hopping_time_plan_info_type::slot::toXML(xml::Element* root)
{
    root->setIntAttribute(u"id", number);
    root->setBoolAttribute(u"transmission_on", on);
}

bool ts::SAT::beam_hopping_time_plan_info_type::slot::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(number, u"id", true, 1, 1, 0x7FFF)&&
        element->getBoolAttribute(on, u"transmission_on", true, false);
}


//----------------------------------------------------------------------------
// Beam Hopping Time Plan
//----------------------------------------------------------------------------

uint16_t ts::SAT::beam_hopping_time_plan_info_type::plan_length(void) const
{
    uint16_t plan_length = 7 + time_of_application.serialized_length() + cycle_duration.serialized_length();
    switch (time_plan_mode()) {
    case HOP_1_TRANSMISSION:
        plan_length += dwell_duration.value().serialized_length() + on_time.value().serialized_length();
        break;
    case HOP_MULTI_TRANSMISSION:
        plan_length += 4
            + ((uint16_t(slot_transmission_on.size()) + padding_size_K(slot_transmission_on.size())) / 8);
        break;
    case HOP_GRID:
        plan_length += grid_size.value().serialized_length() + revisit_duration.value().serialized_length()
            + sleep_time.value().serialized_length() + sleep_duration.value().serialized_length();
        break;
    default:
        plan_length = 0;
        break;
    }
    return plan_length;
}


uint8_t ts::SAT::beam_hopping_time_plan_info_type::time_plan_mode(void) const
{
    if (dwell_duration.has_value() && on_time.has_value()) {
        return HOP_1_TRANSMISSION;
    }
    if (current_slot.has_value() && !slot_transmission_on.empty()) {
        return HOP_MULTI_TRANSMISSION;
    }
    if (grid_size.has_value() && revisit_duration.has_value() && sleep_time.has_value() && sleep_duration.has_value()) {
        return HOP_GRID;
    }
    return 99;
}


void ts::SAT::beam_hopping_time_plan_info_type::serialize(PSIBuffer& buf) const
{
    buf.putUInt32(beamhopping_time_plan_id);
    buf.putReservedZero(4);
    buf.putBits(plan_length(), 12);
    buf.putReservedZero(6);
    uint8_t _time_plan_mode = time_plan_mode();
    buf.putBits(_time_plan_mode, 2);
    time_of_application.serialize(buf);
    cycle_duration.serialize(buf);
    if (_time_plan_mode == HOP_1_TRANSMISSION) {
        dwell_duration.value().serialize(buf);
        on_time.value().serialize(buf);
    }
    else if (_time_plan_mode == HOP_MULTI_TRANSMISSION) {
        buf.putReservedZero(1);
        buf.putBits(slot_transmission_on.size(), 15);
        buf.putReservedZero(1);
        buf.putBits(current_slot.value(), 15);
        for (auto it : slot_transmission_on) {
            it.serialize(buf);
        }
        buf.putReservedZero(padding_size_K(slot_transmission_on.size()));
    }
    else if (_time_plan_mode == HOP_GRID) {
        grid_size.value().serialize(buf);
        revisit_duration.value().serialize(buf);
        sleep_time.value().serialize(buf);
        sleep_duration.value().serialize(buf);
    }
}

void ts::SAT::beam_hopping_time_plan_info_type::deserialize(PSIBuffer& buf)
{
    beamhopping_time_plan_id = buf.getUInt32();
    buf.skipBits(4);
    uint16_t beamhopping_time_plan_length;
    buf.getBits(beamhopping_time_plan_length, 12);
    buf.skipBits(6);
    uint8_t time_plan_mode;
    buf.getBits(time_plan_mode, 2);
    time_of_application.deserialize(buf);
    cycle_duration.deserialize(buf);
    if (time_plan_mode == HOP_1_TRANSMISSION) {
        NCR_type t;
        t.deserialize(buf); dwell_duration = t;
        t.deserialize(buf); on_time = t;
    }
    else if (time_plan_mode == HOP_MULTI_TRANSMISSION) {
        buf.skipBits(1);
        uint16_t bit_map_size;
        buf.getBits(bit_map_size, 15);
        buf.skipBits(1);
        buf.getBits(current_slot, 15);
        for (uint16_t i = 1; i <= bit_map_size; i++) {
            slot newSlot(i, buf);
            slot_transmission_on.push_back(newSlot);
        }
        buf.skipBits(padding_size_K(bit_map_size));
    }
    else if (time_plan_mode == HOP_GRID) {
        NCR_type t;
        t.deserialize(buf); grid_size = t;
        t.deserialize(buf); revisit_duration = t;
        t.deserialize(buf); sleep_time = t;
        t.deserialize(buf); sleep_duration = t;
    }
}

void ts::SAT::beam_hopping_time_plan_info_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"id", beamhopping_time_plan_id, true);
    time_of_application.toXML(root, u"time_of_application");
    cycle_duration.toXML(root, u"cycle_duration");

    if (time_plan_mode() == HOP_1_TRANSMISSION) {
        xml::Element* e = root->addElement(u"time_plan_mode_0");
        dwell_duration.value().toXML(e, u"dwell_duration");
        on_time.value().toXML(e, u"on_time");
    }
    else if (time_plan_mode() == HOP_MULTI_TRANSMISSION) {
        xml::Element* e = root->addElement(u"time_plan_mode_1");
        e->setOptionalIntAttribute(u"current_slot", current_slot);
        for (auto it : slot_transmission_on) {
            it.toXML(e->addElement(u"slot"));
        }
    }
    else if (time_plan_mode() == HOP_GRID) {
        xml::Element* e = root->addElement(u"time_plan_mode_2");
        grid_size.value().toXML(e, u"grid_size");
        revisit_duration.value().toXML(e, u"revisit_duration");
        sleep_time.value().toXML(e, u"sleep_time");
        sleep_duration.value().toXML(e, u"sleep_duration");
    }
}

bool ts::SAT::beam_hopping_time_plan_info_type::fromXML(const xml::Element* element)
{
    int time_plan_mode = -1;
    bool ok = element->getIntAttribute(beamhopping_time_plan_id, u"id", true) &&
        time_of_application.fromXML(element, u"time_of_application") &&
        cycle_duration.fromXML(element, u"cycle_duration");

    if (ok && element->findFirstChild(u"time_plan_mode_0", true)) {
        time_plan_mode = 0;
        NCR_type newNCR;
        const xml::Element* plan = element->findFirstChild(u"time_plan_mode_0", true);

        ok = newNCR.fromXML(plan, u"dwell_duration");
        if (ok) {
            dwell_duration = newNCR;
        }
        ok &= newNCR.fromXML(plan, u"on_time");
        if (ok) {
            on_time = newNCR;
        }
    }
    else if (ok && element->findFirstChild(u"time_plan_mode_1", true)) {
        time_plan_mode = 1;
        const xml::Element* plan = element->findFirstChild(u"time_plan_mode_1", true);
        ok = plan->getOptionalIntAttribute(current_slot, u"current_slot", 0, 0x7FFF);
        xml::ElementVector slots;
        ok &= plan->getChildren(slots, u"slot", 1, 0x7FFF);
        uint16_t highest_slot_number = 0;
        for (size_t j = 0; ok && j < slots.size(); j++) {
            beam_hopping_time_plan_info_type::slot newSlot;
            ok = newSlot.fromXML(slots[j]);
            if (ok) {
                const auto found = std::find(begin(slot_transmission_on), end(slot_transmission_on), newSlot);
                if (found != end(slot_transmission_on)) {
                    slots[j]->report().error(u"slot id=%d already specified in <%s>, line %d", { newSlot.number, plan->name(), slots[j]->lineNumber() });
                    ok = false;
                }
            }
            if (ok) {
                if (newSlot.number > highest_slot_number) {
                    highest_slot_number = newSlot.number;
                }
                slot_transmission_on.push_back(newSlot);
            }
        }
        if (ok && (highest_slot_number != slot_transmission_on.size())) {
            plan->report().error(u"not all <slot> elements specified in <%s>, line %d", { plan->name(), plan->lineNumber() });
            ok = false;
        }
    }
    else if (ok && element->findFirstChild(u"time_plan_mode_2", true)) {
        time_plan_mode = 2;
        NCR_type newNCR;
        const xml::Element* plan = element->findFirstChild(u"time_plan_mode_2", true);

        ok = newNCR.fromXML(plan, u"grid_size");
        if (ok) {
            grid_size = newNCR;
        }
        ok &= newNCR.fromXML(plan, u"revisit_duration");
        if (ok) {
            revisit_duration = newNCR;
        }
        ok &= newNCR.fromXML(plan, u"sleep_time");
        if (ok) {
            sleep_time = newNCR;
        }
        ok &= newNCR.fromXML(plan, u"sleep_duration");
        if (ok) {
            sleep_duration = newNCR;
        }
    }
    if (time_plan_mode == -1) {
        element->report().error(u"no slot type specified in <%s>, line %d", { element->name(), element->lineNumber() });
        ok = false;
    }
    return ok;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SAT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    switch (satellite_table_id) {
        case SATELLITE_POSITION_V2_INFO:
            for (auto it : satellite_position_v2_info) {
                it.serialize(buf);
            }
            break;
        case CELL_FRAGMENT_INFO:
            for (auto it : cell_fragment_info) {
                it.serialize(buf);
            }
            break;
        case TIME_ASSOCIATION_INFO:
            time_association_fragment_info.serialize(buf);
            break;
        case BEAMHOPPING_TIME_PLAN_INFO:
            for (auto it : beam_hopping_time_plan_info) {
                it.serialize(buf);
            }
            break;
        default:
            break;
   }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SAT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    const uint16_t tid_ext = section.tableIdExtension();
    satellite_table_id = (tid_ext & 0x3F00) >> 10;
    table_count = tid_ext & 0x03FF;

    switch (satellite_table_id) {
        case SATELLITE_POSITION_V2_INFO:
            while (buf.canReadBytes(4)) {
                satellite_position_v2_info_type v2inf(buf);
                satellite_position_v2_info.push_back(v2inf);
            }
            break;
        case CELL_FRAGMENT_INFO:
            while (buf.canReadBytes(4)) {
                cell_fragment_info_type cell(buf);
                cell_fragment_info.push_back(cell);
            }
            break;
        case TIME_ASSOCIATION_INFO:
            if (buf.canReadBytes(19)) {
                time_association_fragment_info.deserialize(buf);
            }
            break;
        case BEAMHOPPING_TIME_PLAN_INFO:
            while (buf.canReadBytes(19)) {
                beam_hopping_time_plan_info_type beam(buf);
                beam_hopping_time_plan_info.push_back(beam);
            }
            break;
        default:
            break;
    }
 }


//----------------------------------------------------------------------------
// A static method to display a SAT section.
//----------------------------------------------------------------------------

ts::UString ts::SAT::degrees18(uint32_t twosCompNum)
{
    using SomeType = uint64_t;
    constexpr SomeType sign_bits = ~SomeType{} << 18;
    long long int regularNum = twosCompNum & 1 << 17 ? twosCompNum | sign_bits : twosCompNum;

    return  UString::Format(u"%f", { Double(regularNum) / Double(1000) });
}

ts::UString ts::SAT::degrees19(uint32_t twosCompNum)
{
    using SomeType = uint64_t;
    constexpr SomeType sign_bits = ~SomeType{} << 19;
    long long int regularNum = twosCompNum & 1 << 18 ? twosCompNum | sign_bits : twosCompNum;

    return  UString::Format(u"%f", { Double(regularNum) / Double(1000) });
}

ts::UString ts::SAT::ncr(PSIBuffer& buf)
{
    // Network Clock Reference according to ETSI EN 301 790
    uint64_t base = buf.getBits<uint64_t>(33);
    buf.skipReservedBits(6, 0);
    uint16_t ext = buf.getBits<uint16_t>(9);
    return UString::Format(u"base=%d ext=%d NCR(%d)", { base, ext, (base * 300) + ext});
}


void ts::SAT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    // Display satellite access information
    const uint16_t tid_ext = section.tableIdExtension();
    const uint16_t _satellite_table_id = (tid_ext & 0x3F00) >> 10;
    //const uint16_t _table_count = tid_ext & 0x03FF;

    uint16_t loop;
    switch (_satellite_table_id) {
        case SATELLITE_POSITION_V2_INFO:  // Satellite Position V2 - EN 300 468, clause 5.2.11.2
            loop = 1;
            while (buf.canReadBytes(4)) {
                disp << margin << UString::Format(u"[%d] Satellite id: %06x", { loop++, buf.getBits<uint32_t>(24) });
                buf.skipReservedBits(7, 0);
                uint8_t _position_system = buf.getBits<uint8_t>(1);
                if (_position_system == POSITION_SYSTEM_GEOSTATIONARY) {
                    uint16_t _orbital_position = buf.getUInt16();
                    bool _west_east_flag = buf.getBool();
                    buf.skipReservedBits(7, 0);
                    disp << ", position: " <<
                        UString::Format(u"%d%d%d.%d ", {
                                (_orbital_position & 0xF000) >> 12,
                                (_orbital_position & 0x0F00) >> 8,
                                (_orbital_position & 0x00F0) >> 4,
                                (_orbital_position & 0x000F)}) <<
                        DataName(MY_XML_NAME, u"west_east_indicator", _west_east_flag, NamesFlags::NAME) << std::endl;
                }
                else if (_position_system == POSITION_SYSTEM_EARTH_ORBITING) {
                    uint8_t _epoch_year = buf.getUInt8();
                    uint16_t _day_of_the_year = buf.getUInt16();
                    ieee_float32_t _day_fraction = buf.getFloat32();
                    disp << ", Year: " << int(_epoch_year) << ", day: " << _day_of_the_year << ", frac: " << _day_fraction << std::endl;
                    disp << margin << "Mean motion first derivative: " << buf.getFloat32();
                    disp << ", mean motion second derivative: " << buf.getFloat32() << std::endl;
                    disp << margin << "Drag term: " << buf.getFloat32();
                    disp << ", inclination: " << buf.getFloat32();
                    disp << ", right ascention in ascending node: " << buf.getFloat32() << std::endl;
                    disp << margin << "Eccentricity: " << buf.getFloat32();
                    disp << ", argument of perigree: " << buf.getFloat32();
                    disp << ", mean anomaly:" << buf.getFloat32();
                    disp << ", mean motion: " << buf.getFloat32() << std::endl;
                }
            }
            break;
        case CELL_FRAGMENT_INFO: // Cell Fragment Info - EN 300 468, clause 5.2.11.3
            loop = 1;
            while (buf.canReadBytes(4)) {
                disp << margin << UString::Format(u"[%d] Cell fragment id: %08x", { loop++, buf.getUInt32() });
                bool _first_occurence = buf.getBool();
                bool _last_occurence = buf.getBool();
                disp << ", first: " << UString::TrueFalse(_first_occurence) << ", last: " << UString::TrueFalse(_last_occurence) << std::endl;
                if (_first_occurence) {
                    buf.skipReservedBits(4, 0);
                    disp << margin << "  Center latitude: " << degrees18(buf.getBits<uint32_t>(18));
                    buf.skipReservedBits(5, 0);
                    disp << " longitude: " << degrees19(buf.getBits<uint32_t>(19));
                    disp << ", max distance: " << buf.getUInt24() << std::endl;
                    buf.skipReservedBits(6, 0);
                }
                else {
                    buf.skipReservedBits(4, 0);
                }

                uint16_t _delivery_system_id_loop_count = buf.getBits<uint16_t>(10);
                std::vector<uint32_t> _delivery_system_ids;
                for (uint16_t j = 1; j <= _delivery_system_id_loop_count; j++) {
                    _delivery_system_ids.push_back(buf.getUInt32());
                }
                disp.displayVector(u"  Delivery system IDs:", _delivery_system_ids, margin);

                buf.skipReservedBits(6, 0);
                uint16_t new_delivery_system_id_loop_count = buf.getBits<uint16_t>(10);
                for (uint16_t k = 1; k <= new_delivery_system_id_loop_count; k++) {
                    disp << margin << "  [" << k << "] New delivery system id: " << buf.getUInt32();
                    disp << ", time of application: " << ncr(buf) << std::endl;
                }
                buf.skipReservedBits(6, 0);
                uint16_t obsolescent_delivery_system_id_loop_count = buf.getBits<uint16_t>(10);
                for (uint16_t l = 1; l <= obsolescent_delivery_system_id_loop_count; l++) {
                    disp << margin << "  [" << l << "] Obsolescent delivery system id: " << buf.getUInt32();
                    disp << ", time of obsolescence: " << ncr(buf) << std::endl;
                }
            }
            break;
        case TIME_ASSOCIATION_INFO: {  // Time Association  - EN 300 468, clause 5.2.11.4
                uint8_t _association_type = buf.getBits<uint8_t>(4);
                disp << margin << "Time association: " << DataName(MY_XML_NAME, u"UTC_mode", _association_type, NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
                if (_association_type == 1) {
                    disp << margin << "Leap 59:" << UString::TrueFalse(buf.getBool());
                    disp << ", leap 61: " << UString::TrueFalse(buf.getBool());
                    disp << ", past leap 59: " << UString::TrueFalse(buf.getBool());
                    disp << ", past leap 61: " << UString::TrueFalse(buf.getBool()) << std::endl;
                }
                else {
                    buf.skipReservedBits(4, 0);
                }
                disp << margin << "NCR time: " << ncr(buf);
                disp << ", association timestamp: seconds=" << buf.getUInt64();
                disp << " nanoseconds=" << buf.getUInt32();
            }
            break;
        case BEAMHOPPING_TIME_PLAN_INFO: // Beamhopping Time Plan - EN 300 468, clause 5.2.11.5
            loop = 1;
            while (buf.canReadBytes(19)) {
                disp << margin << UString::Format(u"[%d] Beamhopping Time Plan id: %08x", { loop++, buf.getUInt32() });
                buf.skipReservedBits(4, 0);
                buf.skipBits(12); // beamhopping_time_plan_length
                buf.skipReservedBits(6, 0);
                uint8_t _time_plan_mode = buf.getBits<uint8_t>(2);
                disp << ", mode: " << DataName(MY_XML_NAME, u"time_plan_mode", _time_plan_mode, NamesFlags::VALUE|NamesFlags::DECIMAL) << std::endl;
                disp << margin << "  Time of application: " << ncr(buf);
                disp << ", cycle duration: " << ncr(buf) << std::endl;
                if (_time_plan_mode == HOP_1_TRANSMISSION) {
                    disp << margin << "  Dwell duration: " << ncr(buf);
                    disp << ", on time: " << ncr(buf) << std::endl;
                }
                else if (_time_plan_mode == HOP_MULTI_TRANSMISSION) {
                    buf.skipBits(1);
                    uint16_t _bit_map_size = buf.getBits<int16_t>(15);
                    buf.skipBits(1);
                    disp << margin << "  Current slot: " << buf.getBits<int16_t>(15) << std::endl;

                    std::vector<bool>_slot_transmissions;
                    for (uint16_t j = 1; j <= _bit_map_size; j++) {
                        _slot_transmissions.push_back(buf.getBool());
                    }
                    disp.displayVector(u"  Slot transmission: ", _slot_transmissions, margin, false, 50, 'X', '-');

                    buf.skipBits(padding_size_K(_bit_map_size)); // fill up to byte alignment;
                }
                else if (_time_plan_mode == HOP_GRID) {
                    disp << margin << "  Grid size: " << ncr(buf);
                    disp << ", revisit duration: " << ncr(buf) << std::endl;
                    disp << margin << "  Sleep time: " << ncr(buf);
                    disp << ", sleep duration: " << ncr(buf) << std::endl;
                }
            }
            break;
        default:
            disp << margin << UString::Format(u"!! invalid satellite_table_id: %d (0x%<X)", { _satellite_table_id }) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SAT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"satellite_table_id", satellite_table_id);
    root->setIntAttribute(u"table_count", table_count);

    switch (satellite_table_id) {
        case SATELLITE_POSITION_V2_INFO:
            if (!satellite_position_v2_info.empty()) {
                xml::Element* satInfo = root->addElement(u"satellite_position_v2_info");
                for (auto it : satellite_position_v2_info) {
                    xml::Element* newSatPos = satInfo->addElement(u"satellite_position");
                    it.toXML(newSatPos);
                }
            }
            break;
        case CELL_FRAGMENT_INFO:
            if (!cell_fragment_info.empty()) {
                xml::Element* cellInfo = root->addElement(u"cell_fragment_info");
                for (auto it : cell_fragment_info) {
                    xml::Element* cellFragment = cellInfo->addElement(u"cell_fragment");
                    it.toXML(cellFragment);
                }
            }
            break;
        case TIME_ASSOCIATION_INFO: {
                xml::Element* timeAssocInfo = root->addElement(u"time_association_info");
                time_association_info_type t = time_association_fragment_info;
                t.toXML(timeAssocInfo);
            }
            break;
        case BEAMHOPPING_TIME_PLAN_INFO:
            if (!beam_hopping_time_plan_info.empty()) {
                xml::Element* beamhopInfo = root->addElement(u"beamhopping_timeplan_info");
                for (auto it : beam_hopping_time_plan_info) {
                    xml::Element* beamhopTimeplan = beamhopInfo->addElement(u"beamhopping_timeplan");
                    it.toXML(beamhopTimeplan);
                }
            }
            break;
        default:
            break;
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SAT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(version, u"version", true, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", true, true) &&
        element->getIntAttribute(satellite_table_id, u"satellite_table_id", true, 0, 0, 3) &&
        element->getIntAttribute(table_count, u"table_count", true, 0, 0x000, 0x03FF);

    if (satellite_table_id == TIME_ASSOCIATION_INFO && table_count != 0) {
        element->report().error(u"@table_count must be 0 for Time Association Info (satellite_table_id=%d) in <%s>, line %d", { satellite_table_id, element->name(), element->lineNumber()});
        ok = false;
    }

    if (ok) {
        xml::ElementVector children;
        switch (satellite_table_id) {
            case SATELLITE_POSITION_V2_INFO:
                ok &= element->getChildren(children, u"satellite_position_v2_info", 1, 1);
                if (ok) {
                    xml::ElementVector satellite_positions;
                    ok = children[0]->getChildren(satellite_positions, u"satellite_position", 1);
                    for (size_t i = 0; ok && i < satellite_positions.size(); i++) {
                        satellite_position_v2_info_type newSatellite;
                        ok = newSatellite.fromXML(satellite_positions[i]);
                        if (ok) {
                            satellite_position_v2_info.push_back(newSatellite);
                        }
                    }
                }
                break;

            case CELL_FRAGMENT_INFO:
                ok &= element->getChildren(children, u"cell_fragment_info", 1, 1);
                if (ok) {
                    xml::ElementVector cell_fragments;
                    ok = children[0]->getChildren(cell_fragments, u"cell_fragment", 1);
                    for (size_t i = 0; ok && i < cell_fragments.size(); i++) {
                        cell_fragment_info_type newCellFragment;
                        ok = newCellFragment.fromXML(cell_fragments[i]);
                        if (ok) {
                            cell_fragment_info.push_back(newCellFragment);
                        }
                    }
                }
                break;

            case TIME_ASSOCIATION_INFO:
                ok &= element->getChildren(children, u"time_association_info", 1, 1);
                ok &= time_association_fragment_info.fromXML(children[0]);
                break;

            case BEAMHOPPING_TIME_PLAN_INFO:
                ok &= element->getChildren(children, u"beamhopping_timeplan_info", 1, 1);
                if (ok) {
                    xml::ElementVector beamhopping_timeplans;
                    ok = children[0]->getChildren(beamhopping_timeplans, u"beamhopping_timeplan", 1);
                    for (size_t i = 0; ok && i < beamhopping_timeplans.size(); i++) {
                        beam_hopping_time_plan_info_type newBH;
                        ok = newBH.fromXML(beamhopping_timeplans[i]);
                        if (ok) {
                            beam_hopping_time_plan_info.push_back(newBH);
                        }
                    }
                }
                break;
            default:
                element->report().error(u"invalid @satellite_table_id (%d)  in <%s>, line %d", { satellite_table_id, element->name(), element->lineNumber()});
                ok = false;
                break;
        }
    }
    return ok;
}
