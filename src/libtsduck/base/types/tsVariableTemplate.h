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

#pragma once
#include "tsException.h"


//----------------------------------------------------------------------------
// Constructors, assignment and destructors.
//----------------------------------------------------------------------------

template <typename T>
ts::Variable<T>::Variable(const Variable<T>& other) :
    _access(nullptr)
{
    if (other._access != nullptr) {
        _access = new(_data) T(*(other._access));
    }
}

template <typename T>
ts::Variable<T>::Variable(Variable<T>&& other) :
    _access(nullptr)
{
    if (other._access != nullptr) {
        _access = new(_data) T(std::move(*(other._access)));
        other.clear();
    }
}

TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <typename T>
ts::Variable<T>::~Variable()
{
    clear();
}
TS_POP_WARNING()

template <typename T>
ts::Variable<T>& ts::Variable<T>::operator=(const Variable<T>& other)
{
    if (&other != this) {
        clear();
        if (other._access != nullptr) {
            _access = new(_data) T(*(other._access));
        }
    }
    return *this;
}

template <typename T>
ts::Variable<T>& ts::Variable<T>::operator=(Variable<T>&& other)
{
    if (&other != this) {
        clear();
        if (other._access != nullptr) {
            _access = new(_data) T(std::move(*(other._access)));
            other.clear();
        }
    }
    return *this;
}

template <typename T>
ts::Variable<T>& ts::Variable<T>::operator=(const T& obj)
{
    clear();
    _access = new(_data) T(obj);
    return *this;
}

template <typename T>
bool ts::Variable<T>::setDefault(const T& def)
{
    if (_access != nullptr) {
        // Variable is already set.
        return false;
    }
    else {
        _access = new(_data) T(def);
        return true;
    }
}


//----------------------------------------------------------------------------
// Clear the value.
//----------------------------------------------------------------------------

template <typename T>
void ts::Variable<T>::clear()
{
    if (_access != nullptr) {
        // Safe when the destructor throws an exception
        T* tmp = _access;
        _access = nullptr;
        tmp->~T();
    }
}


//----------------------------------------------------------------------------
// Access the value inside the variable.
//----------------------------------------------------------------------------

template <typename T>
const T& ts::Variable<T>::value() const
{
    if (_access != nullptr) {
        return *_access;
    }
    else {
        throw UninitializedVariable(u"uninitialized variable");
    }
}

template <typename T>
T& ts::Variable<T>::value()
{
    if (_access != nullptr) {
        return *_access;
    }
    else {
        throw UninitializedVariable(u"uninitialized variable");
    }
}

template <typename T>
T ts::Variable<T>::value(const T& def) const
{
    return _access != nullptr ? *_access : def;
}


//----------------------------------------------------------------------------
// Comparison operators.
//----------------------------------------------------------------------------

template <typename T>
bool ts::Variable<T>::identical(const Variable<T>& other) const
{
    return (_access == nullptr && other._access == nullptr ) ||
           (_access != nullptr && other._access != nullptr && *_access == *other._access);
}


template <typename T>
bool ts::Variable<T>::operator==(const Variable<T>& other) const
{
    return _access != nullptr && other._access != nullptr && *_access == *other._access;
}

#if defined(TS_NEED_UNEQUAL_OPERATOR)
template <typename T>
bool ts::Variable<T>::operator!=(const Variable<T>& other) const
{
    // Note than we do not require that T provides operator!=.
    // We just require operator==. So we use !(.. == ..).
    return _access == nullptr || other._access == nullptr || !(*_access == *other._access);
}
#endif

template <typename T>
bool ts::Variable<T>::operator==(const T& obj) const
{
    return _access != nullptr && *_access == obj;
}

#if defined(TS_NEED_UNEQUAL_OPERATOR)
template <typename T>
bool ts::Variable<T>::operator!=(const T& obj) const
{
    // Note than we do not require that T provides operator!=.
    // We just require operator==. So we use !(.. == ..).
    return _access == nullptr || !(*_access == obj);
}
#endif
