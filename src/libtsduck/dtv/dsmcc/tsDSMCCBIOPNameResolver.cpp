//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCBIOPNameResolver.h"


void ts::BIOPNameResolver::clear()
{
    _parent_link.clear();
    _roots.clear();
    _pending.clear();
}


void ts::BIOPNameResolver::addRoot(const NameKey& key)
{
    _roots.insert(key);
}


void ts::BIOPNameResolver::recordBindings(const NameKey& parent, const std::vector<BIOPBinding>& bindings)
{
    for (const auto& b : bindings) {
        const auto target = b.targetLocation();
        if (!target || b.name.empty()) {
            continue;
        }
        // Keep the first parent link seen: carousel versions in flight may contain
        // duplicate or conflicting bindings; first-wins is as valid as last-wins
        // and avoids flapping while a rebuild is in progress.
        _parent_link.try_emplace(*target, ParentLink{parent, b.pathString()});
    }
}


void ts::BIOPNameResolver::defer(uint16_t module_id, std::unique_ptr<BIOPMessage> msg)
{
    _pending.push_back(PendingEmit{module_id, std::move(msg)});
}


void ts::BIOPNameResolver::drain(const EmitCallback& cb)
{
    // Single pass is sufficient: _parent_link is not mutated during a drain.
    for (auto it = _pending.begin(); it != _pending.end();) {
        const UString name = resolveName({it->module_id, it->msg->object_key});
        if (!name.empty()) {
            if (cb) {
                cb(it->module_id, name, *it->msg);
            }
            it = _pending.erase(it);
        }
        else {
            ++it;
        }
    }
}


void ts::BIOPNameResolver::flush(const EmitCallback& cb)
{
    drain(cb);
    if (cb) {
        for (auto& p : _pending) {
            cb(p.module_id, UString(), *p.msg);
        }
    }
    _pending.clear();
}


// Walk child -> parent -> ... -> root, joining local names. Returns empty
// when no path to a root exists (parent module not yet parsed) or on cycle.
ts::UString ts::BIOPNameResolver::resolveName(const NameKey& key) const
{
    UString path;
    if (_roots.contains(key)) {
        path = u"/";
    }
    else {
        std::set<NameKey> visited;
        for (NameKey current = key; ;) {
            if (!visited.insert(current).second) {
                path.clear();
                break;
            }
            const auto it = _parent_link.find(current);
            if (it == _parent_link.end()) {
                path.clear();
                break;
            }
            path = u"/" + it->second.local_name + path;
            current = it->second.parent;
            if (_roots.contains(current)) {
                break;
            }
        }
    }
    return path;
}
