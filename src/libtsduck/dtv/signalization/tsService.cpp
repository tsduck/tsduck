//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsService.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::Service::Service() :
    _modified(false),
    _id(),
    _tsid(),
    _onid(),
    _pmt_pid(),
    _lcn(),
    _type_dvb(),
    _type_atsc(),
    _name(),
    _provider(),
    _eits_present(),
    _eitpf_present(),
    _ca_controlled(),
    _running_status(),
    _major_id_atsc(),
    _minor_id_atsc()
{
}

ts::Service::Service(uint16_t id) :
    Service()
{
    _id = id;
}

ts::Service::Service(const UString& desc) :
    Service()
{
    set(desc);
}

ts::Service::~Service()
{
}


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
        return _id.set() && id ==_id.value();
    }
    else if (ident.scan(u"%d.%d", {&id, &minor})) {
        // Found an ATSC major.minor id.
        return _major_id_atsc.set() && _minor_id_atsc.set() && id == _major_id_atsc.value() && minor == _minor_id_atsc.value();
    }
    else if (exact_match) {
        // This is an exact service name.
        return _name.set() && ident == _name.value();
    }
    else {
        // This is a fuzzy service name.
        return _name.set() && ident.similar(_name.value());
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
    if (_id.set()) {
        fields |= ID;
    }
    if (_tsid.set()) {
        fields |= TSID;
    }
    if (_onid.set()) {
        fields |= ONID;
    }
    if (_pmt_pid.set()) {
        fields |= PMT_PID;
    }
    if (_lcn.set()) {
        fields |= LCN;
    }
    if (_type_dvb.set()) {
        fields |= TYPE_DVB;
    }
    if (_type_atsc.set()) {
        fields |= TYPE_ATSC;
    }
    if (_name.set()) {
        fields |= NAME;
    }
    if (_provider.set()) {
        fields |= PROVIDER;
    }
    if (_eits_present.set()) {
        fields |= EITS;
    }
    if (_eitpf_present.set()) {
        fields |= EITPF;
    }
    if (_ca_controlled.set()) {
        fields |= CA;
    }
    if (_running_status.set()) {
        fields |= RUNNING;
    }
    if (_major_id_atsc.set()) {
        fields |= MAJORID_ATSC;
    }
    if (_minor_id_atsc.set()) {
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

    if (_name.set()) {
        str = u"\"" + _name.value() + u"\"";
    }
    if (_major_id_atsc.set() && _minor_id_atsc.set()) {
        if (!str.empty()) {
            str += u", ";
        }
        str += UString::Format(u"%d.%d", {_major_id_atsc.value(), _minor_id_atsc.value()});
    }
    if (_id.set()) {
        if (!str.empty()) {
            str += u", ";
        }
        str += UString::Format(u"0x%X (%d)", {_id.value(), _id.value()});
    }
    if (_lcn.set()) {
        if (!str.empty()) {
            str += u", ";
        }
        str += UString::Format(u"#%d", {_lcn.value()});
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
#define _SORT_(field)                                                               \
    if (s1.field.set() && !s2.field.set()) {                                        \
        return true;                                                                \
    }                                                                               \
    if (!s1.field.set() && s2.field.set()) {                                        \
        return false;                                                               \
    }                                                                               \
    if (s1.field.set() && s2.field.set() && s1.field.value() != s2.field.value()) { \
        return s1.field.value() < s2.field.value();                                 \
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
