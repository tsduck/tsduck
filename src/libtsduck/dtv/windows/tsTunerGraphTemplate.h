//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#pragma once
#include "tsNullReport.h"


//-----------------------------------------------------------------------------
// Locate all known interfaces in a pin or node of the tuner filter.
//-----------------------------------------------------------------------------

template <class COMCLASS>
void ts::TunerGraph::findTunerSubinterfaces(ComPtr<COMCLASS>& obj)
{
    findTunerSubinterface(obj, IID_IBDA_DigitalDemodulator,  _demods);
    findTunerSubinterface(obj, IID_IBDA_DigitalDemodulator2, _demods2);
    findTunerSubinterface(obj, IID_IBDA_SignalStatistics,    _sigstats);
    findTunerSubinterface(obj, IID_IKsPropertySet,           _tunprops);
}


//-----------------------------------------------------------------------------
// Locate one interface in a pin or node of the tuner filter.
//-----------------------------------------------------------------------------

template <class COMCLASS, class IFACE>
void ts::TunerGraph::findTunerSubinterface(ComPtr<COMCLASS>& obj, const IID& interface_id, std::vector<ComPtr<IFACE>>& ivector)
{
    ComPtr<IFACE> iobj;
    iobj.queryInterface(obj.pointer(), interface_id, NULLREP);
    if (!iobj.isNull()) {
        ivector.push_back(iobj);
    }
}


//-----------------------------------------------------------------------------
// Repeatedly called when searching for a propery.
//-----------------------------------------------------------------------------

template <typename T>
void ts::TunerGraph::SelectProperty(bool& terminated, bool& found, T& retvalue, T val, PropSearch searchtype)
{
    switch (searchtype) {
        case psFIRST:
            retvalue = val;
            terminated = true;
            break;
        case psLAST:
            retvalue = val;
            break;
        case psHIGHEST:
            if (!found || val > retvalue) {
                retvalue = val;
            }
            break;
        case psLOWEST:
            if (!found || val < retvalue) {
                retvalue = val;
            }
            break;
    }
    found = true;
}


//-----------------------------------------------------------------------------
// Search all IKsPropertySet in the tuner until the specified data is found.
//-----------------------------------------------------------------------------

template <typename VALTYPE>
bool ts::TunerGraph::searchTunerProperty(VALTYPE& retvalue, PropSearch searchtype, const ::GUID& propset, int propid)
{
    bool found = false;
    bool terminated = false;

    // Loop on all property set interfaces in the tuner filter.
    for (size_t i = 0; !terminated && i < _tunprops.size(); ++i) {
        VALTYPE val = VALTYPE(0);
        ::DWORD retsize = sizeof(val);
        if (SUCCEEDED(_tunprops[i]->Get(propset, propid, NULL, 0, &val, retsize, &retsize))) {
            SelectProperty(terminated, found, retvalue, val, searchtype);
        }
    }
    return found;
}


//-----------------------------------------------------------------------------
// Search a property, until found, in "ivector" and then _tunprops.
//-----------------------------------------------------------------------------

template <typename VALTYPE, typename IVALTYPE, class FILTER>
bool ts::TunerGraph::searchPropertyImpl(VALTYPE& retvalue,
                                        PropSearch searchtype,
                                        const std::vector<ComPtr<FILTER>>& ivector,
                                        ::HRESULT (__stdcall FILTER::*getmethod)(IVALTYPE*),
                                        const ::GUID& propset,
                                        int propid)
{
    bool found = false;
    bool terminated = false;

    // First step, lookup all interfaces of a given type.
    for (size_t i = 0; !terminated && i < ivector.size(); ++i) {
        IVALTYPE val;
        FILTER* filter = ivector[i].pointer();
        if (SUCCEEDED((filter->*getmethod)(&val))) {
            SelectProperty<VALTYPE>(terminated, found, retvalue, val, searchtype);
        }
    }

    // Second step, lookup tuner properties.
    for (size_t i = 0; !terminated && i < _tunprops.size(); ++i) {
        VALTYPE val;
        ::DWORD retsize = sizeof(val);
        if (SUCCEEDED(_tunprops[i]->Get(propset, propid, NULL, 0, &val, retsize, &retsize))) {
            SelectProperty<VALTYPE>(terminated, found, retvalue, val, searchtype);
        }
    }

    return found;
}


//-----------------------------------------------------------------------------
// Same one with additional handling of unknown return value.
//-----------------------------------------------------------------------------

template <typename VALTYPE, typename ARGTYPE, typename IVALTYPE, class FILTER>
bool ts::TunerGraph::searchVarPropertyImpl(VALTYPE unset,
                                           Variable<ARGTYPE>& parameter,
                                           PropSearch searchtype,
                                           bool reset_unknown,
                                           const std::vector<ComPtr<FILTER>>& ivector,
                                           ::HRESULT (__stdcall FILTER::*getmethod)(IVALTYPE*),
                                           const ::GUID& propset,
                                           int propid)
{
    VALTYPE retvalue = unset;
    bool found = searchPropertyImpl(retvalue, searchtype, ivector, getmethod, propset, propid);
    if (found && retvalue != unset) {
        parameter = ARGTYPE(retvalue);
    }
    else if (reset_unknown) {
        parameter.clear();
    }
    return found;
}
