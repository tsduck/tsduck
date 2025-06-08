//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFeatures.h"
#include "tsCerrReport.h"


//----------------------------------------------------------------------------
// Singleton definition.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::Features);
ts::Features::Features() {}


//----------------------------------------------------------------------------
// Register a feature.
//----------------------------------------------------------------------------

ts::Features::index_t ts::Features::registerFeature(const UString& option, const UString& name, Support support, GetVersionFunc get_version)
{
    CERR.debug(u"registering feature \"%s\"", option);

    // It is possible that the feature has already been registered as part of a shared image.
    // In that case, we are in that shared image. The feature has been declared as optional.
    index_t index = _support_enum.value(option, true, false);
    const bool new_feature = index == Names::UNKNOWN;
    if (new_feature) {
        index = _next_index++;
    }

    auto& feat(_features[index]);
    feat.name = name;
    feat.supported = support != UNSUPPORTED;
    feat.get_version = get_version;

    if (new_feature) {
        feat.option = option;
        if (support != ALWAYS) {
            _support_enum.add(option, index);
        }
        if (get_version != nullptr) {
            _version_enum.add(option, index);
        }
    }

    return index;
}


//----------------------------------------------------------------------------
// Register a feature which is in another shared image.
//----------------------------------------------------------------------------

ts::Features::index_t ts::Features::registerFeature(const UString& option, const fs::path& library)
{
    // Is the feature already registered?
    // This is an optional feature, it must be in _support_enum.
    index_t index = _support_enum.value(option, true, false);

    // Register it only if not yet registered. The shared library will be loaded later.
    if (index == Names::UNKNOWN) {
        CERR.debug(u"registering feature \"%s\", shared library: %s", option, library);
        // Define feature as available in a shared library.
        index = _next_index++;
        auto& feat(_features[index]);
        feat.option = option;
        feat.library_name = library;
        // The feature is optional and versioned.
        _support_enum.add(option, index);
        _version_enum.add(option, index);
    }

    return index;
}


//----------------------------------------------------------------------------
// Load the shared library of a feature, if there is one.
//----------------------------------------------------------------------------

void ts::Features::Feat::loadSharedLibrary()
{
    if (!library_name.empty() && !library.has_value()) {
        // A library name is specified but we never tried to load it.
        // Initialize the SharedLibrary object, this will try to load the library.
        library.emplace(library_name, u"lib", UString(), SharedLibraryFlags::PERMANENT);
    }
}


//----------------------------------------------------------------------------
// Get the description of a feature, nullptr if non-existent.
//----------------------------------------------------------------------------

const ts::Features::Feat* ts::Features::getFeature(index_t index)
{
    const auto it = _features.find(index);
    if (it == _features.end()) {
        // Non-existent feature.
        return nullptr;
    }
    else {
        // If the feature is in a shared library, try to load it once.
        it->second.loadSharedLibrary();
        return &it->second;
    }
}


//----------------------------------------------------------------------------
// Characteristics of a feature.
//----------------------------------------------------------------------------

bool ts::Features::isSupported(index_t index)
{
    const auto feat = getFeature(index);
    return feat != nullptr && feat->supported;
}

bool ts::Features::isSupported(const UString& option)
{
    return isSupported(_support_enum.value(option, true, false));
}

ts::UString ts::Features::getVersion(index_t index)
{
    const auto feat = getFeature(index);
    return feat == nullptr || feat->get_version == nullptr ? UString() : feat->get_version();
}

ts::UString ts::Features::getVersion(const UString& option)
{
    return getVersion(_support_enum.value(option, true, false));
}


//----------------------------------------------------------------------------
// Get the version of all features.
//----------------------------------------------------------------------------

std::list<std::pair<ts::UString, ts::UString>> ts::Features::getAllVersions()
{
    std::list<std::pair<UString, UString>> result;
    for (auto& it : _features) {
        it.second.loadSharedLibrary();
        if (it.second.supported && it.second.get_version != nullptr) {
            result.emplace_back(it.second.name, it.second.get_version());
        }
    }
    return result;
}
