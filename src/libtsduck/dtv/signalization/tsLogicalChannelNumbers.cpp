//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsLogicalChannelNumbers.h"
#include "tsEacemLogicalChannelNumberDescriptor.h"
#include "tsEacemHDSimulcastLogicalChannelDescriptor.h"
#include "tsEutelsatChannelNumberDescriptor.h"
#include "tsDTGLogicalChannelDescriptor.h"
#include "tsDTGHDSimulcastLogicalChannelDescriptor.h"
#include "tsNorDigLogicalChannelDescriptorV1.h"
#include "tsNorDigLogicalChannelDescriptorV2.h"
#include "tsSkyLogicalChannelNumberDescriptor.h"


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::LogicalChannelNumbers::LogicalChannelNumbers(DuckContext& duck) :
    _duck(duck),
    _lcn_map()
{
}

ts::LogicalChannelNumbers::LCN::LCN(uint16_t l, uint16_t t, uint16_t o) :
    lcn(l),
    ts_id(t),
    onet_id(o)
{
}


//----------------------------------------------------------------------------
// Add the logical channel number of a service.
//----------------------------------------------------------------------------

void ts::LogicalChannelNumbers::addLCN(uint16_t lcn, uint16_t srv_id, uint16_t ts_id, uint16_t onet_id)
{
    // Loop for similar entry to update.
    for (auto it = _lcn_map.lower_bound(srv_id); it != _lcn_map.end() && it->first == srv_id; ++it) {
        if (it->second.ts_id == ts_id && it->second.onet_id == onet_id) {
            // Update existing entry.
            it->second.lcn = lcn;
            return;
        }
    }

    // No existing entry found, add a new one.
    _lcn_map.insert(std::make_pair(srv_id, LCN(lcn, ts_id, onet_id)));
}


//----------------------------------------------------------------------------
// Collect all LCN which are declared in a list of descriptors.
//----------------------------------------------------------------------------

size_t ts::LogicalChannelNumbers::addFromDescriptors(const DescriptorList& descs, uint16_t ts_id, uint16_t onet_id)
{
    size_t count = 0;
    for (size_t index = 0; index < descs.size(); ++index) {
        const DescriptorPtr& ptr(descs[index]);
        if (!ptr.isNull() && ptr->isValid()) {

            // Most LCN descriptors are private descriptors. Get tag and PDS.
            const DID tag = ptr->tag();
            const PDS pds = _duck.actualPDS(descs.privateDataSpecifier(index));

            // Check all known forms of LCN descriptors.
            if (pds == PDS_EACEM && tag == DID_LOGICAL_CHANNEL_NUM) {
                const EacemLogicalChannelNumberDescriptor desc(_duck, *ptr);
                count += addFromAbstractLCN(desc, ts_id, onet_id);
            }
            else if (pds == PDS_EACEM && tag == DID_HD_SIMULCAST_LCN) {
                const EacemHDSimulcastLogicalChannelDescriptor desc(_duck, *ptr);
                count += addFromAbstractLCN(desc, ts_id, onet_id);
            }
            else if (pds == PDS_OFCOM && tag == DID_OFCOM_LOGICAL_CHAN) {
                const DTGLogicalChannelDescriptor desc(_duck, *ptr);
                count += addFromAbstractLCN(desc, ts_id, onet_id);
            }
            else if (pds == PDS_OFCOM && tag == DID_OFCOM_HD_SIMULCAST) {
                DTGHDSimulcastLogicalChannelDescriptor desc(_duck, *ptr);
                count += addFromAbstractLCN(desc, ts_id, onet_id);
            }
            else if (pds == PDS_BSKYB && tag == DID_LOGICAL_CHANNEL_SKY) {
                SkyLogicalChannelNumberDescriptor desc(_duck, *ptr);
                if (desc.isValid()) {
                    for (const auto& it : desc.entries) {
                        addLCN(it.lcn, it.service_id, ts_id, onet_id);
                        count++;
                    }
                }
            }
            else if (pds == PDS_EUTELSAT && tag == DID_EUTELSAT_CHAN_NUM) {
                EutelsatChannelNumberDescriptor desc(_duck, *ptr);
                if (desc.isValid()) {
                    for (const auto& it : desc.entries) {
                        addLCN(it.ecn, it.service_id, it.ts_id, it.onetw_id);
                        count++;
                    }
                }
            }
            else if (pds == PDS_NORDIG && tag == DID_NORDIG_CHAN_NUM_V1) {
                NorDigLogicalChannelDescriptorV1 desc(_duck, *ptr);
                if (desc.isValid()) {
                    for (const auto& it : desc.entries) {
                        addLCN(it.lcn, it.service_id, ts_id, onet_id);
                        count++;
                    }
                }
            }
            else if (pds == PDS_NORDIG && tag == DID_NORDIG_CHAN_NUM_V2) {
                NorDigLogicalChannelDescriptorV2 desc(_duck, *ptr);
                if (desc.isValid()) {
                    for (const auto& it1 : desc.entries) {
                        for (const auto& it2 : it1.services) {
                            addLCN(it2.lcn, it2.service_id, ts_id, onet_id);
                            count++;
                        }
                    }
                }
            }
        }
    }
    return count;
}


//----------------------------------------------------------------------------
// Collect LCN for a generic form of LCN descriptor.
//----------------------------------------------------------------------------

