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
#include "tsTablesDisplay.h"
#include "tsTSPacket.h"
#include "tsSectionDemux.h"
#include "tsTablesLoggerArgs.h"
#include "tsSocketAddress.h"
#include "tsUDPSocket.h"
#include "tsCASMapper.h"

namespace ts {
    //!
    //! This class logs sections and tables.
    //!
    class TSDUCKDLL TablesLogger :
        protected TableHandlerInterface,
        protected SectionHandlerInterface
    {
    public:
        //!
        //! Constructor.
        //! @param [in] options Table logging options.
        //! @param [in,out] display Object to display tables and sections.
        //! @param [in,out] report Where to log errors.
        //!
        TablesLogger(const TablesLoggerArgs& options, TablesDisplay& display, Report& report);

        //!
        //! Destructor.
        //!
        virtual ~TablesLogger();

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
        //! Check if the operation is complete.
        //! @return True when the operation is complete (eg. max number of logged tables reached).
        //!
        bool completed() const
        {
            return _abort || _exit;
        }

        //!
        //! Report the demux errors (if any).
        //! @param [in,out] strm Output text stream.
        //!
        void reportDemuxErrors(std::ostream& strm);

    protected:
        // Implementation of interfaces.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;

        //!
        //! Log a section (option @c --log).
        //! @param [in] section The section to log.
        //!
        virtual void logSection(const Section& section);

        //!
        //! Check if a specific section must be filtered and displayed.
        //! @param [in] section The section to check.
        //! @param [in] cas The CAS family for this section.
        //! @return True if the section is filtered and must be displayed.
        //! False if the section shall not be displayed.
        //!
        virtual bool isFiltered(const Section& section, CASFamily cas) const;

    private:
        const TablesLoggerArgs&  _opt;
        TablesDisplay&           _display;
        Report&                  _report;
        bool                     _abort;
        bool                     _exit;
        uint32_t                 _table_count;
        PacketCounter            _packet_count;
        SectionDemux             _demux;
        CASMapper                _cas_mapper;
        std::ofstream            _outfile;         // Binary output file.
        UDPSocket                _sock;            // Output socket.
        std::map<PID,SectionPtr> _shortSections;   // Tracking duplicate short sections by PID.

        // Save a section in a binary file
        void saveSection(const Section&);

        // Pre/post-display of a table or section
        void preDisplay(PacketCounter first, PacketCounter last);
        void postDisplay();

        // Build header of a TLV message
        void startMessage(ByteBlock&, uint16_t message_type, PID pid);

        // Add a section into a TLV message
        void addSection(ByteBlock&, const Section&);

    private:
        // Inaccessible operations.
        TablesLogger() = delete;
        TablesLogger(const TablesLogger&) = delete;
        TablesLogger& operator=(const TablesLogger&) = delete;
    };

    //!
    //! Safe pointer for TablesLogger (not thread-safe).
    //!
    typedef SafePtr<TablesLogger,NullMutex> TablesLoggerPtr;
}
