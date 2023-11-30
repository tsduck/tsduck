//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsService.h"


//----------------------------------------------------------------------------
// Reset using a string description.
// If the string evaluates to an integer (decimal or hexa),
// this is a service id, otherwise this is a service name.
//----------------------------------------------------------------------------

void ts::Service::set(const UString& desc)
{
    clear();

    uint16_t id = 0;
    uint16_t minor = 0;

    if (desc.toInteger(id, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
        // Found a service id.
        setId(id);
    }
    else if (desc.scan(u"%d.%d", {&id, &minor})) {
        // Found an ATSC major.minor id.
        setMajorIdATSC(id);
        setMinorIdATSC(minor);
    }
    else if (!desc.empty()) {
        // Finally, just a service name.
        setName(desc);
    }
}


//----------------------------------------------------------------------------
// Check if a service matches a string identification.
//----------------------------------------------------------------------------

bool ts::Service::match(const UString& ident, bool exact_match) const
{
    uint16_t id = 0;
    uint16_t minor = 0;

    if (ident.toInteger(id, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
        // This is a service id.
        return _id.has_value() && id ==_id.value();
    }
    else if (ident.scan(u"%d.%d", {&id, &minor})) {
        // Found an ATSC major.minor id.
        return _major_id_atsc.has_value() && _minor_id_atsc.has_value() && id == _major_id_atsc.value() && minor == _minor_id_atsc.value();
    }
    else if (exact_match) {
        // This is an exact service name.
        return _name.has_value() && ident == _name.value();
    }
    else {
        // This is a fuzzy service name.
        return _name.has_value() && ident.similar(_name.value());
    }
}


//----------------------------------------------------------------------------
// Clear all fields
//----------------------------------------------------------------------------

void ts::Service::clear()
{
    clearId();
    clearTSId();
    clearONId();
    clearPMTPID();
    clearLCN();
    clearTypeDVB();
    clearTypeATSC();
    clearName();
    clearProvider();
    clearEITsPresent();
    clearEITpfPresent();
    clearCAControlled();
    clearRunningStatus();
    clearMajorIdATSC();
    clearMajorIdATSC();
}


//----------------------------------------------------------------------------
// List of fields which are set in a Service
//----------------------------------------------------------------------------

uint32_t ts::Service::getFields() const
{
    uint32_t fields = 0;
    if (_id.has_value()) {
        fields |= ID;
    }
    if (_tsid.has_value()) {
        fields |= TSID;
    }
    if (_onid.has_value()) {
        fields |= ONID;
    }
    if (_pmt_pid.has_value()) {
        fields |= PMT_PID;
    }
    if (_lcn.has_value()) {
        fields |= LCN;
    }
    if (_type_dvb.has_value()) {
        fields |= TYPE_DVB;
    }
    if (_type_atsc.has_value()) {
        fields |= TYPE_ATSC;
    }
    if (_name.has_value()) {
        fields |= NAME;
    }
    if (_provider.has_value()) {
        fields |= PROVIDER;
    }
    if (_eits_present.has_value()) {
        fields |= EITS;
    }
    if (_eitpf_present.has_value()) {
        fields |= EITPF;
    }
    if (_ca_controlled.has_value()) {
        fields |= CA;
    }
    if (_running_status.has_value()) {
        fields |= RUNNING;
    }
    if (_major_id_atsc.has_value()) {
        fields |= MAJORID_ATSC;
    }
    if (_minor_id_atsc.has_value()) {
        fields |= MINORID_ATSC;
    }
    return fields;
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::Service::toString() const
{
    UString str;
    if (_name.has_value()) {
        str = u"\"" + _name.value() + u"\"";
    }
    if (_major_id_atsc.has_value() && _minor_id_atsc.has_value()) {
        str.format(u"%s%d.%d", {str.empty() ? u"" : u", ", _major_id_atsc.value(), _minor_id_atsc.value()});
    }
    if (_id.has_value()) {
        str.format(u"%s0x%X (%d)", {str.empty() ? u"" : u", ", _id.value(), _id.value()});
    }
    if (_lcn.has_value()) {
        str.format(u"%s#%d", {str.empty() ? u"" : u", ", _lcn.value()});
    }
    if (_hidden.has_value() && *_hidden) {
        str.format(u"%s(hidden)", {str.empty() ? u"" : u" "});
    }
    return str;
}


//----------------------------------------------------------------------------
// Sorting criteria
//----------------------------------------------------------------------------

// Sort macro according to one field: If both objects have this field set,
// sort according to this field. If only one object has this field set,
// it comes first. If none of the two objects have this field set, move
// to next criterion.
#define _SORT_(field)                                     \
    if (s1.field.has_value() && !s2.field.has_value()) {  \
        return true;                                      \
    }                                                     \
    if (!s1.field.has_value() && s2.field.has_value()) {  \
        return false;                                     \
    }                                                     \
    if (s1.field.has_value() && s2.field.has_value() && s1.field.value() != s2.field.value()) { \
        return s1.field.value() < s2.field.value();       \
    }

// Sort1: LCN, ONId, TSId, Id, name, provider, type, PMT PID
bool ts::Service::Sort1 (const Service& s1, const Service& s2)
{
    _SORT_(_lcn)
    _SORT_(_onid)
    _SORT_(_tsid)
    _SORT_(_id)
    _SORT_(_name)
    _SORT_(_provider)
    _SORT_(_type_dvb)
    _SORT_(_type_atsc)
    _SORT_(_pmt_pid)
    return true; // Default: remain stable
}

// Sort2: name, provider, LCN, ONId, TSId, Id, type, PMT PID
bool ts::Service::Sort2 (const Service& s1, const Service& s2)
{
    _SORT_(_name)
    _SORT_(_provider)
    _SORT_(_lcn)
    _SORT_(_onid)
    _SORT_(_tsid)
    _SORT_(_id)
    _SORT_(_type_dvb)
    _SORT_(_type_atsc)
    _SORT_(_pmt_pid)
    return true; // Default: remain stable
}

// Sort3: ONId, TSId, Id, type, name, provider, LCN, PMT PID
bool ts::Service::Sort3 (const Service& s1, const Service& s2)
{
    _SORT_(_onid)
    _SORT_(_tsid)
    _SORT_(_id)
    _SORT_(_type_dvb)
    _SORT_(_type_atsc)
    _SORT_(_name)
    _SORT_(_provider)
    _SORT_(_lcn)
    _SORT_(_pmt_pid)
    return true; // Default: remain stable
}
