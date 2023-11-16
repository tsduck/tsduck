//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  This class logs sections and tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSectionDemux.h"
#include "tsxmlRunningDocument.h"
#include "tsxmlJSONConverter.h"
#include "tsjsonRunningDocument.h"
#include "tsPAT.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! This class logs sections and tables.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL PSILogger: private TableHandlerInterface, private SectionHandlerInterface
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
        virtual ~PSILogger() override;

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
        bool completed() const;

        //!
        //! Report the demux errors (if any).
        //!
        void reportDemuxErrors();

    private:
        // Command line options:
        bool        _all_versions = false;   // Display all versions of PSI tables.
        bool        _clear = false;          // Clear stream, do not wait for a CAT.
        bool        _cat_only = false;       // Only CAT, ignore other PSI.
        bool        _dump = false;           // Dump all sections.
        bool        _use_text = false;       // Produce formatted human-readable tables.
        bool        _use_xml = false;        // Produce XML tables.
        bool        _use_json = false;       // Produce JSON tables.
        bool        _log_xml_line = false;   // Log tables as one XML line in the system message log.
        bool        _log_json_line = false;  // Log tables as one JSON line in the system message log.
        bool        _use_current = false;    // Use PSI tables with "current" flag.
        bool        _use_next = false;       // Use PSI tables with "next" flag.
        UString     _text_destination {};    // Text output file name.
        UString     _xml_destination {};     // XML output file name.
        UString     _json_destination {};    // JSON output file name.
        UString     _log_xml_prefix {};      // Prefix before XML log line.
        UString     _log_json_prefix {};     // Prefix before JSON log line.
        xml::Tweaks _xml_tweaks {};          // XML tweak options.

        // Working data:
        TablesDisplay&           _display;
        DuckContext&             _duck;
        Report&                  _report;
        TableHandlerInterface*   _table_handler = nullptr;   // If not null, also log all complete tables through this handler.
        SectionHandlerInterface* _section_handler = nullptr; // If not null, also log all sections through this handler.
        xml::RunningDocument     _xml_doc {_report};         // XML document, built on-the-fly.
        xml::JSONConverter       _x2j_conv {_report};        // XML-to-JSON converter.
        json::RunningDocument    _json_doc {_report};        // JSON document, built on-the-fly.
        bool                     _abort = false;
        bool                     _pat_ok = false;            // Got a PAT
        bool                     _cat_ok = false;            // Got a CAT or not interested in CAT
        bool                     _sdt_ok = false;            // Got an SDT
        bool                     _bat_ok = false;            // Got a BAT
        int                      _expected_pmt = 0;          // Expected PMT count
        int                      _received_pmt = 0;          // Received PMT count
        PacketCounter            _clear_packets_cnt = 0;
        PacketCounter            _scrambled_packets_cnt = 0;
        PAT                      _previous_pat {};           // Last received PAT.
        SectionDemux             _demux {_duck, this};       // Demux reporting PSI tables.
        Standards                _standards = Standards::NONE; // List of current standards in the PSI logger.

        // Displays a binary table.
        void displayTable(const BinaryTable& table);

        // Implementations of TableHandlerInterface and SectionHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handleSection(SectionDemux&, const Section&) override;
    };

    //!
    //! Safe pointer for PSILogger (not thread-safe).
    //!
    typedef SafePtr<PSILogger, ts::null_mutex> PSILoggerPtr;
}
