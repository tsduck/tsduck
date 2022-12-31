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
//!
//!  @file
//!  Declare the ts::Variable template class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! A template class which defines a @e variable which can be either initialized or uninitialized.
    //! @ingroup cpp
    //!
    //! @tparam T A type or class which describes the content of the variable.
    //! The basic requirement on @a T is the availability of a copy constructor and
    //! operators for assignment and equality.
    //!
    template <typename T>
    class Variable
    {
    private:
        T*      _access;           // point to _data when initialized
        uint8_t _data[sizeof(T)];  // flat memory area for T instance

    public:
        //!
        //! Default constructor, the variable is uninitialized.
        //!
        Variable() noexcept : _access(nullptr) {}

        //!
        //! Copy constructor.
        //!
        //! This object is in the same state as @a other. If @a other is
        //! initialized, this object becomes initialized with the same @a T value.
        //!
        //! @param [in] other Another instance from which to build this object.
        //!
        Variable(const Variable<T>& other);

        //!
        //! Move constructor.
        //!
        //! @param [in,out] other Another instance from which to build this object.
        //!
        Variable(Variable<T>&& other);

        //!
        //! Constructor from a @a T instance, the variable is initialized.
        //!
        //! @param [in] obj The initial value for the variable.
        //!
        Variable(const T& obj) : _access(new(_data) T(obj)) {}

        //!
        //! Virtual destructor
        //!
        virtual ~Variable();

        //!
        //! Assignment operator.
        //!
        //! This object is in the same state as @a other. If @a other is
        //! initialized, this object becomes initialized with the same @a T value.
        //!
        //! @param [in] other Another instance from which to assign this object.
        //! @return A reference to this object.
        //!
        Variable<T>& operator=(const Variable<T>& other);

        //!
        //! Move assignment operator.
        //!
        //! @param [in,out] other Another instance from which to assign this object.
        //! @return A reference to this object.
        //!
        Variable<T>& operator=(Variable<T>&& other);

        //!
        //! Assignment operator from a @a T object.
        //!
        //! This object becomes initialized if it was not already.
        //!
        //! @param [in] obj Value from which to assign this object.
        //! @return A reference to this object.
        //!
        Variable<T>& operator=(const T& obj);

        //!
        //! Check the presence of a value.
        //!
        //! @return True if the variable is initialized, false otherwise.
        //!
        bool set() const { return _access != nullptr; }

        //!
        //! Set a value if the variable is currently unset.
        //! Do nothing if the variable is already initialized.
        //!
        //! @param [in] def A default @a T value if the variable is uninitialized.
        //! @return True if the variable was not initialized and the default value was set,
        //! false if the variable was already initialized.
        //!
        bool setDefault(const T& def);

        //!
        //! Clear the value.
        //!
        //! This object becomes uninitialized if it was not already.
        //!
        void clear();

        //!
        //! Access the constant @a T value inside the variable.
        //!
        //! @return A constant reference to the @a T value inside the variable.
        //! @throw UninitializedVariable If the variable is uninitialized.
        //!
        const T& value() const;

        //!
        //! Access the @a T value inside the variable.
        //!
        //! @return A reference to the @a T value inside the variable.
        //! @throw UninitializedVariable If the variable is uninitialized.
        //!
        T& value();

        //!
        //! Get a copy of the @a T value inside the variable or a default value.
        //!
        //! @param [in] def A default @a T value if the variable is uninitialized.
        //! @return A copy the @a T value inside the variable if the variable is
        //! initialized, @a def otherwise.
        //!
        T value(const T& def) const;

        //!
        //! Equality operator.
        //!
        //! @param [in] other An other instance to compare with.
        //! @return True if both instances are initialized and
        //! contain equal values.
        //!
        bool operator==(const Variable<T>& other) const;

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        //!
        //! Unequality operator.
        //!
        //! @param [in] other An other instance to compare with.
        //! @return True if any instance is uninitialized or both
        //! are initialized with unequal values.
        //!
        bool operator!=(const Variable<T>& other) const;
#endif

        //!
        //! Equality operator with a @a T instance.
        //!
        //! @param [in] obj An object to compare with.
        //! @return True if this object is initialized and its value
        //! is equal to @a obj.
        //!
        bool operator==(const T& obj) const;

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        //!
        //! Unequality operator with a @a T instance.
        //!
        //! @param [in] obj An object to compare with.
        //! @return True if this object is uninitialized or its value
        //! is not equal to @a obj.
        //!
        bool operator!=(const T& obj) const;
#endif

        //!
        //! Identity.
        //!
        //! @param [in] other An other instance to compare with.
        //! @return True if both instances are initialized and
        //! contain equal values or if both instance and not initialized.
        //!
        bool identical(const Variable<T>& other) const;
    };
}

#include "tsVariableTemplate.h"
