//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!
//!  @file
//!  This class logs sections and tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgsSupplierInterface.h"
#include "tsTablesDisplay.h"
#include "tsSectionDemux.h"

namespace ts {
    //!
    //! This class logs sections and tables.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PSILogger :
        public ArgsSupplierInterface,
        private TableHandlerInterface,
        private SectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(PSILogger);
    public:
        //!
        //! Constructor.
        //! @param [in,out] display Object to display tables and sections.
        //!
        explicit PSILogger(TablesDisplay& display);

        //!
        //! Destructor.
        //!
        virtual ~PSILogger();

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;

        //!
        //! Open files, start operations.
        //! The options must have been loaded first.
        //! @return True on success, false on error.
        //!
        bool open();

        //!
        //! Close all operations.
        //!
        void close();

        //!
        //! The following method feeds the logger with a TS packet.
        //! @param [in] pkt A new transport stream packet.
        //!
        void feedPacket(const TSPacket& pkt);

        //!
        //! Check if an error was found.
        //! @return True when an error was found.
        //!
        bool hasErrors() const
        {
            return _abort;
        }

        //!
        //! Return true when the analysis is complete.
        //! @return True when the analysis is complete.
        //!
        bool completed() const
        {
            return _abort || (!_all_versions && _pat_ok && _cat_ok && _sdt_ok && _received_pmt >= _expected_pmt);
        }

        //!
        //! Report the demux errors (if any).
        //!
        void reportDemuxErrors();

    private:
        // Command line options:
        bool    _all_versions;  // Display all versions of PSI tables.
        bool    _clear;         // Clear stream, do not wait for a CAT.
        bool    _cat_only;      // Only CAT, ignore other PSI.
        bool    _dump;          // Dump all sections.
        UString _output;        // Destination name file.
        bool    _use_current;   // Use PSI tables with "current" flag.
        bool    _use_next;      // Use PSI tables with "next" flag.

        // Working data:
        TablesDisplay&   _display;
        DuckContext&     _duck;
        bool             _abort;
        bool             _pat_ok;        // Got a PAT
        bool             _cat_ok;        // Got a CAT or not interested in CAT
        bool             _sdt_ok;        // Got an SDT
        bool             _bat_ok;        // Got a BAT
        int              _expected_pmt;  // Expected PMT count
        int              _received_pmt;  // Received PMT count
        PacketCounter    _clear_packets_cnt;
        PacketCounter    _scrambled_packets_cnt;
        SectionDemux     _demux;         // Demux reporting PSI tables.
        Standards        _standards;     // List of current standards in the PSI logger.

        // Implementations of TableHandlerInterface and SectionHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;
    };

    //!
    //! Safe pointer for PSILogger (not thread-safe).
    //!
    typedef SafePtr<PSILogger, NullMutex> PSILoggerPtr;
}
