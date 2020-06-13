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
#include "tsTablesLoggerFilterInterface.h"
#include "tsArgs.h"
#include "tsTSPacket.h"
#include "tsSectionDemux.h"
#include "tsTextFormatter.h"
#include "tsSocketAddress.h"
#include "tsUDPSocket.h"
#include "tsCASMapper.h"
#include "tsxmlTweaks.h"
#include "tsxmlDocument.h"

namespace ts {
    //!
    //! This class logs sections and tables.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TablesLogger :
        public ArgsSupplierInterface,
        protected TableHandlerInterface,
        protected SectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TablesLogger);
    public:
        //!
        //! Constructor.
        //! @param [in,out] display Object to display tables and sections.
        //!
        TablesLogger(TablesDisplay& display);

        //!
        //! Destructor.
        //!
        virtual ~TablesLogger();

        //!
        //! Default table log size.
        //! With option -\-log, specify how many bytes are displayed at the
        //! beginning of the table payload (the header is not displayed).
        //! The default is 8 bytes.
        //!
        static constexpr size_t DEFAULT_LOG_SIZE = 8;

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;

        //!
        //! The following method feeds the logger with a TS packet.
        //! @param [in] pkt A new transport stream packet.
        //!
        void feedPacket(const TSPacket& pkt);

        //!
        //! Open files, start operations.
        //! The options must have been loaded first.
        //! @return True on success, false on error.
        //!
        bool open();

        //!
        //! Close all operations, flush tables if required, close files and sockets.
        //! No longer accept packets. Automatically done in destructor.
        //!
        void close();

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

        //!
        //! Static routine to analyze UDP messages as sent by the table logger (option --ip-udp).
        //! @param [in] data Address of UDP message.
        //! @param [in] size Size in bytes of UDP message.
        //! @param [in] no_encapsulation When true, the UDP message contains raw sections.
        //! When false, the UDP message contains a TLV structure.
        //! @param [out] sections List of sections in the message.
        //! @param [out] timestamp Time of the collection of the table. Available only in TLV message.
        //! Contains Time::Epoch if not available.
        //! @return True on success, false on invalid message.
        //!
        static bool AnalyzeUDPMessage(const uint8_t* data, size_t size, bool no_encapsulation, SectionPtrVector& sections, Time& timestamp);

    protected:
        // Implementation of interfaces.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;

    private:
        // Command line options:
        bool                     _use_text;          // Produce formatted human-readable tables.
        bool                     _use_xml;           // Produce XML tables.
        bool                     _use_binary;        // Save binary sections.
        bool                     _use_udp;           // Send sections using UDP/IP.
        UString                  _text_destination;  // Text output file name.
        UString                  _xml_destination;   // XML output file name.
        UString                  _bin_destination;   // Binary output file name.
        UString                  _udp_destination;   // UDP/IP destination address:port.
        bool                     _multi_files;       // Multiple binary output files (one per section).
        bool                     _flush;             // Flush output file.
        bool                     _rewrite_xml;       // Rewrite a new XML file for each table.
        bool                     _rewrite_binary;    // Rewrite a new binary file for each table.
        UString                  _udp_local;         // Name of outgoing local address (empty if unspecified).
        int                      _udp_ttl;           // Time-to-live socket option.
        bool                     _udp_raw;           // UDP messages contain raw sections, not structured messages.
        bool                     _all_sections;      // Collect all sections, as they appear.
        bool                     _all_once;          // Collect all sections but only once per PID/TID/TDIext/secnum/version.
        uint32_t                 _max_tables;        // Max number of tables to dump.
        bool                     _time_stamp;        // Display time stamps with each table.
        bool                     _packet_index;      // Display packet index with each table.
        bool                     _logger;            // Table logger.
        size_t                   _log_size;          // Size of table to log.
        bool                     _no_duplicate;      // Exclude duplicated short sections on a PID.
        bool                     _pack_all_sections; // Pack all sections as if they were one table.
        bool                     _pack_and_flush;    // Pack and flush incomplete tables before exiting.
        bool                     _fill_eit;          // Add missing empty sections to incomplete EIT's before exiting.
        bool                     _use_current;       // Use tables with "current" flag.
        bool                     _use_next;          // Use tables with "next" flag.
        xml::Tweaks              _xml_tweaks;        // XML tweak options.
        PIDSet                   _initial_pids;      // Initial PID's to filter.

        // Working data:
        TablesDisplay&           _display;
        DuckContext&             _duck;
        Report&                  _report;
        bool                     _abort;
        bool                     _exit;
        uint32_t                 _table_count;
        PacketCounter            _packet_count;
        SectionDemux             _demux;
        CASMapper                _cas_mapper;
        TextFormatter            _xmlOut;            // XML output formatter.
        xml::Document            _xmlDoc;            // XML root document.
        bool                     _xmlOpen;           // The XML root element is open.
        std::ofstream            _binfile;           // Binary output file.
        UDPSocket                _sock;              // Output socket.
        std::map<PID,SectionPtr> _shortSections;     // Tracking duplicate short sections by PID.
        std::map<PID,SectionPtr> _allSections;       // Tracking duplicate sections by PID (with --all-sections).
        std::set<uint64_t>       _sectionsOnce;      // Tracking sets of PID/TID/TDIext/secnum/version with --all-once.
        TablesLoggerFilterVector _section_filters;   // All registered section filters.

        // Create a binary file. On error, set _abort and return false.
        bool createBinaryFile(const UString& name);

        // Save a section in a binary file
        void saveBinarySection(const Section&);

        // Open/write/close XML tables.
        bool createXML(const UString& name);
        void saveXML(const BinaryTable& table);
        void closeXML();

        // Send UDP table and section.
        void sendUDP(const BinaryTable& table);
        void sendUDP(const Section& section);

        // Pre/post-display of a table or section
        void preDisplay(PacketCounter first, PacketCounter last);
        void postDisplay();

        // Check if a specific section must be filtered and displayed.
        bool isFiltered(const Section& section, uint16_t cas);

        // Log a section (option --log).
        void logSection(const Section& section);
    };

    //!
    //! Safe pointer for TablesLogger (not thread-safe).
    //!
    typedef SafePtr<TablesLogger,NullMutex> TablesLoggerPtr;
}
