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

#include "tstlvMessage.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::tlv::Message::Message(TAG tag) :
    _has_version(false),
    _version(0),
    _tag(tag)
{
}

ts::tlv::Message::Message(VERSION protocol_version, TAG tag) :
    _has_version(true),
    _version(protocol_version),
    _tag(tag)
{
}

ts::tlv::Message::~Message()
{
}


//----------------------------------------------------------------------------
// Serialize a message.
//----------------------------------------------------------------------------

void ts::tlv::Message::serialize(Serializer& zer) const
{
    // Insert the version if the message has one (depends on the protocol)
    if (_has_version) {
        zer.putInt8(_version);
    }

    // Open a nested factory to avoid breaking open TLV
    Serializer pzer(zer);
    pzer.openTLV(_tag);
    serializeParameters(pzer);
    pzer.closeTLV();
}


//----------------------------------------------------------------------------
// Dump routine. Create a string representing the message content.
// The implementation in the base class dumps the common fields.
// Can be used by subclasses.
//----------------------------------------------------------------------------

ts::UString ts::tlv::Message::dump(size_t indent) const
{
    return dumpOptionalHexa(indent, u"protocol_version", _has_version, _version) +
        dumpHexa(indent, u"message_type", _tag);
}


//----------------------------------------------------------------------------
// Helper routine for dump routines in subclasses
//----------------------------------------------------------------------------

ts::UString ts::tlv::Message::dumpOptional(size_t indent, const UString& name, bool has_value, const ByteBlock& bl, uint32_t flags)
{
    if (has_value) {
        return UString::Format(u"%*s%s (%d bytes) = ", {indent, u"", name, bl.size()}) +
               ((flags & UString::SINGLE_LINE) ? u"" : u"\n") +
               UString::Dump(bl.data(), bl.size(), flags, indent + 4) +
               ((flags & UString::SINGLE_LINE) ? u"\n" : u"");
    }
    else {
        return UString();
    }
}


//----------------------------------------------------------------------------
// Helper routine for dump routines in subclasses
//----------------------------------------------------------------------------

ts::UString ts::tlv::Message::dumpVector(size_t indent, const UString& name, const std::vector<UString>& val)
{
    UString result;
    for (const auto& s : val) {
        result += UString::Format(u"%*s%s = \"%s\"\n", {indent, u"", name, s});
    }
    return result;
}
