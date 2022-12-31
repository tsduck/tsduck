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
//
//  Factory class for TLV messages
//
//----------------------------------------------------------------------------

#pragma once
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Internal method: Check the size of a parameter.
// Should never throw an exception, except bug in the
// DeserializeParameters method of the Message subclasses.
//----------------------------------------------------------------------------

template <typename T>
void ts::tlv::MessageFactory::checkParamSize(TAG tag, const ParameterMultimap::const_iterator& it) const
{
    const size_t expected = dataSize<T>();
    if (it->second.length != expected) {
        throw DeserializationInternalError(UString::Format(u"Bad size for parameter 0x%X in message, expected %d bytes, found %d", {tag, expected, it->second.length}));
    }
}


//----------------------------------------------------------------------------
// Get first occurence of an integer parameter:
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::tlv::MessageFactory::get(TAG tag) const
{
    const auto it = _params.find(tag);
    if (it == _params.end()) {
        throw DeserializationInternalError(UString::Format(u"No parameter 0x%X in message", {tag}));
    }
    else {
        checkParamSize<INT>(tag, it);
        return GetInt<INT>(it->second.addr);
    }
}


//----------------------------------------------------------------------------
// Get all occurences of an integer parameter.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::tlv::MessageFactory::get(TAG tag, std::vector<INT>& param) const
{
    // Reinitialize result vector
    param.clear();
    param.reserve(_params.count(tag));
    // Fill vector with parameter values
    const auto last = _params.upper_bound (tag);
    for (auto it = _params.lower_bound(tag); it != last; ++it) {
        checkParamSize<INT>(tag, it);
        param.push_back(GetInt<INT>(it->second.addr));
    }
}


//----------------------------------------------------------------------------
// Get a compound TLV parameter using a derived class of Message.
//----------------------------------------------------------------------------

template <class MSG>
void ts::tlv::MessageFactory::getCompound(TAG tag, MSG& param) const
{
    MessagePtr gen;
    getCompound (tag, gen);
    MSG* msg = dynamic_cast<MSG*> (gen.pointer());
    if (msg == 0) {
        throw DeserializationInternalError(UString::Format(u"Wrong compound TLV type for parameter 0x%X", {tag}));
    }
    param = *msg;
}


//----------------------------------------------------------------------------
// Get all occurences of a compound TLV parameter using a derived class of
// Message.
//----------------------------------------------------------------------------

template <class MSG>
void ts::tlv::MessageFactory::getCompound(TAG tag, std::vector<MSG>& param) const
{
    // Reinitialize result vector
    param.clear();
    // Fill vector with parameter values
    auto it = _params.lower_bound(tag);
    const auto last = _params.upper_bound(tag);
    for (int i = 0; it != last; ++it, ++i) {
        if (it->second.compound.isNull()) {
            throw DeserializationInternalError(UString::Format(u"Occurence %d of parameter 0x%X not a compound TLV", {i, tag}));
        }
        else {
            MessagePtr gen;
            it->second.compound->factory(gen);
            MSG* msg = dynamic_cast<MSG*> (gen.pointer());
            if (msg == 0) {
                throw DeserializationInternalError(UString::Format(u"Wrong compound TLV type for occurence %d of parameter 0x%X", {i, tag}));
            }
            param.push_back(*msg);
        }
    }
}
