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

#include "tsArgMix.h"
#include "tsUString.h"

const std::string ts::ArgMix::empty;
const ts::UString ts::ArgMix::uempty;


//----------------------------------------------------------------------------
// ArgMix constructors.
//----------------------------------------------------------------------------

ts::ArgMix::ArgMix() :
    _type(0),
    _size(0),
    _value(int32_t(0)),
    _aux(nullptr)
{
}

ts::ArgMix::ArgMix(const ts::ArgMix& other) :
    _type(other._type),
    _size(other._size),
    _value(other._value),
    _aux(other._aux == nullptr ? nullptr : new UString(*other._aux))
{
}

ts::ArgMix::ArgMix(ts::ArgMix&& other) :
    _type(other._type),
    _size(other._size),
    _value(other._value),
    _aux(other._aux)
{
    other._aux = nullptr;
}

ts::ArgMix::ArgMix(TypeFlags type, size_t size, const Value value) :
    _type(type),
    _size(uint8_t(size)),
    _value(value),
    _aux(nullptr)
{
}


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::ArgMix::~ArgMix()
{
    // Deallocate auxiliary string, when there is one.
    if (_aux != nullptr) {
        delete _aux;
        _aux = nullptr;
    }
}


//----------------------------------------------------------------------------
// Return ArgMix value as a string.
//----------------------------------------------------------------------------

const char* ts::ArgMix::toCharPtr() const
{
    switch (_type) {
        case STRING | BIT8: {
            // A pointer to char.
            return _value.charptr == nullptr ? "" : _value.charptr;
        }
        case STRING | BIT8 | CLASS: {
            // A pointer to std::string.
            return _value.string == nullptr ? "" : _value.string->c_str();
        }
        default: {
            return "";
        }
    }
}

const ts::UChar* ts::ArgMix::toUCharPtr() const
{
    switch (_type) {
        case STRING | BIT16: {
            // A pointer to UChar.
            return _value.ucharptr == nullptr ? u"" : _value.ucharptr;
        }
        case STRING | BIT16 | CLASS: {
            // A pointer to UString.
            return _value.ustring == nullptr ? u"" : _value.ustring->c_str();
        }
        case STRING | BIT8: {
            // A pointer to char. Need to allocate an auxiliary string.
            if (_value.charptr != nullptr && _aux == nullptr) {
                _aux = new UString;
                _aux->assignFromUTF8(_value.charptr);
            }
            return _aux == nullptr ? u"" : _aux->c_str();
        }
        case STRING | BIT8 | CLASS: {
            // A pointer to std::string. Need to allocate an auxiliary string.
            if (_value.string != nullptr && _aux == nullptr) {
                _aux = new UString;
                _aux->assignFromUTF8(*_value.string);
            }
            return _aux == nullptr ? u"" : _aux->c_str();
        }
        case STRING | BIT16 | CLASS | STRINGIFY: {
            // A pointer to StringifyInterface. Need to allocate an auxiliary string.
            if (_value.stringify != nullptr && _aux == nullptr) {
                _aux = new UString(_value.stringify->toString());
            }
            return _aux == nullptr ? u"" : _aux->c_str();
        }
        case ANUMBER: {
            // A pointer to AbstractNumer. Need to allocate an auxiliary string.
            if (_value.anumber != nullptr && _aux == nullptr) {
                _aux = new UString(_value.anumber->toString());
            }
            return _aux == nullptr ? u"" : _aux->c_str();
        }
        default: {
            return u"";
        }
    }
}

const std::string& ts::ArgMix::toString() const
{
    if (_type == (STRING | BIT8 | CLASS)) {
        return _value.string != nullptr ? *_value.string : empty;
    }
    else {
        return empty;
    }
}

const ts::UString& ts::ArgMix::toUString() const
{
    switch (_type) {
        case STRING | BIT8: {
            // A pointer to char. Need to allocate an auxiliary string.
            if (_value.charptr != nullptr && _aux == nullptr) {
                _aux = new UString;
                _aux->assignFromUTF8(_value.charptr);
            }
            return _aux == nullptr ? uempty : *_aux;
        }
        case STRING | BIT8 | CLASS: {
            // A pointer to std::string. Need to allocate an auxiliary string.
            if (_value.string != nullptr && _aux == nullptr) {
                _aux = new UString;
                _aux->assignFromUTF8(*_value.string);
            }
            return _aux == nullptr ? uempty : *_aux;
        }
        case STRING | BIT16: {
            // A pointer to UChar. Need to allocate an auxiliary string.
            if (_value.charptr != nullptr && _aux == nullptr) {
                _aux = new UString(_value.ucharptr);
            }
            return _aux == nullptr ? uempty : *_aux;
        }
        case STRING | BIT16 | CLASS: {
            // A pointer to UString.
            return _value.ustring == nullptr ? uempty : *_value.ustring;
        }
        case STRING | BIT16 | CLASS | STRINGIFY: {
            // A pointer to StringifyInterface. Need to allocate an auxiliary string.
            if (_value.stringify != nullptr && _aux == nullptr) {
                _aux = new UString(_value.stringify->toString());
            }
            return _aux == nullptr ? uempty : *_aux;
        }
        case ANUMBER: {
            // A pointer to AbstractNumer. Need to allocate an auxiliary string.
            if (_value.anumber != nullptr && _aux == nullptr) {
                _aux = new UString(_value.anumber->toString());
            }
            return _aux == nullptr ? uempty : *_aux;
        }
        default: {
            return uempty;
        }
    }
}


//----------------------------------------------------------------------------
// Return ArgMix value as a double.
//----------------------------------------------------------------------------

double ts::ArgMix::toDouble() const
{
    if ((_type & DOUBLE) == DOUBLE) {
        return _value.dbl;
    }
    else if ((_type & ANUMBER) == ANUMBER) {
        return _value.anumber->toDouble();
    }
    else if (isSigned()) {
        return double(toInt64());
    }
    else if (isUnsigned()) {
        return double(toUInt64());
    }
    else {
        return 0.0;
    }
}


//----------------------------------------------------------------------------
// Return ArgMix value as an AbstractNumber.
//----------------------------------------------------------------------------

const ts::AbstractNumber& ts::ArgMix::toAbstractNumber() const
{
    switch (_type) {
        case ANUMBER: {
            return _value.anumber != nullptr ? *_value.anumber : *AbstractNumber::INVALID;
        }
        default: {
            return *AbstractNumber::INVALID;
        }
    }
}
