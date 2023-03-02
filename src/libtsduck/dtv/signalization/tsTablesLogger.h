//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#include "tsBinaryTable.h"
#include "tsTablesLoggerFilterInterface.h"
#include "tsTime.h"
#include "tsTSPacket.h"
#include "tsSectionDemux.h"
#include "tsUDPSocket.h"
#include "tsCASMapper.h"
#include "tsxmlTweaks.h"
#include "tsxmlRunningDocument.h"
#include "tsxmlJSONConverter.h"
#include "tsjsonRunningDocument.h"

namespace ts {
    //!
    //! This class logs sections and tables.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TablesLogger :
        protected TableHandlerInterface,
        protected SectionHandlerInterface,
        protected InvalidSectionHandlerInterface
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
        virtual ~TablesLogger() override;

        //!
        //! Default table log size.
        //! With option -\-log, specify how many bytes are displayed at the
        //! beginning of the table payload (the header is not displayed).
        //! The default is 8 bytes.
        //!
        static constexpr size_t DEFAULT_LOG_SIZE = 8;

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);

        //!
        //! Set a table handler which is called for each complete table in addition to logging.
        //! When the table handler or the section handler is not null, there is no default logging.
        //! To have the tables or sections displayed, you must explicitly specify -\-text-output -.
        //! @param [in] h The new table handler.
        //!
        void setTableHandler(TableHandlerInterface* h)
        {
            _table_handler = h;
        }

        //!
        //! Set a section handler which is called for each section in addition to logging.
        //! When the table handler or the section handler is not null, there is no default logging.
        //! To have the tables or sections displayed, you must explicitly specify -\-text-output -.
        //! @param [in] h The new handler.
        //!
        void setSectionHandler(SectionHandlerInterface* h)
        {
            _section_handler = h;
        }

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
        //! Report the demux errors (if any).
        //! @param [in,out] report Output Report object.
        //! @param [in] level Severity level to report.
        //!
        void reportDemuxErrors(Report& report, int level = Severity::Info);

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
        virtual void handleInvalidSection(SectionDemux&, const DemuxedData&) override;

    private:
        // Command line options:
        bool                     _use_text;          // Produce formatted human-readable tables.
        bool                     _use_xml;           // Produce XML tables.
        bool                     _use_json;          // Produce JSON tables.
        bool                     _use_binary;        // Save binary sections.
        bool                     _use_udp;           // Send sections using UDP/IP.
        UString                  _text_destination;  // Text output file name.
        UString                  _xml_destination;   // XML output file name.
        UString                  _json_destination;  // JSON output file name.
        UString                  _bin_destination;   // Binary output file name.
        UString                  _udp_destination;   // UDP/IP destination address:port.
        bool                     _bin_multi_files;   // Multiple binary output files (one per section).
        bool                     _bin_stdout;        // Output binary sections on stdout.
        bool                     _flush;             // Flush output file.
        bool                     _rewrite_xml;       // Rewrite a new XML file for each table.
        bool                     _rewrite_json;      // Rewrite a new JSON file for each table.
        bool                     _rewrite_binary;    // Rewrite a new binary file for each table.
        bool                     _log_xml_line;      // Log tables as one XML line in the system message log.
        bool                     _log_json_line;     // Log tables as one JSON line in the system message log.
        bool                     _log_hexa_line;     // Log tables as one hexa line in the system message log.
        UString                  _log_xml_prefix;    // Prefix before XML log line.
        UString                  _log_json_prefix;   // Prefix before JSON log line.
        UString                  _log_hexa_prefix;   // Prefix before hexa log line.
        UString                  _udp_local;         // Name of outgoing local address (empty if unspecified).
        int                      _udp_ttl;           // Time-to-live socket option.
        bool                     _udp_raw;           // UDP messages contain raw sections, not structured messages.
        bool                     _all_sections;      // Collect all sections, as they appear.
        bool                     _all_once;          // Collect all sections but only once per PID/TID/TDIext/secnum/version.
        bool                     _invalid_sections;  // Display invalid sections.
        bool                     _invalid_only;      // Display invalid sections only, not valid tables and sections.
        bool                     _invalid_versions;  // Track invalid section versions.
        uint32_t                 _max_tables;        // Max number of tables to dump.
        bool                     _time_stamp;        // Display time stamps with each table.
        bool                     _packet_index;      // Display packet index with each table.
        bool                     _logger;            // Table logger.
        size_t                   _log_size;          // Size of table to log.
        bool                     _no_duplicate;      // Exclude consecutive duplicated short sections on a PID.
        bool                     _no_deep_duplicate; // Exclude duplicated sections on a PID, even non-consecutive.
        bool                     _pack_all_sections; // Pack all sections as if they were one table.
        bool                     _pack_and_flush;    // Pack and flush incomplete tables before exiting.
        bool                     _fill_eit;          // Add missing empty sections to incomplete EIT's before exiting.
        bool                     _use_current;       // Use tables with "current" flag.
        bool                     _use_next;          // Use tables with "next" flag.
        xml::Tweaks              _xml_tweaks;        // XML tweak options.
        PIDSet                   _initial_pids;      // Initial PID's to filter.
        BinaryTable::XMLOptions  _xml_options;       // XML conversion options.

        // Working data:
        TablesDisplay&           _display;
        DuckContext&             _duck;
        Report&                  _report;
        TableHandlerInterface*   _table_handler;     // If not null, also log all complete tables through this handler.
        SectionHandlerInterface* _section_handler;   // If not null, also log all sections through this handler.
        bool                     _abort;
        bool                     _exit;
        uint32_t                 _table_count;
        PacketCounter            _packet_count;
        SectionDemux             _demux;
        CASMapper                _cas_mapper;
        xml::RunningDocument     _xml_doc;           // XML document, built on-the-fly.
        xml::JSONConverter       _x2j_conv;          // XML-to-JSON converter.
        json::RunningDocument    _json_doc;          // JSON document, built on-the-fly.
        std::ofstream            _bin_file;          // Binary output file.
        UDPSocket                _sock;              // Output socket.
        std::map<PID,ByteBlock>  _short_sections;    // Tracking duplicate short sections by PID with a section hash.
        std::map<PID,ByteBlock>  _last_sections;     // Tracking duplicate sections by PID with a section hash (with --all-sections).
        std::map<PID,std::set<ByteBlock>> _deep_hashes; // Tracking of deep duplicate sections.
        std::set<uint64_t>       _sections_once;     // Tracking sets of PID/TID/TDIext/secnum/version with --all-once.
        TablesLoggerFilterVector _section_filters;   // All registered section filters.

        // Create a binary file. On error, set _abort and return false.
        bool createBinaryFile(const UString& name);

        // Save a section in a binary file
        void saveBinarySection(const Section&);

        // Log XML and/or JSON one-liners.
        void logXMLJSON(const BinaryTable& table);

        // Send UDP table and section.
        void sendUDP(const BinaryTable& table);
        void sendUDP(const Section& section);

        // Pre/post-display of a table or section
        void preDisplay(PacketCounter first, PacketCounter last);
        void postDisplay();

        // Check if a specific section must be filtered and displayed.
        bool isFiltered(const Section& section, uint16_t cas);

        // Log a section (option --log).
        UString logHeader(const DemuxedData&);
        void logSection(const Section&);
        void logInvalid(const DemuxedData&, const UString&);

        // Detect and track duplicate section by PID.
        bool isDuplicate(PID pid, const Section& section, std::map<PID,ByteBlock> TablesLogger::* tracker);
        bool isDeepDuplicate(PID pid, const Section& section);
    };

    //!
    //! Safe pointer for TablesLogger (not thread-safe).
    //!
    typedef SafePtr<TablesLogger,NullMutex> TablesLoggerPtr;
}
