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
//!  Template safe pointer (reference-counted, auto-delete, thread-safe).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsFatal.h"
#include "tsGuardMutex.h"
#include "tsMutex.h"
#include "tsNullMutex.h"

namespace ts {
    //!
    //!  Template safe pointer (reference-counted, auto-delete, thread-safe).
    //!  @ingroup cpp
    //!
    //!  The ts::SafePtr template class is an implementation of the
    //!  @e safe @e pointer design pattern. A safe pointer implements the
    //!  semantics of a standard pointer with automatic memory management.
    //!
    //!  Safe pointer objects pointing to the same object can be assigned
    //!  like any standard pointer (elementary type). A reference counter
    //!  on the pointed object is maintained and the pointed object is
    //!  automatically deleted when no more safe pointer references the
    //!  object.
    //!
    //!  @b Limitation: The automatic deletion of the pointed object occurs
    //!  @em only when the reference counter reaches zero. There are
    //!  cases where this never happens. Typically, when two objects reference
    //!  each other but are no longer referenced anywhere else, these two objects
    //!  are lost forever and will never be deleted. So, beware that smart
    //!  pointers do not prevent from memory leaks in some pathological cases.
    //!  As usual, be sure to design safely.
    //!
    //!  @b Limitation: Because the automatic deletion is performed using the
    //!  operator @c delete, safe pointers cannot point on array types since
    //!  arrays must be deleted using the operator @c delete[].
    //!
    //!  The standard operators for elementary type pointers also exist for
    //!  safe pointers (assignment, comparison, dereferencing, etc.)
    //!
    //!  A safe pointer can be @e null, this is the default value. In this case,
    //!  the safe pointer does not reference any object. To test if a safe
    //!  pointer is a null pointer, use the method @c isNull(). Do not
    //!  use comparisons such as <code>p == nullptr</code>, the result will be incorrect.
    //!
    //!  The ts::SafePtr template class can be made thread-safe using a mutex.
    //!  The type of mutex to use is given by the template parameter @a MUTEX
    //!  which must be a subclass of ts::MutexInterface. By default,
    //!  ts::NullMutex is used. The default implementation is consequently
    //!  not thread-safe but there is no synchronization overhead. To use
    //!  safe pointers in a multi-thread environment, specify an actual
    //!  mutex implementation for the target environment.
    //!
    //!  @tparam T The type of the pointed object. Cannot be an array type.
    //!  @tparam MUTEX A subclass of ts::MutexInterface which is used to
    //!  synchronize access to the safe pointer internal state.
    //!
    template <typename T, class MUTEX = NullMutex>
    class SafePtr
    {
    public:
        //!
        //! Generic definition of the pointed type for this safe pointer.
        //!
        typedef T DataType;

        //!
        //! Generic definition of the mutex for this safe pointer.
        //!
        typedef MUTEX MutexType;

        //!
        //! Default constructor using an optional unmanaged object.
        //!
        //! The optional argument @a p can be either @c nullptr
        //! or the address of an @e unmanaged dynamically allocated object
        //! (i.e. which has been allocated using the operator @c new).
        //! In this case, @e unmanaged means that the object must not
        //! be already controlled by another set of safe pointers.
        //!
        //! This constructor is typically used in combination with the
        //! operator @c new to allocate an object which is immediatly
        //! managed by safe pointers.
        //!
        //! Example:
        //! @code
        //! ts::SafePtr<Foo> ptr(new Foo(...));
        //! @endcode
        //!
        //! @param [in] p A pointer to an object of class @a T.
        //! The default value is @c nullptr. In this
        //! case, the safe pointer is a null pointer.
        //! @exception std::bad_alloc Thrown if insufficient memory
        //! is available for internal safe pointer management.
        //!
        SafePtr(T* p = nullptr) :
            _shared(new SafePtrShared(p))
        {
        }

        //!
        //! Copy constructor.
        //!
        //! This object references the same @a T object as @a sp.
        //! If @a sp is a null pointer, this object is also a null pointer.
        //!
        //! @param [in] sp Another safe pointer instance.
        //!
        SafePtr(const SafePtr<T,MUTEX>& sp) :
            _shared(sp._shared->attach())
        {
        }

