//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDescriptorContext.h"
#include "tsDescriptorList.h"
#include "tsAbstractTable.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DescriptorContext::DescriptorContext(const DuckContext& duck, TID tid, Standards standards, CASID casid, const REGIDVector& regids, PDS pds) :
    _duck(duck),
    _tid(tid),
    _casid(casid),
    _standards(standards),
    _default_regids(regids),
    _default_pds(pds)
{
}

ts::DescriptorContext::DescriptorContext(const DuckContext& duck, const DescriptorList& dlist, size_t index, CASID casid) :
    _duck(duck),
    _casid(casid),
    _dlist(&dlist),
    _dlist_index(index),
    _use_defaults(false)
{
}

//----------------------------------------------------------------------------
// Set descriptor lists.
//----------------------------------------------------------------------------

void ts::DescriptorContext::setCurrentDescriptorList(const DescriptorList* dlist, size_t index)
{
    _dlist = dlist;
    _dlist_index = index;
    _top_dlist = _low_dlist = nullptr;
    _top_size = _low_size = 0;
    _top_regids.clear();
    _low_regids.clear();
    _top_regids_valid = _low_regids_valid = false;
    _low_pds = PDS_NULL;
    _low_pds_valid = false;
    _use_defaults = _dlist == nullptr;
}

void ts::DescriptorContext::setCurrentRawDescriptorList(const void* data, size_t size)
{
    _low_dlist = reinterpret_cast<const uint8_t*>(data);
    _low_size = size;
    _low_regids.clear();
    _low_regids_valid = false;
    _low_pds = PDS_NULL;
    _low_pds_valid = false;
    _dlist = nullptr;
    _dlist_index = 0;
    _use_defaults = _low_dlist == nullptr && _top_dlist == nullptr;
}

void ts::DescriptorContext::setTopLevelRawDescriptorList(const void* data, size_t size)
{
    _top_dlist = reinterpret_cast<const uint8_t*>(data);
    _top_size = size;
    _top_regids.clear();
    _top_regids_valid = false;
    _dlist = nullptr;
    _dlist_index = 0;
    _use_defaults = _low_dlist == nullptr && _top_dlist == nullptr;
}

void ts::DescriptorContext::moveRawDescriptorListToTop()
{
    _top_dlist = _low_dlist;
    _top_size = _low_size;
    _top_regids = std::move(_low_regids);
    _top_regids_valid = _low_regids_valid;
    _low_dlist = nullptr;
    _low_size = 0;
    _low_regids.clear();
    _low_regids_valid = false;
    _low_pds = PDS_NULL;
    _low_pds_valid = false;
    _dlist = nullptr;
    _dlist_index = 0;
    _use_defaults = _low_dlist == nullptr && _top_dlist == nullptr;
}


//----------------------------------------------------------------------------
// Get simple properties.
//----------------------------------------------------------------------------

ts::TID ts::DescriptorContext::getTableId() const
{
    return _dlist != nullptr ? _dlist->tableId() : _tid;
}

ts::Standards ts::DescriptorContext::getStandards() const
{
    return _duck.standards() | _standards;
}

ts::CASID ts::DescriptorContext::getCAS() const
{
    return _duck.casId(_casid);
}


//----------------------------------------------------------------------------
// Get the list of DVB private data specifier.
//----------------------------------------------------------------------------

ts::PDS ts::DescriptorContext::getPDS()
{
    // If no descriptor list is set, use default from constructor.
    if (_use_defaults) {
        return _duck.actualPDS(_default_pds);
    }

    // If the PDS was not yet searched in the current descriptor list, do it now.
    if (!_low_pds_valid) {
        _low_pds = PDS_NULL;
        if (_dlist != nullptr) {
            // Look backward from the end of descriptor list until first PDS descriptor.
            size_t index = std::min(_dlist_index + 1, _dlist->size());
            while (index-- > 0) {
                const auto& desc((*_dlist)[index]);
                if (desc != nullptr && desc->isValid() && desc->tag() == DID_DVB_PRIV_DATA_SPECIF && desc->payloadSize() >= 4) {
                    _low_pds = GetUInt32(desc->payload());
                    break;
                }
            }
            _low_pds_valid = true;
        }
        else if (_low_dlist != nullptr) {
            // Unstructured descriptor list. We cannot loop backward from descriptor to descriptor.
            // Loop forward the entire list and keep the last PDS.
            const uint8_t* data = _low_dlist;
            size_t size = _low_size;
            while (size >= 6) { // 6 = min PDS descriptor size
                if (data[0] == DID_DVB_PRIV_DATA_SPECIF && data[1] >= 4) {
                    _low_pds = GetUInt32(data + 2);
                }
                const size_t dsize = std::min<size_t>(2 + data[1], size);
                data += dsize;
                size -= dsize;
            }
            _low_pds_valid = true;
        }
    }
    return _duck.actualPDS(_low_pds);
}


