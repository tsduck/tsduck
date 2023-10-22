//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  This class rebuilds MPEG tables and sections from TS packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDemux.h"
#include "tsTablesPtr.h"
#include "tsTableHandlerInterface.h"
#include "tsSectionHandlerInterface.h"
#include "tsInvalidSectionHandlerInterface.h"
#include "tsETID.h"

namespace ts {
    //!
    //! This class rebuilds MPEG tables and sections from TS packets.
    //! @ingroup mpeg
    //!
    //! Long sections are validated with CRC. Corrupted sections are not reported.
    //!
    //! Sections with the @e next indicator are ignored. Only sections with the @e current indicator are reported.
    //!
    class TSDUCKDLL SectionDemux: public AbstractDemux
    {
        TS_NOBUILD_NOCOPY(SectionDemux);
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef AbstractDemux SuperClass;

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! Contextual information (such as standards) are accumulated in the context from demuxed sections.
        //! @param [in] table_handler The object to invoke when a new complete table is extracted.
        //! @param [in] section_handler The object to invoke when any valid section is extracted.
        //! @param [in] pid_filter The set of PID's to demux.
        //!
        explicit SectionDemux(DuckContext& duck,
                              TableHandlerInterface* table_handler = nullptr,
                              SectionHandlerInterface* section_handler = nullptr,
                              const PIDSet& pid_filter = NoPID);

        // Inherited methods
        virtual void feedPacket(const TSPacket& pkt) override;

        //!
        //! Pack sections in all incomplete tables and notify these rebuilt tables.
        //!
        //! All incomplete tables which have not yet been notified are packed.
        //! This means that missing sections are ignored and the tables are
        //! built from existing sections only, as if they were contiguous.
        //! Then, the table handler is invoked for each table.
        //!
        //! This may create inconsistent tables since sections are missing.
        //! But this may me useful at the end of a table collecting sessions
        //! to grab incomplete tables.
        //!
        //! @see BinaryTable::packSections()
        //!
        void packAndFlushSections()
        {
            fixAndFlush(true, false);
        }

        //!
        //! Add missing sections in all incomplete EIT's and notify these rebuilt tables.
        //!
        //! All DVB Event Information Tables (EIT) which have not yet been notified are
        //! completed. Missing sections are added with sections without events.
        //! Then, the table handler is invoked for each table.
        //!
        //! This is typically useful at the end of processing when segmented EIT's are
        //! collected but no empty section was collected at end of segments.
        //!
        void fillAndFlushEITs()
        {
            fixAndFlush(false, true);
        }

        //!
        //! Replace the table handler.
        //! @param [in] h The new handler.
        //!
        void setTableHandler(TableHandlerInterface* h)
        {
            _table_handler = h;
        }

        //!
        //! Replace the section handler.
        //! @param [in] h The new handler.
        //!
        void setSectionHandler(SectionHandlerInterface* h)
        {
            _section_handler = h;
        }

        //!
        //! Replace the invalid section handler.
        //! This object is invoked each time an invalid section is extracted from the stream,
        //! maybe due to invalid section length, invalid CRC32, etc. This type of data block
        //! is not a valid section and is never used in the standard table or section handler.
        //! @param [in] h The new handler.
        //!
        void setInvalidSectionHandler(InvalidSectionHandlerInterface* h)
        {
            _invalid_handler = h;
        }

        //!
        //! Filter sections based on current/next indicator.
        //! @param [in] current Get "current" tables. This is true by default.
        //! @param [in] next Get "next" tables. This is false by default.
        //!
        void setCurrentNext(bool current, bool next)
        {
            _get_current = current;
            _get_next = next;
        }

        //!
        //! Track / untrack invalid section version numbers.
        //! By default, if a section version does not change, the section is ignored.
        //! When this tracking is enabled, the content of the sections are tracked and
        //! a table is demuxed when a section version does not change but the content
        //! changes. This is considered as an error according to MPEG rules.
        //! @param [in] on Track invalid section versions. This is false by default.
        //!
        void trackInvalidSectionVersions(bool on)
        {
            _track_invalid_version = on;
        }

        //!
        //! Set the log level for messages reporting transport stream errors in demux.
        //! By default, the log level is Severity::Debug.
        //! @param [in] level The new log level for messages reporting transport stream errors.
        //!
        void setTransportErrorLogLevel(int level)
        {
            _ts_error_level = level;
        }

