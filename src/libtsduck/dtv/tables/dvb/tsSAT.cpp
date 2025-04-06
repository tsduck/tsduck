//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2025, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSAT.h"
#include "tsPSIRepository.h"
#include "tsSatelliteDeliverySystemDescriptor.h"

#define MY_XML_NAME u"SAT"
#define MY_CLASS ts::SAT
#define MY_TID ts::TID_SAT
#define MY_PID ts::PID_SAT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});

using Double = ts::FloatingPoint<double>;

constexpr auto CHECK_UNSPECIFIED = 0;
constexpr auto CHECK_REQUIRED = 1;
constexpr auto CHECK_DISALLOWED = 2;


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
    root->setAttribute(u"orbital_position", UString::Format(u"%d.%d",  orbital_position / 10, orbital_position % 10 ));
    root->setEnumAttribute(SatelliteDeliverySystemDescriptor::DirectionNames(), u"west_east_flag", west_east_flag);
}

bool ts::SAT::satellite_position_v2_info_type::geostationary_position_type::fromXML(const xml::Element* element)
{
    UString orbit;
    bool ok = element->getAttribute(orbit, u"orbital_position", true) &&
              element->getEnumAttribute(west_east_flag, SatelliteDeliverySystemDescriptor::DirectionNames(), u"west_east_flag", true);

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
            element->report().error(u"Invalid value '%s' for attribute 'orbital_position' in <%s> at line %d, use 'nn.n'", orbit, element->name(), element->lineNumber());
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
            element->report().error(u"either <geostationary> or <earth_orbiting> must be provided in <%s>, line %d", element->name(), element->lineNumber());
            ok = false;
        }
        if (ok && (geos.size() + eos.size() != 1)) {
            element->report().error(u"only one of <geostationary> or <earth_orbiting> is permitted in <%s>, line %d", element->name(), element->lineNumber());
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

void ts::SAT::NCR_type::toXML(xml::Element* parent, const UString& element_name)
{
    toXML(parent->addElement(element_name));
}

void ts::SAT::NCR_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"base", base);
    root->setIntAttribute(u"ext", ext);
}


bool ts::SAT::NCR_type::fromXML(const xml::Element* parent, const UString& element_name)
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
    for (const auto& it : delivery_system_ids) {
        buf.putUInt32(it);
    }
    buf.putReservedZero(6);
    buf.putBits(new_delivery_system_ids.size(), 10);
    for (const auto& it : new_delivery_system_ids) {
        it.serialize(buf);
    }
    buf.putReservedZero(6);
    buf.putBits(obsolescent_delivery_system_ids.size(), 10);
    for (const auto& it : obsolescent_delivery_system_ids) {
        it.serialize(buf);
    }
}

void ts::SAT::cell_fragment_info_type::deserialize(PSIBuffer& buf)
{
    cell_fragment_id = buf.getUInt32();
    first_occurence = buf.getBool();
    last_occurence = buf.getBool();
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
    on = buf.getBool();
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
                    slots[j]->report().error(u"slot id=%d already specified in <%s>, line %d", newSlot.number, plan->name(), slots[j]->lineNumber());
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
            plan->report().error(u"not all <slot> elements specified in <%s>, line %d", plan->name(), plan->lineNumber());
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
        element->report().error(u"no slot type specified in <%s>, line %d", element->name(), element->lineNumber());
        ok = false;
    }
    return ok;
}


//----------------------------------------------------------------------------
// Satellite position v3 - Non geostationary satellite
//----------------------------------------------------------------------------

