//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Representation of a terrestrial_delivery_system_descriptor
//
//----------------------------------------------------------------------------

#include "tsTerrestrialDeliverySystemDescriptor.h"



//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor() :
    AbstractDescriptor (DID_TERREST_DELIVERY),
    centre_frequency (0),
    bandwidth (0),
    high_priority (true),
    no_time_slicing (true),
    no_mpe_fec (true),
    constellation (0),
    hierarchy (0),
    code_rate_hp (0),
    code_rate_lp (0),
    guard_interval (0),
    transmission_mode (0),
    other_frequency (false)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor (const Descriptor& desc) :
    AbstractDescriptor (DID_TERREST_DELIVERY)
{
    deserialize (desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::serialize (Descriptor& desc) const
{
    uint8_t data[13];
    data[0] = _tag;
    data[1] = 11;
    PutUInt32 (data + 2, centre_frequency);
    data[6] = (bandwidth << 5) |
              (uint8_t (high_priority) << 4) |
              (uint8_t (no_time_slicing) << 3) |
              (uint8_t (no_mpe_fec) << 2) |
              0x03;
    data[7] = (constellation << 6) |
              ((hierarchy & 0x07) << 3) |
              (code_rate_hp & 0x07);
    data[8] = (code_rate_lp << 5) |
              ((guard_interval & 0x03) << 3) |
              ((transmission_mode & 0x03) << 1) |
              (uint8_t (other_frequency));
    data[9] = data[10] = data[11] = data[12] = 0xFF;

    Descriptor d (data, sizeof(data));
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::deserialize (const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 7;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        centre_frequency = GetUInt32 (data);
        bandwidth = (data[4] >> 5) & 0x07;
        high_priority = (data[4] & 0x10) != 0;
        no_time_slicing = (data[4] & 0x08) != 0;
        no_mpe_fec = (data[4] & 0x04) != 0;
        constellation = (data[5] >> 6) & 0x03;
        hierarchy = (data[5] >> 3) & 0x07;
        code_rate_hp = data[5] & 0x07;
        code_rate_lp = (data[6] >> 5) & 0x07;
        guard_interval = (data[6] >> 3) & 0x03;
        transmission_mode = (data[6] >> 1) & 0x03;
        other_frequency = (data[6] & 0x01) != 0;
    }
}