        //!
        //! Move constructor.
        //!
        //! @param [in,out] sp Another safe pointer instance.
        //!
        SafePtr(SafePtr<T,MUTEX>&& sp) noexcept :
            _shared(sp._shared)
        {
            sp._shared = nullptr;
        }

        //!
        //! Destructor.
        //!
        //! If this object is not a null pointer, the reference counter
        //! is decremented. When the reference counter reaches zero, the
        //! pointed object is automatically deleted.
        //!
        ~SafePtr();

        //!
        //! Assignment between safe pointers.
        //!
        //! After the assignment, this object references the same @a T object
        //! as @a sp. If this object was previously not the null pointer, the
        //! reference counter of the previously referenced @a T object is
        //! decremented. If the reference counter reaches zero, the previously
        //! pointed object is automatically deleted.
        //!
        //! @param [in] sp The value to assign.
        //! @return A reference to this object.
        //!
        SafePtr<T,MUTEX>& operator=(const SafePtr<T,MUTEX>& sp);

        //!
        //! Move assignment between safe pointers.
        //!
        //! @param [in,out] sp The value to assign.
        //! @return A reference to this object.
        //!
        SafePtr<T,MUTEX>& operator=(SafePtr<T,MUTEX>&& sp) noexcept;

        //!
        //! Assignment from a standard pointer @c T*.
        //!
        //! The pointed @c T object becomes managed by this safe pointer.
        //!
        //! The standard pointer @a p can be either @c nullptr
        //! or the address of an @e unmanaged dynamically allocated object
        //! (i.e. which has been allocated using the operator @c new).
        //! In this case, @e unmanaged means that the object must not
        //! be already controlled by another set of safe pointers.
        //!
        //! After the assignment, this object references the @a T object.
        //! If this safe pointer object was previously not the null pointer, the
        //! reference counter of the previously referenced @a T object is
        //! decremented. If the reference counter reaches zero, the previously
        //! pointed object is automatically deleted.
        //!
        //! @param [in] p A pointer to an object of class @a T.
        //! @return A reference to this object.
        //! @exception std::bad_alloc Thrown if insufficient memory
        //! is available for internal safe pointer management.
        //!
        SafePtr<T,MUTEX>& operator=(T* p);

        //!
        //! Equality operator.
        //!
        //! Check if this safe pointer and the @a sp safe pointer point to same object.
        //!
        //! @b Caveat: Null pointers are not reliably compared with this operator.
        //! It shall not be used to compare against null pointer. Do not
        //! check <code>== nullptr</code>, use the method @c isNull() instead.
        //! Also, if both safe pointers are null pointers, the result is
        //! unpredictable, it can be true or false.
        //!
        //! @param [in] sp A safe pointer to compare with.
        //! @return True if both safe pointers reference the same object
        //! and false otherwise.
        //!
        bool operator==(const SafePtr<T,MUTEX> &sp) const
        {
            return sp._shared == this->_shared;
        }

#if defined(TS_NEED_UNEQUAL_OPERATOR)
        //!
        //! Unequality operator.
        //!
        //! Check if this safe pointer and the @a sp safe pointer point to different objects.
        //!
        //! @b Caveat: Null pointers are not reliably compared with this operator.
        //! It shall not be used to compare against null pointer. Do not
        //! check <code>!= nullptr</code>, use the method @c isNull() instead.
        //! Also, if both safe pointers are null pointers, the result is
        //! unpredictable, it can be true or false.
        //!
        //! @param [in] sp A safe pointer to compare with.
        //! @return True if both safe pointers reference distinct objects
        //! and false otherwise.
        //!
        bool operator!=(const SafePtr<T,MUTEX> &sp) const
        {
            return sp._shared != this->_shared;
        }
#endif

        //!
        //! Redirection operator.
        //!
        //! With this operator, a safe pointer can be used in the same way as a
        //! standard pointer.
        //!
        //! Example:
        //! @code
        //! class Foo
        //! {
        //! public:
        //!     void open();
        //! };
        //!
        //! ts::SafePtr<Foo> ptr(new Foo);
        //! ptr->open();
        //! @endcode
        //!
        //! If this object is the null pointer, the operator returns zero and
        //! the further dereferencing operation will likely throw an exception.
        //!
        //! @return A standard pointer @c T* to the pointed object or
        //! @c nullptr if this object is the null pointer.
        //!
        T* operator->() const
        {
            return _shared->pointer();
        }

