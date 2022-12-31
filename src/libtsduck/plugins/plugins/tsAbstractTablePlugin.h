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
//!  Abstract base class for plugins which process one table (PAT, CAT, etc.)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsTablePatchXML.h"

namespace ts {
    //!
    //! Abstract base class for plugins which process one type of table (PAT, CAT, etc.)
    //! @ingroup plugin
    //!
    class TSDUCKDLL AbstractTablePlugin : public ProcessorPlugin, protected TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(AbstractTablePlugin);
    public:
        // Implementation of ProcessorPlugin interface.
        // If overridden by subclass, superclass must be explicitly invoked.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

        //!
        //! Default bitrate of new PID if a table is created.
        //!
        static constexpr BitRate::int_t DEFAULT_BITRATE = 3000;

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] tsp Object to communicate with the Transport Stream Processor main executable.
        //! @param [in] description A short one-line description, eg. "Descrambler for 'xyz' CAS".
        //! @param [in] syntax A short one-line syntax summary, default: u"[options] [service]".
        //! @param [in] table_name Name of the table to process (for help text).
        //! @param [in] pid PID containing the tables to process. Does nothing when PID_NULL.
        //! @param [in] default_bitrate Default bitrate of new PID if a table is created.
        //! @param [in] new_table_help Additional help text for the creation of a new table.
        //!
        AbstractTablePlugin(TSP* tsp,
                            const UString& description,
                            const UString& syntax,
                            const UString& table_name,
                            PID pid = PID_NULL,
                            BitRate default_bitrate = DEFAULT_BITRATE,
                            const UString& new_table_help = UString());

        //!
        //! Set a new PID to process.
        //! @param [in] pid PID containing the tables to process. Does nothing when PID_NULL.
        //!
        void setPID(PID pid);

        //!
        //! Modify one table from the PID to process.
        //! Must be implemented by subclasses.
        //! @param [in,out] table A table from the processed PID. Can be modified by handleTable().
        //! @param [in,out] is_target Indicate that @a table is the one we are looking for.
        //! Initially true. Can be set to false by handleTable() to indicate that this is another
        //! table from the same PID as the target table.
        //! @param [in,out] reinsert Indicate that the modified @a table shall be reinserted in the
        //! PID. Initially true. Can be set to false by handleTable() to indicate that this table
        //! shall be removed from the PID.
        //!
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert) = 0;

        //!
        //! Create a new empty table when none is found in the PID.
        //! Must be implemented by subclasses.
        //! @param [out] table The default empty table.
        //!
        virtual void createNewTable(BinaryTable& table) = 0;

        //!
        //! Called by the subclass when some external event forces an update of the table.
        //! Most subclasses will not need to call this.
        //! @param [in,out] table The new updated table.
        //! Modified when common modification options are specified.
        //!
        void forceTableUpdate(BinaryTable& table);

        //!
        //! Set the error flag to terminate the processing asap.
        //! @param [in] on Error state (true by default).
        //!
        void setError(bool on = true) { _abort = on; }

        //!
        //! Check if the error flags was set.
        //! @return True if an error was set.
        //!
        bool hasError() const { return _abort; }

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

    private:
        bool              _abort;            // Error, abort as soon as possible.
        UString           _table_name;       // Table name, informational only.
        BitRate           _default_bitrate;  // Default bitrate of new PID.
        PID               _pid;              // PID to process.
        bool              _found_pid;        // Found the target PID.
        bool              _found_table;      // Found an instance of the target table.
        PacketCounter     _pkt_create;       // Packet# after which a new table shall be created
        PacketCounter     _pkt_insert;       // Packet# after which a PID packet shall be inserted
        MilliSecond       _create_after_ms;  // Create a new table if none found after that time.
        BitRate           _bitrate;          // PID's bitrate (if no previous table found).
        PacketCounter     _inter_pkt;        // Packet interval between two PID packets.
        bool              _incr_version;     // Increment table version.
        bool              _set_version;      // Set a new table version.
        uint8_t           _new_version;      // New table version.
        SectionDemux      _demux;            // Section demux.
        CyclingPacketizer _pzer;             // Packetizer for modified tables.
        TablePatchXML     _patch_xml;        // Table patcher using XML patch files.

        // Reinsert a table in the target PID.
        void reinsertTable(BinaryTable& table, bool is_target_table);
    };
}