//----------------------------------------------------------------------------
// Get the list of registration ids.
//----------------------------------------------------------------------------

ts::REGIDVector ts::DescriptorContext::getREGIDs()
{
    REGIDVector regids;
    getREGIDs(regids);
    return regids;
}

void ts::DescriptorContext::getREGIDs(REGIDVector& regids)
{
    // Always insert default registration ids at the beginning.
    regids.clear();
    _duck.updateREGIDs(regids);

    // If no descriptor list is set, use default from constructor.
    if (_use_defaults) {
        regids.insert(regids.end(), _default_regids.begin(), _default_regids.end());
        return;
    }

    // If REGID's from the top-level list are not search yet, do if now.
    if (!_top_regids_valid) {
        _top_regids.clear();
        if (_dlist != nullptr) {
            if (_dlist->table() != nullptr) {
                const DescriptorList* top = _dlist->table()->topLevelDescriptorList();
                if (top != nullptr && top != _dlist) {
                    updateREGIDs(_top_regids, *top, NPOS, false);
                }
            }
            _top_regids_valid = true;
        }
        else if (_top_dlist != nullptr && _top_dlist != _low_dlist) {
            updateREGIDs(_top_regids, _top_dlist, _top_size, false);
            _top_regids_valid = true;
        }
    }

    // Add all registration ids from top-level list.
    regids.insert(regids.end(), _top_regids.begin(), _top_regids.end());

    // If REGID's from the lower-level list are not search yet, do if now.
    if (!_low_regids_valid) {
        _low_regids.clear();
        if (_dlist != nullptr) {
            updateREGIDs(_low_regids, *_dlist, _dlist_index, true);
            _low_regids_valid = true;
        }
        else if (_low_dlist != nullptr) {
            updateREGIDs(_low_regids, _low_dlist, _low_size, true);
            _low_regids_valid = true;
        }
    }

    // Add all registration ids from lower-level list.
    regids.insert(regids.end(), _low_regids.begin(), _low_regids.end());
}


//----------------------------------------------------------------------------
// Update registration ids from a structured descriptor list.
//----------------------------------------------------------------------------

void ts::DescriptorContext::updateREGIDs(REGIDVector& regids, const DescriptorList& dlist, size_t max_index, bool is_low)
{
    for (size_t index = 0; index < dlist.size() && index <= max_index; ++index) {
        const auto& desc(dlist[index]);
        if (desc != nullptr && desc->isValid()) {
            if (desc->tag() == DID_MPEG_REGISTRATION && desc->payloadSize() >= 4) {
                // Add a registration id.
                regids.push_back(GetUInt32(desc->payload()));
            }
            else if (is_low && desc->tag() == DID_DVB_PRIV_DATA_SPECIF && desc->payloadSize() >= 4) {
                // Opportunistic collection of PDS...
                _low_pds = GetUInt32(desc->payload());
                _low_pds_valid = true;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Update registration ids from an unstructured descriptor list.
//----------------------------------------------------------------------------

void ts::DescriptorContext::updateREGIDs(REGIDVector& regids, const uint8_t* data, size_t size, bool is_low)
{
    while (size >= 6) { // 6 = min REGID or PDS descriptor size
        if (data[0] == DID_MPEG_REGISTRATION && data[1] >= 4) {
            // Add a registration id.
            regids.push_back(GetUInt32(data + 2));
        }
        else if (is_low && data[0] == DID_DVB_PRIV_DATA_SPECIF && data[1] >= 4) {
            // Opportunistic collection of PDS...
            _low_pds = GetUInt32(data + 2);
            _low_pds_valid = true;
        }
        const size_t dsize = std::min<size_t>(2 + data[1], size);
        data += dsize;
        size -= dsize;
    }
}