        //!
        //! Accessor operator.
        //!
        //! With this operator, a safe pointer can be used in the same way as a
        //! standard pointer.
        //!
        //! Example:
        //! @code
        //! void f(Foo&);
        //!
        //! ts::SafePtr<Foo> ptr(new Foo);
        //! f(*ptr);
        //! @endcode
        //!
        //! If this object is the null pointer, the operator will likely throw an exception.
        //!
        //! @return A reference @c T& to the pointed object.
        //!
        T& operator*() const
        {
            return *_shared->pointer();
        }

        //!
        //! Release the pointed object from the safe pointer management.
        //!
        //! The previously pointed object is not deallocated,
        //! its address is returned. All safe pointers which pointed
        //! to the object now point to @c nullptr.
        //!
        //! @b Caveat: The previously pointed object will no longer
        //! be automatically deleted. The caller must explicitly delete
        //! it later using the returned pointer value.
        //!
        //! @return A standard pointer @c T* to the previously pointed object.
        //! Return @c nullptr if this object was the null pointer.
        //!
        T* release()
        {
            return _shared->release();
        }

        //!
        //! Deallocate the previous pointed object and set the pointer to the new object.
        //!
        //! All safe pointers which pointed to the same object now point to the new one.
        //! The previously pointed object is deleted using the operator @c delete.
        //!
        //! The standard pointer @a p can be either @c nullptr
        //! or the address of an @e unmanaged dynamically allocated object
        //! (i.e. which has been allocated using the operator @c new).
        //! In this case, @e unmanaged means that the object must not
        //! be already controlled by another set of safe pointers.
        //!
        //! @param [in] p A pointer to an object of class @a T.
        //!
        void reset(T *p = nullptr)
        {
            _shared->reset(p);
        }

        //!
        //! Clear this instance of the safe pointer.
        //!
        //! The referenced object is deallocated if no more reference exists.
        //! Then, this safe pointer becomes the null pointer.
        //! @c sp.clear() is equivalent to <code>sp = (T*)(nullptr)</code>.
        //!
        //! @exception std::bad_alloc Thrown if insufficient memory
        //! is available for internal safe pointer management.
        //!
        void clear()
        {
            _shared->detach();
            _shared = new SafePtrShared(nullptr);
        }

        //!
        //! Check if this safe pointer is a null pointer.
        //!
        //! @return True if this safe pointer is a null pointer,
        //! false otherwise.
        //!
        bool isNull() const
        {
            return _shared->isNull();
        }

        //!
        //! Upcast operation.
        //!
        //! This method converts a safe pointer to an object of class @a T into a safe
        //! pointer to an object of a super-class @a ST of @a T.
        //!
        //! If this object is not the null pointer, the ownership of the pointed object
        //! is @e transfered to a new safe pointer to @a ST. This new safe pointer is
        //! returned. This object (the safe pointer to @a T) and all other safe pointers
        //! to @a T which pointed to the same object become null pointers.
        //!
        //! @tparam ST A super-class of @a T (immediate or indirect).
        //! @return If this object is not the null pointer, return a safe pointer to the
        //! same object, interpreted as @a ST. Otherwise, return the null pointer. The
        //! @a MUTEX type of the returned safe pointer is the same as used in this object.
        //!
        template <typename ST, typename std::enable_if<std::is_base_of<ST, T>::value>::type* = nullptr>
        SafePtr<ST,MUTEX> upcast()
        {
            return _shared->template upcast<ST>();
        }

        //!
        //! Downcast operation.
        //!
        //! This method converts a safe pointer to an object of class @a T into a safe
        //! pointer to an object of a subclass @a ST of @a T.
        //! This operation is equivalent to a @a dynamic_cast operator on regular pointers.
        //!
        //! If this object is not the null pointer and points to an instance of @a ST,
        //! the ownership of the pointed object is @e transfered to a new safe pointer
        //! to @a ST. This new safe pointer is returned. This object (the safe pointer
        //! to @a T) and all other safe pointers to @a T which pointed to the same
        //! object become null pointers.
        //!
        //! If this object is the null pointer or points to a @a T object which is not
        //! an instance of @a ST, the returned value is the null pointer and this object
        //! is unmodified.
        //!
        //! @tparam ST A subclass of @a T (immediate or indirect).
        //! @return If this object is not the null pointer and points to an instance of @a ST,
        //! return a safe pointer to the same object, interpreted as @a ST. Otherwise, return
        //! the null pointer. The @a MUTEX type of the returned safe pointer is the same as
        //! used in this object.
        //!
        template <typename ST, typename std::enable_if<std::is_base_of<T, ST>::value>::type* = nullptr>
        SafePtr<ST,MUTEX> downcast()
        {
            return _shared->template downcast<ST>();
        }

