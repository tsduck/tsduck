//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsTSPacket.h"
#include "tsSectionDemux.h"
#include "tsPSILoggerOptions.h"

namespace ts {
    //!
    //! This class logs sections and tables.
    //!
    class TSDUCKDLL PSILogger : private TableHandlerInterface, private SectionHandlerInterface
    {
    public:
        //!
        //! Constructor.
        //! @param [in] options PSI logging options.
        //!
        PSILogger(PSILoggerOptions& options);

        //!
        //! Destructor.
        //!
        ~PSILogger();

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
            return _abort || (!_opt.all_versions && _pat_ok && _cat_ok && _sdt_ok && _received_pmt >= _expected_pmt);
        }

        //!
        //! Report the demux errors (if any).
        //!
        void reportDemuxErrors();

    private:
        PSILoggerOptions& _opt;
        bool              _abort;
        bool              _pat_ok;        // Got a PAT
        bool              _cat_ok;        // Got a CAT or not interested in CAT
        bool              _sdt_ok;        // Got an SDT
        bool              _bat_ok;        // Got an SDT
        int               _expected_pmt;  // Expected PMT count
        int               _received_pmt;  // Received PMT count
        PacketCounter     _clear_packets_cnt;
        PacketCounter     _scrambled_packets_cnt;
        SectionDemux      _demux;
        std::ofstream     _outfile;
        std::ostream&     _out;           // Output file 

        // Hooks
        virtual void handleTable(SectionDemux&, const BinaryTable&);
        virtual void handleSection(SectionDemux&, const Section&);
    };

    //!
    //! Safe pointer for PSILogger (not thread-safe).
    //!
    typedef SafePtr<PSILogger, NullMutex> PSILoggerPtr;
}
