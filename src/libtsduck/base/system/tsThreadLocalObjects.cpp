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

#include "tsThreadLocalObjects.h"
#include "tsFatal.h"

TS_DEFINE_SINGLETON(ts::ThreadLocalObjects);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::ThreadLocalObjects::ThreadLocalObjects() :
#if defined(TS_WINDOWS)
    _tls_index(::TlsAlloc())
#else
    _key(0)
#endif
{
#if defined(TS_WINDOWS)
    if (_tls_index == TLS_OUT_OF_INDEXES) {
        const char err[] = "ThreadLocalObjects: TlsAlloc fatal error";
        FatalError(err, sizeof(err)-1);
    }
#else
    if (::pthread_key_create(&_key, DeleteThreadLocalRepository) != 0) {
        const char err[] = "ThreadLocalObjects: pthread_key_create fatal error";
        FatalError(err, sizeof(err)-1);
    }
#endif
}

ts::ThreadLocalObjects::ThreadLocalRepository::ThreadLocalRepository() :
    objects()
{
}


//----------------------------------------------------------------------------
// Get or create the ThreadLocalRepository of the current thread. Never null.
//----------------------------------------------------------------------------

ts::ThreadLocalObjects::ThreadLocalRepository* ts::ThreadLocalObjects::getCurrentRepo()
{
    // Get the thread-local storage for our index or key.
    // When none found, this is the first time we acccess this thread, create the repo.
    // No need to synchronize, we work on local thread data.

#if defined(TS_WINDOWS)
    ThreadLocalRepository* repo = reinterpret_cast<ThreadLocalRepository*>(::TlsGetValue(_tls_index));
    if (repo == nullptr) {
        repo = new ThreadLocalRepository;
        CheckNonNull(repo);
        if (::TlsSetValue(_tls_index, repo) == 0) {
            const char err[] = "ThreadLocalObjects: TlsSetValue fatal error";
            FatalError(err, sizeof(err)-1);
        }
    }
    return repo;
#else
    ThreadLocalRepository* repo = reinterpret_cast<ThreadLocalRepository*>(::pthread_getspecific(_key));
    if (repo == nullptr) {
        repo = new ThreadLocalRepository;
        CheckNonNull(repo);
        if (::pthread_setspecific(_key, repo) != 0) {
            const char err[] = "ThreadLocalObjects: pthread_setspecific fatal error";
            FatalError(err, sizeof(err)-1);
        }
    }
    return repo;
#endif
}


//----------------------------------------------------------------------------
// Delete all local objects in the current thread.
//----------------------------------------------------------------------------

void ts::ThreadLocalObjects::deleteLocalObjects()
{
#if defined(TS_WINDOWS)
    ThreadLocalRepository* repo = reinterpret_cast<ThreadLocalRepository*>(::TlsGetValue(_tls_index));
    if (repo != nullptr) {
        ::TlsSetValue(_tls_index, nullptr);
        delete repo;
    }
#else
    ThreadLocalRepository* repo = reinterpret_cast<ThreadLocalRepository*>(::pthread_getspecific(_key));
    if (repo != nullptr) {
        ::pthread_setspecific(_key, nullptr);
        delete repo;
    }
#endif
}


//----------------------------------------------------------------------------
// Cleanup functions.
//----------------------------------------------------------------------------

#if !defined(TS_WINDOWS)
void ts::ThreadLocalObjects::DeleteThreadLocalRepository(void* arg)
{
    // The arg points to the ThreadLocalRepository of the thread.
    // Deallocating this structure calls the destructors of all safe pointers.
    delete reinterpret_cast<ThreadLocalRepository*>(arg);
}
#endif


//----------------------------------------------------------------------------
// Get/set user local storage.
//----------------------------------------------------------------------------

void ts::ThreadLocalObjects::setLocalObject(const UString& name, const ObjectPtr& obj)
{
    getCurrentRepo()->objects[name] = obj;
}

ts::ObjectPtr ts::ThreadLocalObjects::getLocalObject(const UString& name)
{
    return getCurrentRepo()->objects[name];
}