        //!
        //! Change mutex type.
        //!
        //! This method converts a safe pointer to an object of class @a T into a safe
        //! pointer to the same class @a T with a different class of mutex.
        //!
        //! If this object is not the null pointer, the ownership of the pointed object
        //! is @e transfered to a new safe pointer. This new safe pointer is returned.
        //! This object and all other safe pointers of the same class which pointed to
        //! the same object become null pointers.
        //!
        //! @tparam NEWMUTEX Another subclass of ts::MutexInterface which is used to
        //! synchronize access to the new safe pointer internal state.
        //! @return A safe pointer to the same object.
        //!
        template <typename NEWMUTEX>
        SafePtr<T,NEWMUTEX> changeMutex()
        {
            return _shared->template changeMutex<NEWMUTEX>();
        }

        //!
        //! Get a standard pointer @c T* on the pointed object.
        //!
        //! @b Warning: This is an unchecked operation. Do not store
        //! this pointer to dereference it later since the pointed
        //! object may have been deleted in the meantime if no more
        //! safe pointer reference the object.
        //!
        //! @return A standard pointer @c T* to the pointed object
        //! or @c nullptr if this object is the null pointer.
        //!
        T* pointer() const
        {
            return _shared->pointer();
        }

        //!
        //! Get the reference count value.
        //!
        //! This is informational only. In multi-threaded environments,
        //! the actual reference count may change before the result is
        //! actually used.
        //!
        //! @return The number of safe pointer objects which reference
        //! the same pointed object.
        //!
        int count() const
        {
            return _shared->count();
        }

    private:
        // All safe pointers which reference the same T object share one single SafePtrShared object.
        class SafePtrShared
        {
            TS_NOBUILD_NOCOPY(SafePtrShared);
        private:
            // Private members:
            T*    _ptr;        // pointer to actual object
            int   _ref_count;  // reference counter
            MUTEX _mutex;      // protect the SafePtrShared

        public:
            // Constructor. Initial reference count is 1.
            SafePtrShared(T* p) : _ptr(p), _ref_count(1), _mutex() {}

            // Destructor. Deallocate actual object (if any).
            ~SafePtrShared();

            // Same semantics as SafePtr counterparts:
            T* release();
            void reset(T* p);
            T* pointer();
            int count();
            bool isNull();

            // Increment reference count and return this.
            SafePtrShared* attach();

            // Decrement reference count and deallocate this if needed.
            // Return true if deleted, false otherwise.
            bool detach();

            // The following three methods have the same semantics as their SafePtr counterparts.
            // Their definitions were previously in file tsSafePtrTemplate.h. But the double
            // template construct caused a failure in Coverity. So we inline them here now.
            // Note that GCC, Clang and Visual Studio always worked properly with the
            // separate definitions.

            // Perform a class downcast (cast to a subclass).
            template <typename ST> SafePtr<ST,MUTEX> downcast()
            {
                GuardMutex lock(_mutex);
                ST* sp = dynamic_cast<ST*>(_ptr);
                if (sp != nullptr) {
                    // Successful downcast, the original safe pointer must be released.
                    _ptr = nullptr;
                }
                return SafePtr<ST,MUTEX>(sp);
            }

            // Perform a class upcast.
            template <typename ST> SafePtr<ST,MUTEX> upcast()
            {
                GuardMutex lock(_mutex);
                ST* sp = _ptr;
                _ptr = nullptr;
                return SafePtr<ST,MUTEX>(sp);
            }

            // Change mutex type.
            template <typename NEWMUTEX> SafePtr<T,NEWMUTEX> changeMutex()
            {
                GuardMutex lock(_mutex);
                T* sp = _ptr;
                _ptr = nullptr;
                return SafePtr<T,NEWMUTEX>(sp);
            }
        };

        // This is the only member field in SafePtr.
        SafePtrShared* _shared;
    };
}

#include "tsSafePtrTemplate.h"
