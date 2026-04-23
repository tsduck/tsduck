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
    // Decompress the assembled module payload when ctx.is_compressed is set. Returns an
    // empty ByteBlock on decompression failure so the caller can skip BIOP parsing
    // rather than feeding compressed bytes into it.
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
}


void ts::DSMCCCarousel::clear()
{
    _assembler.clear();
    _names.clear();
}


void ts::DSMCCCarousel::flushPendingObjects()
{
    _names.flush(_on_object);
}


void ts::DSMCCCarousel::feedUserToNetwork(const DSMCCUserToNetworkMessage& unm)
{
    // SRG bootstrap only applies to object carousels. In data-carousel mode
    // (DVB-SSU, etc.) the DSI's private_data is a GroupInfoIndication, not an
    // IOR pointing to a ServiceGateway — the upstream UNM parser reads it as
    // an IOR regardless, so dsi.ior would be garbage here. Skip bootstrap; the
    // assembler still consumes DII as usual.
    if (_scan_biop) {
        if (const auto* dsi = unm.toDSI()) {
            bootstrapFromDSI(*dsi);
        }
    }
    _assembler.feedUserToNetwork(unm);
}


// Extract the ServiceGateway location from the DSI's IOR and pre-seed it as a
// root in the name resolver. If nothing matches, we leave the scan fallback in
// scanBIOPObjects() to catch the SRG when its module is parsed.
void ts::DSMCCCarousel::bootstrapFromDSI(const DSMCCUserToNetworkMessage::DownloadServerInitiate& dsi)
{
    for (const auto& tp : dsi.ior.tagged_profiles) {
        if (tp.profile_id_tag != DSMCC_TAG_BIOP) {
            continue;
        }
        for (const auto& lc : tp.lite_components) {
            if (lc.component_id_tag != DSMCC_TAG_OBJECT_LOCATION) {
                continue;
            }
            _names.addRoot({lc.module_id, lc.object_key_data});
            _duck.report().verbose(u"DSI bootstrap: ServiceGateway at carousel_id=0x%X module_id=0x%X (object_key %d bytes)",
                                   lc.carousel_id, lc.module_id, lc.object_key_data.size());
            return;
        }
    }
    _duck.report().verbose(u"DSI bootstrap: no BIOP ObjectLocation in IOR, falling back to module scan");
}


void ts::DSMCCCarousel::onAssemblerModuleComplete(const DSMCCModuleAssembler::ModuleContext& ctx)
{
    ByteBlock payload;
    DecompressIfNeeded(payload, ctx, _duck.report());

    // Skip BIOP parsing when decompression failed (payload empty) — parsing would
    // otherwise chew through random bytes and flood the log with spurious errors.
    // Also skip when the caller has explicitly disabled it (data carousel mode).
    if (_scan_biop && !payload.empty()) {
        scanBIOPObjects(ctx.module_id, payload);
    }

    if (_on_module) {
        _on_module(ctx.module_id, payload);
    }
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

    _names.drain(_on_object);

    _duck.report().verbose(u"Module 0x%X: %d BIOP object(s), %d supported, %d bytes trailing",
                           module_id, count, supported, buf.remainingReadBytes());
}