        //!
        //! Demux status information.
        //! It contains error counters.
        //!
        struct TSDUCKDLL Status
        {
            // Members:
            uint64_t invalid_ts;       //!< Number of invalid TS packets.
            uint64_t discontinuities;  //!< Number of TS packets discontinuities.
            uint64_t scrambled;        //!< Number of scrambled TS packets (undecoded).
            uint64_t inv_sect_length;  //!< Number of invalid section length.
            uint64_t inv_sect_index;   //!< Number of invalid section index.
            uint64_t inv_sect_version; //!< Number of invalid section version (version unchanged with content change).
            uint64_t wrong_crc;        //!< Number of sections with wrong CRC32.
            uint64_t is_next;          //!< Number of sections with "next" flag (not yet applicable).
            uint64_t truncated_sect;   //!< Number of truncated sections.

            //!
            //! Default constructor.
            //!
            Status();

            //!
            //! Constructor from the current status of a SectionDemux.
            //! @param [in] demux A section demux.
            //!
            Status(const SectionDemux& demux);

            //!
            //! Reset the content of the demux status.
            //!
            void reset();

            //!
            //! Check if any counter is non zero.
            //! @return True if any error counter is not zero.
            //!
            bool hasErrors() const;

            //!
            //! Display the content of a status block.
            //! @param [in,out] strm A standard stream in output mode.
            //! @param [in] indent Left indentation size.
            //! @param [in] errors_only If true, don't report zero counters.
            //! @return A reference to the @a strm object.
            //!
            std::ostream& display(std::ostream& strm, int indent = 0, bool errors_only = false) const;

            //!
            //! Display the content of a status block.
            //! @param [in,out] report Output Report object.
            //! @param [in] level Severity level to report.
            //! @param [in] prefix Prefix string on each line.
            //! @param [in] errors_only If true, don't report zero counters.
            //!
            void display(Report& report, int level = Severity::Info, const UString& prefix = UString(), bool errors_only = false) const;
        };

        //!
        //! Get the current status of the demux.
        //! @param [out] status The returned status.
        //!
        void getStatus(Status& status) const { status = _status; }

        //!
        //! Check if the demux has errors.
        //! @return True if any error counter is not zero.
        //!
        bool hasErrors() const { return _status.hasErrors(); }

    protected:
        // Inherited methods
        virtual void immediateReset() override;
        virtual void immediateResetPID(PID pid) override;

    private:
        // Feed the depacketizer with a TS packet (PID already filtered).
        void processPacket(const TSPacket&);

        // This internal structure contains the analysis context for one TID/TIDext into one PID.
        struct ETIDContext
        {
            bool    notified = false;   // The table was reported to application through a handler
            uint8_t version = 0;        // Version of this table
            size_t  sect_expected = 0;  // Number of expected sections in table
            size_t  sect_received = 0;  // Number of received sections in table
            SectionPtrVector sects {};  // Array of sections

            // Default constructor.
            ETIDContext() = default;

            // Init for a new table.
            void init(uint8_t new_version, uint8_t last_section);

            // Notify the application if the table is complete.
            // Do not notify twice the same table.
            // If pack is true, build a packed version of the table and report it.
            // If fill_eit is true, add missing sections in EIT.
            void notify(SectionDemux& demux, bool pack, bool fill_eit);
        };

        // This internal structure contains the analysis context for one PID.
        struct PIDContext
        {
            PacketCounter pusi_pkt_index = 0;    // Index of last packet with PUSI in this PID
            uint8_t       continuity = 0;        // Last continuity counter
            bool          sync = false;          // We are synchronous in this PID
            ByteBlock     ts {};                 // TS payload buffer
            std::map<ETID,ETIDContext> tids {};  // TID analysis contexts

            // Default constructor.
            PIDContext() = default;

            // Called when packet synchronization is lost on the pid.
            void syncLost();
        };

        // Notify the application if the table is complete.
        // Do not notify twice the same table.
        // If pack is true, build a packed version of the table and report it.
        // If fill_eit is true, add missing sections in EIT.
        void fixAndFlush(bool pack, bool fill_eit);

        // Private members:
        TableHandlerInterface*          _table_handler = nullptr;
        SectionHandlerInterface*        _section_handler = nullptr;
        InvalidSectionHandlerInterface* _invalid_handler = nullptr;
        std::map<PID,PIDContext>        _pids {};
        Status _status {};
        bool   _get_current = true;
        bool   _get_next = false;
        bool   _track_invalid_version = false;
        int    _ts_error_level {Severity::Debug};
    };
}

//!
//! Output operator for the status of a ts::SectionDemux.
//! @param [in,out] strm A standard stream in output mode.
//! @param [in] status The status of a ts::SectionDemux.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::SectionDemux::Status& status)
{
    return status.display(strm);
}
