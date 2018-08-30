//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsArgMix.h"
#include "tsUString.h"
TSDUCK_SOURCE;

const std::string ts::ArgMix::empty;
const ts::UString ts::ArgMix::uempty;

//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::ArgMix::~ArgMix()
{
    // Deallocate auxiliary string, when there is one.
    if (_aux != 0) {
        delete _aux;
        _aux = 0;
    }
}


//----------------------------------------------------------------------------
// Return ArgMix value as a string.
//----------------------------------------------------------------------------

const char* ts::ArgMix::toCharPtr() const
{
    switch (_type) {
        case STRING | BIT8 | CLASS:
            return _value.string == 0 ? "" : _value.string->c_str();
        case STRING | BIT8:
            return _value.charptr == 0 ? "" : _value.charptr;
        default:
            return "";
    }
}

const ts::UChar* ts::ArgMix::toUCharPtr() const
{
    switch (_type) {
        case STRING | BIT16: {
            // A pointer to UChar.
            return _value.ucharptr == 0 ? u"" : _value.ucharptr;
        }
        case STRING | BIT16 | CLASS: {
            // A pointer to UString.
            return _value.ustring == 0 ? u"" : _value.ustring->c_str();
        }
        case STRING | BIT16 | CLASS | STRINGIFY: {
            // A pointer to StringifyInterface. Need to allocate an auxiliary string.
            if (_value.stringify != 0 && _aux == 0) {
                _aux = new UString(_value.stringify->toString());
            }
            return _aux == 0 ? u"" : _aux->c_str();
        }
        default: {
            return u"";
        }
    }
}

const std::string& ts::ArgMix::toString() const
{
    return _type == (STRING | BIT8 | CLASS) && _value.string != 0 ? *_value.string : empty;
}

const ts::UString& ts::ArgMix::toUString() const
{
    switch (_type) {
        case STRING | BIT16 | CLASS: {
            // A pointer to UString.
            return _value.ustring == 0 ? uempty : *_value.ustring;
        }
        case STRING | BIT16 | CLASS | STRINGIFY: {
            // A pointer to StringifyInterface. Need to allocate an auxiliary string.
            if (_value.stringify != 0 && _aux == 0) {
                _aux = new UString(_value.stringify->toString());
            }
            return _aux == 0 ? uempty : *_aux;
        }
        default: {
            return uempty;
        }
    }
}
