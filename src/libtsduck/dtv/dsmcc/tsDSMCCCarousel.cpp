//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCCarousel.h"
#include "tsZlib.h"


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
}


void ts::DSMCCCarousel::onAssemblerModuleComplete(const DSMCCModuleAssembler::ModuleContext& ctx)
{
    if (!_on_module) {
        return;
    }

    if (ctx.is_compressed) {
        ByteBlock uncompressed;
        if (Zlib::Decompress(uncompressed, ctx.payload)) {
            _duck.report().verbose(u"Module 0x%X decompressed size: %d", ctx.module_id, uncompressed.size());
            if (ctx.original_size > 0 && uncompressed.size() != ctx.original_size) {
                _duck.report().warning(u"Module 0x%X: decompressed size mismatch, expected %d, got %d",
                                       ctx.module_id, ctx.original_size, uncompressed.size());
            }
            _on_module(ctx.module_id, uncompressed);
        }
        else {
            _duck.report().error(u"Module 0x%X: decompression failed", ctx.module_id);
            _on_module(ctx.module_id, ctx.payload);
        }
    }
    else {
        _on_module(ctx.module_id, ctx.payload);
    }
}
