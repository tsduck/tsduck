//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCCarousel.h"
#include "tsZlib.h"
#include "tsDSMCCBIOPMessage.h"
#include "tsPSIBuffer.h"


namespace {
    void DecompressIfNeeded(ts::ByteBlock& out, const ts::DSMCCModuleAssembler::ModuleContext& ctx, ts::Report& report)
    {
        if (!ctx.is_compressed) {
            out = ctx.payload;
        }
        else if (ts::Zlib::Decompress(out, ctx.payload)) {
            report.verbose(u"Module 0x%X decompressed size: %d", ctx.module_id, out.size());
            if (ctx.original_size > 0 && out.size() != ctx.original_size) {
                report.warning(u"Module 0x%X: decompressed size mismatch, expected %d, got %d", ctx.module_id, ctx.original_size, out.size());
            }
        }
        else {
            report.error(u"Module 0x%X: decompression failed", ctx.module_id);
            out.clear();
        }
    }
}


ts::DSMCCCarousel::DSMCCCarousel(DuckContext& duck) :
    _duck(duck),
    _assembler(duck)
{
    _assembler.setModuleCompletedHandler([this](const DSMCCModuleAssembler::ModuleContext& ctx) {
        onAssemblerModuleComplete(ctx);
    });
    _assembler.setModuleDiscoveredHandler([this](uint32_t download_id, uint16_t module_id) {
        onAssemblerModuleDiscovered(download_id, module_id);
    });
}


void ts::DSMCCCarousel::clear()
{
    _assembler.clear();
    _names.clear();
    _groups.clear();
    _module_to_dl.clear();
    _dsi_bootstrapped = false;
}


void ts::DSMCCCarousel::flushPendingObjects()
{
    _names.flush([this](uint16_t module_id, const UString& name, const BIOPMessage& msg) {
        emitObject(module_id, name, msg);
    });
}


void ts::DSMCCCarousel::emitObject(uint16_t module_id, const UString& name, const BIOPMessage& msg)
{
    if (!_on_object) {
        return;
    }
    const auto it = _module_to_dl.find(module_id);
    const uint32_t dl = (it != _module_to_dl.end()) ? it->second : 0;
    _on_object(dl, module_id, name, msg);
}


void ts::DSMCCCarousel::feedUserToNetwork(const DSMCCUserToNetworkMessage& unm)
{
    if (const auto* dsi = unm.toDSI(); dsi != nullptr && !_dsi_bootstrapped) {
        // Object-carousel DSI: ior is populated, bootstrap the SRG location.
        // Data-carousel DSI: group_info is populated; record each group so
        // module-completion accounting can attribute DDBs to the right group.
        if (_scan_biop) {
            bootstrapFromDSI(*dsi);
        }
        else {
            recordDSIGroups(*dsi);
        }
    }
    _assembler.feedUserToNetwork(unm);
}


void ts::DSMCCCarousel::recordDSIGroups(const DSMCCUserToNetworkMessage::DownloadServerInitiate& dsi)
{
    _dsi_bootstrapped = true;

    for (const auto& group : dsi.group_info.groups) {
        // group_id == download_id
        GroupContext& ctx = _groups[group.group_id];
        ctx.download_id = group.group_id;
        ctx.group_size = group.group_size;
        ctx.compatibility = group.group_compatibility;
        ctx.announced_by_dsi = true;
        _duck.report().verbose(u"DSI group: id=0x%X, size=%d, %d compatibility descriptor(s)",
                               group.group_id, group.group_size, group.group_compatibility.descs.size());
    }
}


// Extract the ServiceGateway location from the DSI's IOR and pre-seed it as a
// root in the name resolver. If nothing matches, we leave the scan fallback in
// scanBIOPObjects() to catch the SRG when its module is parsed.
void ts::DSMCCCarousel::bootstrapFromDSI(const DSMCCUserToNetworkMessage::DownloadServerInitiate& dsi)
{
    for (const auto& profile : dsi.ior.tagged_profiles) {
        if (profile.profile_id_tag != DSMCC_TAG_BIOP) {
            continue;
        }
        for (const auto& comp : profile.lite_components) {
            if (comp.component_id_tag != DSMCC_TAG_OBJECT_LOCATION) {
                continue;
            }
            _names.addRoot({comp.module_id, comp.object_key_data});
            _dsi_bootstrapped = true;
            _duck.report().verbose(u"DSI bootstrap: ServiceGateway at carousel_id=0x%X module_id=0x%X (object_key %d bytes)",
                                   comp.carousel_id, comp.module_id, comp.object_key_data.size());
            return;
        }
    }
    _duck.report().verbose(u"DSI bootstrap: no BIOP ObjectLocation in IOR, falling back to module scan");
}


void ts::DSMCCCarousel::onAssemblerModuleComplete(const DSMCCModuleAssembler::ModuleContext& ctx)
{
    ByteBlock payload;
    DecompressIfNeeded(payload, ctx, _duck.report());

    // Skip BIOP parsing when decompression failed (payload empty) or parsing disabled
    if (_scan_biop && !payload.empty()) {
        scanBIOPObjects(ctx.module_id, payload);
    }

    if (_on_module) {
        _on_module(ctx.download_id, ctx.module_id, payload);
    }

    // Group-level accounting. The discovery callback already inserted
    // module_id into module_ids; here we tally completions and emit a
    // group-completion event the first time the group fills up.
    // TODO: This is ugly, to be refactored
    auto it = _groups.find(ctx.download_id);
    if (it != _groups.end()) {
        GroupContext& gctx = it->second;
        const bool was_complete = gctx.isComplete();
        ++gctx.modules_complete;
        if (!was_complete && gctx.isComplete() && _on_group) {
            _on_group(gctx);
        }
    }
}


void ts::DSMCCCarousel::onAssemblerModuleDiscovered(uint32_t download_id, uint16_t module_id)
{
    GroupContext& ctx = _groups[download_id];
    ctx.download_id = download_id;
    ctx.module_ids.insert(module_id);
    _module_to_dl[module_id] = download_id;
}


void ts::DSMCCCarousel::scanBIOPObjects(uint16_t module_id, const ByteBlock& payload)
{
    PSIBuffer buf(_duck, payload.data(), payload.size());
    size_t count = 0;
    size_t supported = 0;

    // Parse every BIOP message in the module, hand the carousel object graph
    // (roots and parent->child name links) to the resolver, and queue the message
    // for emission. Resolution happens in drain(), so it's insensitive to message
    // order within the module and across modules.
    // TODO: to be refactored, this is a bit of a hack
    while (buf.remainingReadBytes() >= BIOPMessageHeader::HEADER_SIZE && !buf.error()) {
        const size_t before = buf.currentReadByteOffset();
        auto msg = BIOPMessage::Parse(buf);
        ++count;
        if (msg) {
            ++supported;
            const BIOPNameResolver::NameKey self {module_id, msg->object_key};
            if (msg->kindTag() == BIOPObjectKind::SERVICE_GATEWAY) {
                _names.addRoot(self);
            }
            if (const auto* bindings = msg->bindingList()) {
                _names.recordBindings(self, *bindings);
            }
            _names.defer(module_id, std::move(msg));
        }
        if (buf.error() || buf.currentReadByteOffset() == before) {
            break;
        }
    }

    _names.drain([this](uint16_t mid, const UString& name, const BIOPMessage& msg) {
        emitObject(mid, name, msg);
    });

    _duck.report().verbose(u"Module 0x%X: %d BIOP object(s), %d supported, %d bytes trailing",
                           module_id, count, supported, buf.remainingReadBytes());
}
