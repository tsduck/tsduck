//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DSM-CC carousel extractor (library-level driver).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDSMCCCarousel.h"
#include "tsSectionDemux.h"
#include "tsTableHandlerInterface.h"
#include "tsTSPacket.h"

namespace ts {

    //!
    //! Drives a DSM-CC Object Carousel extraction end-to-end on a single PID.
    //!
    //! Holds the section demux, the carousel engine and the output policy
    //! (what to write, where). Callers feed raw TS packets via feedPacket();
    //! the extractor parses sections, assembles modules, resolves BIOP object
    //! paths and writes files to disk. Shared between the `dsmcc` plugin and
    //! the `tsdsmcc` standalone tool.
    //!
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL DSMCCExtractor : private TableHandlerInterface
    {
        TS_NOCOPY(DSMCCExtractor);

    public:
        //!
        //! Extraction policy.
        //!
        struct Options {
            UString out_dir {};          //!< Directory where carousel files are written. Required unless list_mode.
            bool    list_mode = false;   //!< If true, parse and report but write nothing.
            bool    dump_modules = false;//!< If true, also dump raw assembled module payloads under out_dir/modules/.
            bool    data_carousel = false;//!< If true, skip BIOP parsing and write each module as out_dir/module_XXXX.bin.
        };

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] options Extraction policy.
        //!
        DSMCCExtractor(DuckContext& duck, const Options& options);

        //!
        //! Attach the section demux to the carousel PID.
        //! @param [in] pid PID carrying the DSM-CC sections.
        //!
        void setPID(PID pid);

        //!
        //! Feed one TS packet through the demux.
        //! @param [in] pkt The packet.
        //!
        void feedPacket(const TSPacket& pkt);

        //!
        //! Flush deferred BIOP objects and, in list_mode, print the summary.
        //!
        void flush();

    private:
        struct FileEntry {
            UString path {};
            size_t  size = 0;
        };

        DuckContext&            _duck;
        Options                 _options;
        DSMCCCarousel           _carousel;
        SectionDemux            _demux;
        std::vector<FileEntry>  _files {};

        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        void onModuleCompleted(uint16_t module_id, const ByteBlock& payload);
        void onObjectReady(uint16_t module_id, const UString& name, const BIOPMessage& msg);
        void extractFile(const UString& name, const BIOPFileMessage& file);
        void printListSummary();
    };
}  // namespace ts