void ts::SAT::satellite_position_v3_info_type::v3_satellite_time::serialize(PSIBuffer& buf) const
{
    buf.putUInt8(year);
    buf.putBits(0x00, 7);
    buf.putBits(day, 9);
    buf.putFloat32(day_fraction);
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_time::deserialize(PSIBuffer& buf)
{
    year = buf.getUInt8();
    buf.skipBits(7);
    day = buf.getBits<uint16_t>(9);
    day_fraction = buf.getFloat32();
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_time::toXML(xml::Element* root)
{
    root->setIntAttribute(u"year", year, false);
    root->setIntAttribute(u"day", day, false);
    root->setFloatAttribute(u"day_fraction", day_fraction);
}

bool ts::SAT::satellite_position_v3_info_type::v3_satellite_time::fromXML(const xml::Element* element, const UString& name)
{
    xml::ElementVector named_children;
    return element->getChildren(named_children, name, 1, 1) &&
           named_children[0]->getIntAttribute(year, u"year", true, 0, 0, 99) &&
           named_children[0]->getIntAttribute(day, u"day", true, 1, 1, 366) &&
           named_children[0]->getFloatAttribute(day_fraction, u"day_fraction", true, 0, 0, 1.0);
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_time::display(TablesDisplay& disp, PSIBuffer& buf) {
    disp << "(year=" << int(buf.getUInt8());
    buf.skipReservedBits(7, 0);
    disp << ", day=" << buf.getBits<uint16_t>(9);
    disp << ", fraction=" <<UString::Float(double(buf.getFloat32())) << ")";
}


void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_metadata_type::serialize(PSIBuffer& buf) const
{
    total_start_time.serialize(buf);
    total_stop_time.serialize(buf);
    buf.putBit(0);
    buf.putBit(interpolation_type.has_value() && interpolation_degree.has_value());
    buf.putBits(interpolation_type.value_or(0), 3);
    buf.putBits(interpolation_degree.value_or(0), 3);
    if (usable_start_time.has_value()) {
        usable_start_time.value().serialize(buf);
    }
    if (usable_stop_time.has_value()) {
        usable_stop_time.value().serialize(buf);
    }
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_metadata_type::deserialize(PSIBuffer& buf, bool usable_start_time_flag, bool usable_stop_time_flag)
{
    total_start_time.deserialize(buf);
    total_stop_time.deserialize(buf);
    buf.skipBits(1);
    buf.skipBits(1);        // interpolation_flag
    interpolation_type = buf.getBits<uint8_t>(3);
    interpolation_degree = buf.getBits<uint8_t>(3);
    if (usable_start_time_flag) {
        v3_satellite_time t(buf);
        usable_start_time = t;
    }
    if (usable_stop_time_flag) {
        v3_satellite_time t(buf);
        usable_stop_time = t;
    }
}

// Thread-safe init-safe static data patterns.
const ts::Names& ts::SAT::InterpolationTypes()
{
    static const Names data({
        {u"Linear",   1},
        {u"Lagrange", 2},
        {u"Hermite",  4},
    });
    return data;
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_metadata_type::toXML(xml::Element* root)
{
    total_start_time.toXML(root->addElement(u"total_start_time"));
    total_stop_time.toXML(root->addElement(u"total_stop_time"));
    if (interpolation_type.has_value() && interpolation_degree.has_value()) {
        root->setEnumAttribute(InterpolationTypes(), u"interpolation_type", interpolation_type.value());
        root->setIntAttribute(u"interpolation_degree", interpolation_degree.value());
    }
    if (usable_start_time.has_value()) {
        usable_start_time.value().toXML(root->addElement(u"usable_start_time"));
    }
    if (usable_stop_time.has_value()) {
        usable_stop_time.value().toXML(root->addElement(u"usable_stop_time"));
    }
}

bool ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_metadata_type::fromXML(const xml::Element* element)
{
    v3_satellite_time t;
    uint8_t           t_interpolation_type = 0;
    uint8_t           t_interpolation_degree = 0;
    bool              ok = total_start_time.fromXML(element, u"total_start_time") &&
                           total_stop_time.fromXML(element, u"total_stop_time") &&
                           element->getEnumAttribute(t_interpolation_type, InterpolationTypes(), u"interpolation_type", true) &&
                           element->getIntAttribute(t_interpolation_degree, u"interpolation_degree", true, 0, 0, 7);
    if (ok) {
        interpolation_type = t_interpolation_type;
        interpolation_degree = t_interpolation_degree;
    }
    if (element->hasChildElement(u"usable_start_time")) {
        if (t.fromXML(element, u"usable_start_time")) {
            usable_start_time = t;
        }
        else {
            ok = false;
        }
    }
    if (element->hasChildElement(u"usable_stop_time")) {
        if (t.fromXML(element, u"usable_stop_time")) {
            usable_stop_time = t;
        }
        else {
            ok = false;
        }
    }
    return ok;
}


void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_ephemeris_data_type::serialize(PSIBuffer& buf) const
{
    epoch.serialize(buf);
    buf.putFloat32(ephemeris_x);
    buf.putFloat32(ephemeris_y);
    buf.putFloat32(ephemeris_z);
    buf.putFloat32(ephemeris_x_dot);
    buf.putFloat32(ephemeris_y_dot);
    buf.putFloat32(ephemeris_z_dot);
    if (ephemeris_x_ddot.has_value() && ephemeris_y_ddot.has_value() && ephemeris_z_ddot.has_value()) {
        buf.putFloat32(ephemeris_x_ddot.value());
        buf.putFloat32(ephemeris_y_ddot.value());
        buf.putFloat32(ephemeris_z_ddot.value());
    }
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_ephemeris_data_type::deserialize(PSIBuffer& buf, bool ephemeris_accel_flag)
{
    epoch.deserialize(buf);
    ephemeris_x = buf.getFloat32();
    ephemeris_y = buf.getFloat32();
    ephemeris_z = buf.getFloat32();
    ephemeris_x_dot = buf.getFloat32();
    ephemeris_y_dot = buf.getFloat32();
    ephemeris_z_dot = buf.getFloat32();
    if (ephemeris_accel_flag) {
        ephemeris_x_ddot = buf.getFloat32();
        ephemeris_y_ddot = buf.getFloat32();
        ephemeris_z_ddot = buf.getFloat32();
    }
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_ephemeris_data_type::toXML(xml::Element* root)
{
    epoch.toXML(root->addElement(u"epoch"));
    root->setFloatAttribute(u"ephemeris_x", ephemeris_x);
    root->setFloatAttribute(u"ephemeris_y", ephemeris_y);
    root->setFloatAttribute(u"ephemeris_z", ephemeris_z);
    root->setFloatAttribute(u"ephemeris_x_dot", ephemeris_x_dot);
    root->setFloatAttribute(u"ephemeris_y_dot", ephemeris_y_dot);
    root->setFloatAttribute(u"ephemeris_z_dot", ephemeris_z_dot);
    if (ephemeris_x_ddot.has_value() && ephemeris_y_ddot.has_value() && ephemeris_z_ddot.has_value()) {
        root->setFloatAttribute(u"ephemeris_x_ddot", ephemeris_x_ddot.value());
        root->setFloatAttribute(u"ephemeris_y_ddot", ephemeris_y_ddot.value());
        root->setFloatAttribute(u"ephemeris_z_ddot", ephemeris_z_ddot.value());
    }
}

bool ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_ephemeris_data_type::fromXML(const xml::Element* element, uint8_t& ephemeris_accel_check_type)
{
    bool ok = epoch.fromXML(element, u"epoch") &&
              element->getFloatAttribute(ephemeris_x, u"ephemeris_x", true) &&
              element->getFloatAttribute(ephemeris_y, u"ephemeris_y", true) &&
              element->getFloatAttribute(ephemeris_z, u"ephemeris_z", true) &&
              element->getFloatAttribute(ephemeris_x_dot, u"ephemeris_x_dot", true) &&
              element->getFloatAttribute(ephemeris_y_dot, u"ephemeris_y_dot", true) &&
              element->getFloatAttribute(ephemeris_z_dot, u"ephemeris_z_dot", true) &&
              element->getOptionalFloatAttribute(ephemeris_x_ddot, u"ephemeris_x_ddot") &&
              element->getOptionalFloatAttribute(ephemeris_y_ddot, u"ephemeris_y_ddot") &&
              element->getOptionalFloatAttribute(ephemeris_z_ddot, u"ephemeris_z_ddot");
    uint8_t optional_count = ephemeris_x_ddot.has_value() + ephemeris_y_ddot.has_value() + ephemeris_z_ddot.has_value();
    if (optional_count != 0 && optional_count != 3) {
        element->report().error(u"all or none of the ephemeris acceleration values (ddot values x, y and z) must be specified in <%s>, line %d", element->name(), element->lineNumber());
        ok = false;
    }
    switch (ephemeris_accel_check_type) {
        case CHECK_UNSPECIFIED:
            // first time through - set the state for remainder of <empheris_data> elements
            ephemeris_accel_check_type = (optional_count == 3) ? CHECK_REQUIRED : CHECK_DISALLOWED;
            break;
        case CHECK_REQUIRED:
            if (optional_count != 3) {
                element->report().error(u"ephemeris acceleration values (x_ddot, y_ddot and z_ddot) must be specified in <%s>, line %d", element->name(), element->lineNumber());
                ok = false;
            }
            break;
        case CHECK_DISALLOWED:
            if (optional_count != 0) {
                element->report().error(u"ephemeris acceleration values (x_ddot, y_ddot and z_ddot) must not be specified in <%s>, line %d", element->name(), element->lineNumber());
                ok = false;
            }
            break;
        default:
            element->report().severe(u"unhandled ephemeris_accel_check_type value(%d) in v3_satellite_ephemeris_data_type::fromXML", ephemeris_accel_check_type);
            ok = false;
            break;
    }
    return ok;
}


void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_covariance_data_type::serialize(PSIBuffer& buf) const
{
    covariace_epoch.serialize(buf);
    for (auto j : covariance_element) {
        buf.putFloat32(j);
    }
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_covariance_data_type::deserialize(PSIBuffer& buf)
{
    covariace_epoch.deserialize(buf);
    for (auto j = 0; j < NUM_COVARIANCE_ELEMENTS; j++) {
        covariance_element.push_back(buf.getFloat32());
    }
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_covariance_data_type::toXML(xml::Element* root)
{
    covariace_epoch.toXML(root->addElement(u"epoch"));
    for (auto j : covariance_element) {
        root->addElement(u"element")->addText(UString::Float(double(j)));
    }
}

bool ts::SAT::satellite_position_v3_info_type::v3_satellite_type::v3_satellite_covariance_data_type::fromXML(const xml::Element* element)
{
    xml::ElementVector covariance_elements;
    bool ok = covariace_epoch.fromXML(element, u"epoch") &&
              element->getChildren(covariance_elements, u"element", NUM_COVARIANCE_ELEMENTS, NUM_COVARIANCE_ELEMENTS);
    if (ok) {
        for (auto it : covariance_elements) {
            ieee_float32_t x = 0;
            ts::UString    text;
            if (it->getText(text) && text.toFloat(x)) {
                covariance_element.push_back(x);
            }
            else {
                element->report().error(u"Covariance element must be a float (found %s) in <%s>, line %d", text, element->name(), element->lineNumber());
                ok = false;
            }
        }
    }
    return ok;
}

bool ts::SAT::satellite_position_v3_info_type::v3_satellite_type::hasEphemerisAcceleration()
{
    bool rc = !ephemeris_data.empty();
    rc &= ephemeris_data[0].hasAcceleration();
    // we only need to check the first ephemeris element as all elements are the same.
    // for (size_t i = 0; rc && i < ephemeris_data.size(); i++) {
    //    rc &= ephemeris_data[i].hasAcceleration();
    //}
    return rc;
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::serialize(PSIBuffer& buf)
{
    buf.putUInt24(satellite_id);
    buf.putBits(0x00, 3);
    buf.putBit(metadata.has_value());
    buf.putBit(metadata.has_value() && metadata.value().usable_start_time.has_value());  // usable_start_time_flag
    buf.putBit(metadata.has_value() && metadata.value().usable_stop_time.has_value());  // usable_stop_time_flag
    buf.putBit(hasEphemerisAcceleration2());  // ephemeris_accel_flag
    buf.putBit(covariance.has_value());  //covariance_flag
    if (metadata.has_value()) {
        metadata.value().serialize(buf);
    }
    buf.putBits(ephemeris_data.size(), 16);
    for (const auto& it : ephemeris_data) {
        it.serialize(buf);
    }
    if (covariance.has_value()) {
        covariance.value().serialize(buf);
    }
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::deserialize(PSIBuffer& buf)
{
    satellite_id = buf.getUInt24();
    buf.skipBits(3);
    bool metadata_flag = buf.getBool();
    bool usable_start_time_flag = buf.getBool();
    bool usable_stop_time_flag = buf.getBool();
    bool ephemeris_accel_flag = buf.getBool();
    bool covariance_flag = buf.getBool();
    if (metadata_flag) {
        v3_satellite_metadata_type sat_metadata(buf, usable_start_time_flag, usable_stop_time_flag);
        metadata = sat_metadata;
    }
    uint16_t ephemeris_data_count = buf.getUInt16();
    for (auto i = 0; (i < ephemeris_data_count) && buf.canReadBytes(31); i++) {
        v3_satellite_ephemeris_data_type e(buf, ephemeris_accel_flag);
        ephemeris_data.push_back(e);
    }
    if (covariance_flag) {
        v3_satellite_covariance_data_type c(buf);
        covariance = c;
    }
}

void ts::SAT::satellite_position_v3_info_type::v3_satellite_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"satellite_id", satellite_id, true);
    if (metadata.has_value()) {
        metadata.value().toXML(root);
    }
    for (auto it : ephemeris_data) {
        it.toXML(root->addElement(u"ephemeris_data"));
    }
    if (covariance.has_value()) {
        covariance.value().toXML(root->addElement(u"covariance"));
    }
}

bool ts::SAT::satellite_position_v3_info_type::v3_satellite_type::fromXML(const xml::Element* element)
{
    xml::ElementVector em_data, cov_element;
    bool ok = element->getIntAttribute(satellite_id, u"satellite_id", true, 0, 0, 0xFFFFFF) &&
              element->getChildren(em_data, u"ephemeris_data", 0, 0xFFFF) &&
              element->getChildren(cov_element, u"covariance", 0, 1);
    if (element->hasChildElement(u"total_start_time") && element->hasChildElement(u"total_stop_time")) {
        v3_satellite_metadata_type m;
        if (m.fromXML(element)) {
            metadata = m;
        }
        else {
            ok = false;
        }
    }
    uint8_t ephemeris_accel_check = CHECK_UNSPECIFIED;
    for (auto it : em_data) {
        v3_satellite_ephemeris_data_type t;
        if (t.fromXML(it, ephemeris_accel_check)) {
            ephemeris_data.push_back(t);
        }
        else {
            ok = false;
        }
    }
    if (!cov_element.empty()) {
        v3_satellite_covariance_data_type c;
        if (c.fromXML(cov_element[0])) {
            covariance = c;
        }
        else {
            ok = false;
        }
    }
    return ok;
}

void ts::SAT::satellite_position_v3_info_type::serialize(PSIBuffer& buf) const
{
    buf.putBits(oem_version_major, 4);
    buf.putBits(oem_version_minor, 4);
    creation_date.serialize(buf);
    for (auto it : v3_satellites) {
        it.serialize(buf);
    }
}

void ts::SAT::satellite_position_v3_info_type::deserialize(PSIBuffer& buf)
{
    oem_version_major = buf.getBits<uint8_t>(4);
    oem_version_minor = buf.getBits<uint8_t>(4);
    creation_date.deserialize(buf);
    while (buf.canReadBytes(6)) {
        v3_satellite_type new_v3_sat(buf);
        v3_satellites.push_back(new_v3_sat);
    }
}

void ts::SAT::satellite_position_v3_info_type::toXML(xml::Element* root)
{
    root->setIntAttribute(u"oem_version_major", oem_version_major);
    root->setIntAttribute(u"oem_version_minor", oem_version_minor);
    creation_date.toXML(root->addElement(u"creation_date"));
    for (auto it : v3_satellites) {
        it.toXML(root->addElement(u"v3_satellite"));
    }
}

bool ts::SAT::satellite_position_v3_info_type::fromXML(const xml::Element* element)
{
    xml::ElementVector satellites;
    bool ok = element->getIntAttribute(oem_version_major, u"oem_version_major", true, 0, 0, 0xF) &&
              element->getIntAttribute(oem_version_minor, u"oem_version_minor", true, 0, 0, 0xF) &&
              creation_date.fromXML(element, u"creation_date") &&
              element->getChildren(satellites, u"v3_satellite");

    for (auto it : satellites) {
        v3_satellite_type new_sat;
        if (new_sat.fromXML(it)) {
            v3_satellites.push_back(new_sat);
        }
        else {
            ok = false;
        }
    }
    return ok;
}

void ts::SAT::satellite_position_v3_info_type::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "OEM Version: major=" << int(buf.getBits<uint8_t>(4));
    disp << ", minor=" << int(buf.getBits<uint8_t>(4));
    disp << ", creation date: ";
    v3_satellite_time::display(disp, buf);
    disp << std::endl;
    uint16_t satellite_index = 0;
    while (buf.canReadBytes(6)) {
        disp << margin << "Satellite [" << satellite_index++ << "] id: " << UString::Hexa(buf.getUInt24(), 6) << std::endl;
        buf.skipReservedBits(3, 0);
        bool metadata_flag = buf.getBool();
        bool usable_start_time_flag = buf.getBool();
        bool usable_stop_time_flag = buf.getBool();
        bool ephemeris_accel_flag = buf.getBool();
        bool covariance_flag = buf.getBool();
        if (metadata_flag) {
            disp << margin << " Total start: ";
            v3_satellite_time::display(disp, buf);
            disp << ", total stop: ";
            v3_satellite_time::display(disp, buf);
            disp << std::endl;
            buf.skipReservedBits(1, 0);
            bool _interpolation_flag = buf.getBool();
            disp << margin << " Interpolation: " << UString::TrueFalse(_interpolation_flag);
            if (_interpolation_flag) {
                disp << ", type: " << DataName(MY_XML_NAME, u"interpolation_type", buf.getBits<uint8_t>(3), NamesFlags::NAME);
                disp << ", degree: " << int(buf.getBits<uint8_t>(3));
            }
            else {
                buf.skipBits(6);
            }
            disp << std::endl;
            if (usable_start_time_flag) {
                disp << margin << " Usable start time: ";
                v3_satellite_time::display(disp, buf);
            }
            if (usable_stop_time_flag) {
                disp << (usable_start_time_flag ? ", u" : " U") << "sable end time: ";
                v3_satellite_time::display(disp, buf);
            }
            if (usable_start_time_flag || usable_stop_time_flag) {
                disp << std::endl;
            }
        }
        uint16_t ephemeris_data_count = buf.getUInt16();
        for (uint16_t j = 0; j < ephemeris_data_count; j++) {
            disp << margin << " Ephemeris data [" << j << "] epoch: ";
            v3_satellite_time::display(disp, buf);
            disp << std::endl;
            ieee_float32_t x = buf.getFloat32();  //ephemeris_x
            ieee_float32_t y = buf.getFloat32();  //ephemeris_y
            ieee_float32_t z = buf.getFloat32();  //ephemeris_z
            disp << margin << UString::Format(u"Position x: %f, y: %f, z: %f", double(x), double(y), double(z));
            x = buf.getFloat32();  //ephemeris_x_dot
            y = buf.getFloat32();  //ephemeris_y_dot
            z = buf.getFloat32();  //ephemeris_z_dot
            disp << UString::Format(u", Velocity x: %f, y: %f, z: %f", double(x), double(y), double(z)) << std::endl;
            if (ephemeris_accel_flag) {
                x = buf.getFloat32();  //ephemeris_x_ddot
                y = buf.getFloat32();  //ephemeris_y_ddot
                z = buf.getFloat32();  //ephemeris_z_ddot
                disp << margin << UString::Format(u"Acceleration x: %f, y: %f, z: %f ", double(x), double(y), double(z)) << std::endl;
            }
        }
        if (covariance_flag) {
            disp << margin << " Covariance epoch: ";
            v3_satellite_time::display(disp, buf);
            disp << std::endl;
            UStringVector covariance_element;
            const UString _zero = UString::Float(0);
            for (auto j = 1; j <= NUM_COVARIANCE_ELEMENTS; j++) {
                covariance_element.push_back(UString::Float(double(buf.getFloat32())));
                if (j == 1) {
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                }
                else if (j == 3) {
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                }
                else if (j == 6) {
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                }
                else if (j == 10) {
                    covariance_element.push_back(_zero);
                    covariance_element.push_back(_zero);
                }
                else if (j == 15) {
                    covariance_element.push_back(_zero);
                }
            }
            disp.displayVector(u" Covariance matrix:", covariance_element, margin, true, 6);
        }
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SAT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    switch (satellite_table_id) {
        case SATELLITE_POSITION_V2_INFO:
            for (const auto& it : satellite_position_v2_info) {
                it.serialize(buf);
            }
            break;
        case CELL_FRAGMENT_INFO:
            for (const auto& it : cell_fragment_info) {
                it.serialize(buf);
            }
            break;
        case TIME_ASSOCIATION_INFO:
            time_association_fragment_info.serialize(buf);
            break;
        case BEAMHOPPING_TIME_PLAN_INFO:
            for (const auto& it : beam_hopping_time_plan_info) {
                it.serialize(buf);
            }
            break;
        case SATELLITE_POSITION_V3_INFO:
            if (satellite_position_v3_info.has_value()) {
                satellite_position_v3_info.value().serialize(buf);
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
        case SATELLITE_POSITION_V3_INFO:
            while (buf.canReadBytes(4)) {
                satellite_position_v3_info_type v3inf(buf);
                satellite_position_v3_info = v3inf;
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

    return UString::Format(u"%f", Double(regularNum) / Double(1000));
}

ts::UString ts::SAT::degrees19(uint32_t twosCompNum)
{
    using SomeType = uint64_t;
    constexpr SomeType sign_bits = ~SomeType{} << 19;
    long long int regularNum = twosCompNum & 1 << 18 ? twosCompNum | sign_bits : twosCompNum;

    return UString::Format(u"%f", Double(regularNum) / Double(1000));
}

ts::UString ts::SAT::ncr(PSIBuffer& buf)
{
    // Network Clock Reference according to ETSI EN 301 790
    uint64_t base = buf.getBits<uint64_t>(33);
    buf.skipReservedBits(6, 0);
    uint16_t ext = buf.getBits<uint16_t>(9);
    return UString::Format(u"base=%d ext=%d NCR(%d)",  base, ext, (base * 300) + ext);
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
                disp << margin << UString::Format(u"[%d] Satellite id: %06x", loop++, buf.getBits<uint32_t>(24));
                buf.skipReservedBits(7, 0);
                uint8_t _position_system = buf.getBits<uint8_t>(1);
                if (_position_system == POSITION_SYSTEM_GEOSTATIONARY) {
                    uint16_t _orbital_position = buf.getUInt16();
                    bool     _west_east_flag = buf.getBool();
                    buf.skipReservedBits(7, 0);
                    disp << ", position: "
                         << UString::Format(u"%d%d%d.%d ",
                                            (_orbital_position & 0xF000) >> 12,
                                            (_orbital_position & 0x0F00) >> 8,
                                            (_orbital_position & 0x00F0) >> 4,
                                            (_orbital_position & 0x000F))
                         << DataName(MY_XML_NAME, u"west_east_indicator", _west_east_flag, NamesFlags::NAME) << std::endl;
                }
                else if (_position_system == POSITION_SYSTEM_EARTH_ORBITING) {
                    uint8_t        _epoch_year = buf.getUInt8();
                    uint16_t       _day_of_the_year = buf.getUInt16();
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
        case CELL_FRAGMENT_INFO:  // Cell Fragment Info - EN 300 468, clause 5.2.11.3
            loop = 1;
            while (buf.canReadBytes(4)) {
                disp << margin << UString::Format(u"[%d] Cell fragment id: %08x", loop++, buf.getUInt32());
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

                uint16_t              _delivery_system_id_loop_count = buf.getBits<uint16_t>(10);
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
            disp << margin << "Time association: " << DataName(MY_XML_NAME, u"UTC_mode", _association_type, NamesFlags::NAME_VALUE | NamesFlags::DECIMAL) << std::endl;
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
        } break;
        case BEAMHOPPING_TIME_PLAN_INFO:  // Beamhopping Time Plan - EN 300 468, clause 5.2.11.5
            loop = 1;
            while (buf.canReadBytes(19)) {
                disp << margin << UString::Format(u"[%d] Beamhopping Time Plan id: %08x", loop++, buf.getUInt32());
                buf.skipReservedBits(4, 0);
                buf.skipBits(12);  // beamhopping_time_plan_length
                buf.skipReservedBits(6, 0);
                uint8_t _time_plan_mode = buf.getBits<uint8_t>(2);
                disp << ", mode: " << DataName(MY_XML_NAME, u"time_plan_mode", _time_plan_mode, NamesFlags::NAME_VALUE | NamesFlags::DECIMAL) << std::endl;
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

                    std::vector<bool> _slot_transmissions;
                    for (uint16_t j = 1; j <= _bit_map_size; j++) {
                        _slot_transmissions.push_back(buf.getBool());
                    }
                    disp.displayVector(u"  Slot transmission: ", _slot_transmissions, margin, false, 50, 'X', '-');

                    buf.skipBits(padding_size_K(_bit_map_size));  // fill up to byte alignment;
                }
                else if (_time_plan_mode == HOP_GRID) {
                    disp << margin << "  Grid size: " << ncr(buf);
                    disp << ", revisit duration: " << ncr(buf) << std::endl;
                    disp << margin << "  Sleep time: " << ncr(buf);
                    disp << ", sleep duration: " << ncr(buf) << std::endl;
                }
            }
            break;
        case SATELLITE_POSITION_V3_INFO: {
                satellite_position_v3_info_type v3;
                v3.display(disp, buf, margin);
            }
            break;
        default:
            disp << margin << UString::Format(u"!! invalid satellite_table_id: %n",  _satellite_table_id ) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SAT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"satellite_table_id", satellite_table_id);
    root->setIntAttribute(u"table_count", table_count);

    switch (satellite_table_id) {
        case SATELLITE_POSITION_V2_INFO:
            if (!satellite_position_v2_info.empty()) {
                xml::Element* satInfo = root->addElement(u"satellite_position_v2_info");
                for (auto it : satellite_position_v2_info) {
                    it.toXML(satInfo->addElement(u"satellite_position"));
                }
            }
            break;
        case CELL_FRAGMENT_INFO:
            if (!cell_fragment_info.empty()) {
                xml::Element* cellInfo = root->addElement(u"cell_fragment_info");
                for (auto it : cell_fragment_info) {
                    it.toXML(cellInfo->addElement(u"cell_fragment"));
                }
            }
            break;
        case TIME_ASSOCIATION_INFO: {
                time_association_info_type t = time_association_fragment_info;
                t.toXML(root->addElement(u"time_association_info"));
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
        case SATELLITE_POSITION_V3_INFO:
            if (satellite_position_v3_info.has_value()) {
                satellite_position_v3_info_type t = satellite_position_v3_info.value();
                t.toXML(root->addElement(u"satellite_position_v3_info"));
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
        element->getIntAttribute(_version, u"version", true, 0, 0, 31) &&
        element->getBoolAttribute(_is_current, u"current", true, true) &&
        element->getIntAttribute(satellite_table_id, u"satellite_table_id", true, 0, satellite_table_id_min, satellite_table_id_max) &&
        element->getIntAttribute(table_count, u"table_count", true, 0, 0x000, 0x03FF);

    if (satellite_table_id == TIME_ASSOCIATION_INFO && table_count != 0) {
        element->report().error(u"@table_count must be 0 for Time Association Info (satellite_table_id=%d) in <%s>, line %d", satellite_table_id, element->name(), element->lineNumber());
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

            case SATELLITE_POSITION_V3_INFO:
                ok &= element->getChildren(children, u"satellite_position_v3_info", 1, 1);
                if (ok) {
                    satellite_position_v3_info_type v3_system;
                    if (v3_system.fromXML(children[0])) {
                        satellite_position_v3_info = v3_system;
                    }
                    else {
                        ok = false;
                    }
                }
                break;

            default:
                element->report().error(u"invalid @satellite_table_id (%d)  in <%s>, line %d", satellite_table_id, element->name(), element->lineNumber());
                ok = false;
                break;
        }
    }
    return ok;
}