size_t ts::LogicalChannelNumbers::addFromAbstractLCN(const AbstractLogicalChannelDescriptor& desc, uint16_t ts_id, uint16_t onet_id)
{
    size_t count = 0;
    if (desc.isValid()) {
        for (const auto& it : desc.entries) {
            addLCN(it.lcn, it.service_id, ts_id, onet_id);
            count++;
        }
    }
    return count;
}


//----------------------------------------------------------------------------
// Collect all LCN which are declared in a NIT.
//----------------------------------------------------------------------------

size_t ts::LogicalChannelNumbers::addFromNIT(const NIT& nit, uint16_t ts_id, uint16_t onet_id)
{
    size_t count = 0;
    if (nit.isValid()) {
        for (const auto& it : nit.transports) {
            if ((ts_id == 0xFFFF || it.first.transport_stream_id == 0xFFFF || ts_id == it.first.transport_stream_id) &&
                (onet_id == 0xFFFF || it.first.original_network_id == 0xFFFF || onet_id == it.first.original_network_id))
            {
                count += addFromDescriptors(it.second.descs, it.first.transport_stream_id, it.first.original_network_id);
            }
        }
    }
    return count;
}


//----------------------------------------------------------------------------
// Get the logical channel number of a service.
//----------------------------------------------------------------------------

uint16_t ts::LogicalChannelNumbers::getLCN(const ServiceIdTriplet& srv) const
{
    return getLCN(srv.service_id, srv.transport_stream_id, srv.original_network_id);
}

uint16_t ts::LogicalChannelNumbers::getLCN(uint16_t srv_id, uint16_t ts_id, uint16_t onet_id) const
{
    uint16_t lcn = 0xFFFF;
    for (auto it = _lcn_map.lower_bound(srv_id); it != _lcn_map.end() && it->first == srv_id; ++it) {
        if (it->second.ts_id == ts_id) {
            if (it->second.onet_id == onet_id) {
                // Found an exact match, including if both are 0xFFFF, final value.
                lcn = it->second.lcn;
                break;
            }
            else if (it->second.onet_id == 0xFFF) {
                // Possible match, keep it but continue to search an exact match.
                lcn = it->second.lcn;
            }
        }
    }
    return lcn;
}


//----------------------------------------------------------------------------
// Get all known services by logical channel number.
//----------------------------------------------------------------------------

void ts::LogicalChannelNumbers::getLCNs(std::map<uint16_t,ServiceIdTriplet>& lcns, uint16_t ts_id, uint16_t onet_id) const
{
    lcns.clear();
    for (auto it : _lcn_map) {
        if ((ts_id == 0xFFFF || it.second.ts_id == 0xFFFF || ts_id == it.second.ts_id) &&
            (onet_id == 0xFFFF || it.second.onet_id == 0xFFFF || onet_id == it.second.onet_id))
        {
            lcns.insert(std::make_pair(it.second.lcn, ServiceIdTriplet(it.first, it.second.ts_id, it.second.onet_id)));
        }
    }
}


//----------------------------------------------------------------------------
// Update a service description with its LCN.
//----------------------------------------------------------------------------

bool ts::LogicalChannelNumbers::updateService(Service& srv, bool replace) const
{
    if (srv.hasId() && srv.hasTSId() && (replace || !srv.hasLCN())) {
        const uint16_t onid = srv.hasONId() ? srv.getONId() : 0xFFFF;
        const uint16_t lcn = getLCN(srv.getId(), srv.getTSId(), onid);
        if (lcn != 0xFFFF) {
            srv.setLCN(lcn);
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Update a list of service descriptions with LCN's.
//----------------------------------------------------------------------------

size_t ts::LogicalChannelNumbers::updateServices(ServiceList& srv_list, bool replace, bool add) const
{
    // Coudn of updated/added services.
    size_t count = 0;

    // Build a copy of the internal LCN map, remove them one by one when used.
    LCNMap lcns(_lcn_map);

    // Update LCN's in existing services.
    for (auto lcn_it = lcns.begin(); lcn_it != lcns.end(); ) {
        bool found = false;

        // Loop on all services and update matching ones.
        for (auto& srv_it : srv_list) {
            // Check if this service match the current LCN (onet id must match or be unspecified).
            if (srv_it.hasId(lcn_it->first) &&
                srv_it.hasTSId(lcn_it->second.ts_id) &&
                (lcn_it->second.onet_id == 0xFFFF || !srv_it.hasONId() || srv_it.hasONId(lcn_it->second.onet_id)))
            {
                found = true;
                if (!srv_it.hasLCN(lcn_it->second.lcn)) {
                    srv_it.setLCN(lcn_it->second.lcn);
                    ++count;
                }
            }
        }

        // Move to next LCN. Remove current from the list if found in the list of services.
        if (found) {
            lcn_it = lcns.erase(lcn_it);
        }
        else {
            ++lcn_it;
        }
    }

    // Add remaining LCN's in the list of services.
    if (add) {
        for (const auto& lcn_it : lcns) {
            auto srv = srv_list.emplace(srv_list.end());
            srv->setId(lcn_it.first);
            srv->setLCN(lcn_it.second.lcn);
            srv->setTSId(lcn_it.second.ts_id);
            if (lcn_it.second.onet_id != 0xFFFF) {
                srv->setONId(lcn_it.second.onet_id);
            }
            ++count;
        }
    }

    return count;
}
