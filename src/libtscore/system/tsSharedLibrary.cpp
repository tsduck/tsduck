//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSharedLibrary.h"
#include "tsSysUtils.h"

#if defined(TS_UNIX)
    #include "tsBeforeStandardHeaders.h"
    #include <dlfcn.h>
    #include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Constructor: Load a shared library
//----------------------------------------------------------------------------

ts::SharedLibrary::SharedLibrary(const fs::path& filename, SharedLibraryFlags flags, Report& report) :
    _report(report),
    _flags(flags)
{
    if (!filename.empty()) {
        load(filename);
    }
}


//----------------------------------------------------------------------------
// Destructor: Unload the shared library
//----------------------------------------------------------------------------

ts::SharedLibrary::~SharedLibrary()
{
    // If mapping is not permanent, unload the shared library.
    if ((_flags & SharedLibraryFlags::PERMANENT) == SharedLibraryFlags::NONE) {
        unload();
    }
}


//----------------------------------------------------------------------------
// Try to load an alternate file if the shared library is not yet loaded.
//----------------------------------------------------------------------------

void ts::SharedLibrary::load(const fs::path& filename)
{
    if (_is_loaded) {
        return; // already loaded
    }

    _filename = filename;
    _report.debug(u"trying to load \"%s\"", _filename);

    // Load the shared library.
#if defined(TSDUCK_STATIC)
    _error = u"statically linked application";
#elif defined(TS_WINDOWS)
    _module = ::LoadLibraryExW(_filename.c_str(), nullptr, 0);
    _is_loaded = _module != nullptr;
    if (!_is_loaded) {
        _error.assignFromUTF8(SysErrorCodeMessage());
    }
#else
    _dl = ::dlopen(_filename.c_str(), RTLD_NOW | RTLD_GLOBAL);
    _is_loaded = _dl != nullptr;
    if (!_is_loaded) {
        _error.assignFromUTF8(dlerror());
    }
#endif

    // Normalize error messages
    if (!_is_loaded) {
        if (_error.empty()) {
            _error = u"error loading " + filename;
        }
        else if (_error.find(UString(filename)) == NPOS) {
            _error = filename + u": " + _error;
        }
        _report.debug(_error);
    }
}


//----------------------------------------------------------------------------
// Force unload, even if permanent
//----------------------------------------------------------------------------

void ts::SharedLibrary::unload()
{
    if (_is_loaded) {
#if defined(TSDUCK_STATIC)
        // Nothing to do, load() previously failed.
#elif defined(TS_WINDOWS)
        ::FreeLibrary(_module);
#else
        ::dlclose(_dl);
#endif
        _is_loaded = false;
    }
}


//----------------------------------------------------------------------------
// Get the value of a symbol. Return 0 on error.
//----------------------------------------------------------------------------

void* ts::SharedLibrary::getSymbol(const std::string& name) const
{
    if (!_is_loaded) {
        return nullptr;
    }
    else {
        void* result = nullptr;
#if defined(TSDUCK_STATIC)
        // Nothing to do, load() previously failed.
#elif defined(TS_WINDOWS)
        result = reinterpret_cast<void*>(::GetProcAddress(_module, name.c_str()));
#else
        result = ::dlsym(_dl, name.c_str());
#endif
        if (result == nullptr) {
            _report.debug(u"symbol %s not found in %s", name, _filename);
        }
        return result;
    }
}
