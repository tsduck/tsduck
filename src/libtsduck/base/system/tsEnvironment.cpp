//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsEnvironment.h"
#include "tsSingleton.h"

#if defined(TS_MAC) || defined(TS_BSD)
    extern char **environ; // not defined in public headers
#endif

// External calls to environment variables are not reentrant. Use a global mutex.
TS_STATIC_INSTANCE(std::mutex, (), EnvironmentMutex);


//----------------------------------------------------------------------------
// Check if an environment variable exists
//----------------------------------------------------------------------------

bool ts::EnvironmentExists(const UString& name)
{
    std::lock_guard<std::mutex> lock(EnvironmentMutex::Instance());

#if defined(TS_WINDOWS)
    std::array<::WCHAR, 2> unused;
    return ::GetEnvironmentVariableW(name.wc_str(), unused.data(), ::DWORD(unused.size())) != 0;
#else
    return ::getenv(name.toUTF8().c_str()) != nullptr;
#endif
}


//----------------------------------------------------------------------------
// Get the value of an environment variable.
// Return default value if does not exist.
//----------------------------------------------------------------------------

ts::UString ts::GetEnvironment(const UString& name, const UString& def)
{
    std::lock_guard<std::mutex> lock(EnvironmentMutex::Instance());

#if defined(TS_WINDOWS)
    std::vector<::WCHAR> value;
    value.resize(512);
    ::DWORD size = ::GetEnvironmentVariableW(name.wc_str(), value.data(), ::DWORD(value.size()));
    if (size >= ::DWORD(value.size())) {
        value.resize(size_t(size + 1));
        size = ::GetEnvironmentVariableW(name.wc_str(), value.data(), ::DWORD(value.size()));
    }
    return size <= 0 ? def : UString(value, size);
#else
    const char* value = ::getenv(name.toUTF8().c_str());
    return value != nullptr ? UString::FromUTF8(value) : def;
#endif
}


//----------------------------------------------------------------------------
// Set the value of an environment variable.
//----------------------------------------------------------------------------

bool ts::SetEnvironment(const UString& name, const UString& value)
{
    std::lock_guard<std::mutex> lock(EnvironmentMutex::Instance());

#if defined(TS_WINDOWS)
    return ::SetEnvironmentVariableW(name.wc_str(), value.wc_str()) != 0;
#else
    // In case of error, setenv(3) is documented to return -1 but not setting errno.
    return ::setenv(name.toUTF8().c_str(), value.toUTF8().c_str(), 1) == 0;
#endif
}


//----------------------------------------------------------------------------
// Delete an environment variable.
//----------------------------------------------------------------------------

bool ts::DeleteEnvironment(const UString& name)
{
    std::lock_guard<std::mutex> lock(EnvironmentMutex::Instance());

#if defined(TS_WINDOWS)
    return ::SetEnvironmentVariableW(name.wc_str(), nullptr) != 0;
#else
    // In case of error, unsetenv(3) is documented to return -1 but and set errno.
    // It is also documented to silently ignore non-existing variables.
    return ::unsetenv(name.toUTF8().c_str()) == 0;
#endif
}


//----------------------------------------------------------------------------
// Expand environment variables inside a file path (or any string).
// Environment variable references are '$name' or '${name}'.
// In the first form, 'name' is the longest combination of letters, digits and underscore.
// A combination \$ is interpreted as a literal $, not an environment variable reference.
//----------------------------------------------------------------------------

ts::UString ts::ExpandEnvironment(const UString& path)
{
    const size_t len = path.length();
    UString expanded;
    expanded.reserve(2 * len);
    size_t index = 0;
    while (index < len) {
        if (path[index] == '\\' && index+1 < len && path[index+1] == '$') {
            // Escaped dollar
            expanded += '$';
            index += 2;
        }
        else if (path[index] != '$') {
            // Regular character
            expanded += path[index++];
        }
        else {
            // Environment variable reference.
            // First, locate variable name and move index in path.
            UString varname;
            if (++index < len) {
                if (path[index] == '{') {
                    // '${name}' format
                    const size_t last = path.find('}', index);
                    if (last == NPOS) {
                        varname = path.substr(index + 1);
                        index = len;
                    }
                    else {
                        varname = path.substr(index + 1, last - index - 1);
                        index = last + 1;
                    }
                }
                else {
                    // '$name' format
                    const size_t last = path.find_first_not_of(u"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", index);
                    if (last == NPOS) {
                        varname = path.substr(index);
                        index = len;
                    }
                    else {
                        varname = path.substr(index, last - index);
                        index = last;
                    }
                }
            }
            // Second, replace environment variable
            expanded += GetEnvironment(varname);
        }
    }
    return expanded;
}


//----------------------------------------------------------------------------
// Add a "name=value" string to a container.
// If exact is true, the definition is always valid.
// Otherwise, cleanup the string and ignore lines without "="
//----------------------------------------------------------------------------

namespace {
    void AddNameValue(ts::Environment& env, const ts::UString& line, bool exact)
    {
        ts::UString s(line);

        // With loose line, do some initial cleanup.
        if (!exact) {
            s.trim();
            if (s.empty() || s.front() == u'#') {
                // Empty or comment line
                return;
            }
        }

        // Locate the "=" between name and value.
        const size_t pos = s.find(u"=");

        if (pos == ts::NPOS) {
            // With exact line, no "=" means empty value.
            // With loose line, not a valid definition.
            if (exact) {
                env.insert(std::make_pair(s, ts::UString()));
            }
        }
        else {
            // Isolate name and value.
            ts::UString name(s.substr(0, pos));
            ts::UString value(s.substr(pos + 1));
            // With loose line, do some additional cleanup.
            if (!exact) {
                name.trim();
                value.trim();
                if (value.size() >= 2 && (value.front() == u'\'' || value.front() == u'"') && value.back() == value.front()) {
                    // Remove surrounding quotes in the value.
                    value.pop_back();
                    value.erase(0, 1);
                }
            }
            if (!name.empty()) {
                env.insert(std::make_pair(name, value));
            }
        }
    }
}


//----------------------------------------------------------------------------
// Get the content of the entire environment (all environment variables).
//----------------------------------------------------------------------------

void ts::GetEnvironment(Environment& env)
{
    std::lock_guard<std::mutex> lock(EnvironmentMutex::Instance());
    env.clear();

#if defined(TS_WINDOWS)

    const ::LPWCH strings = ::GetEnvironmentStringsW();
    if (strings != 0) {
        size_t len;
        for (const ::WCHAR* p = strings; (len = ::wcslen(p)) != 0; p += len + 1) {
            assert(sizeof(::WCHAR) == sizeof(UChar));
            AddNameValue(env, UString(reinterpret_cast<const UChar*>(p), len), true);
        }
        ::FreeEnvironmentStringsW(strings);
    }

#else

    for (char** p = ::environ; *p != nullptr; ++p) {
        AddNameValue(env, UString::FromUTF8(*p), true);
    }

#endif
}


//----------------------------------------------------------------------------
// Load a text file containing environment variables.
//----------------------------------------------------------------------------

bool ts::LoadEnvironment(Environment& env, const UString& fileName)
{
    env.clear();
    UStringList lines;
    const bool ok = UString::Load(lines, fileName);
    if (ok) {
        for (const auto& it : lines) {
            AddNameValue(env, it, false);
        }
    }
    return ok;
}
