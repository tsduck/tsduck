//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsThreadLocalObjects.h"
#include "tsFatal.h"

TS_DEFINE_SINGLETON(ts::ThreadLocalObjects);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::ThreadLocalObjects::ThreadLocalObjects()
{
#if defined(TS_WINDOWS)
    if ((_tls_index = ::TlsAlloc()) == TLS_OUT_OF_INDEXES) {
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
