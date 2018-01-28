//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  This class rebuilds MPEG tables and sections from TS packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDemux.h"
#include "tsETID.h"
#include "tsTableHandlerInterface.h"
#include "tsSectionHandlerInterface.h"

namespace ts {
    //!
    //! This class rebuilds MPEG tables and sections from TS packets.
    //!
    //! Long sections are validated with CRC. Corrupted sections are not reported.
    //!
    //! Sections with the @e next indicator are ignored. Only sections with the @e current indicator are reported.
    //!
    class TSDUCKDLL SectionDemux: public AbstractDemux
    {
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef AbstractDemux SuperClass;

        //!
        //! Constructor.
        //! @param [in] table_handler The object to invoke when a new complete table is extracted.
        //! @param [in] section_handler The object to invoke when any section is extracted.
        //! @param [in] pid_filter The set of PID's to demux.
        //!
        SectionDemux(TableHandlerInterface* table_handler = 0,
                     SectionHandlerInterface* section_handler = 0,
                     const PIDSet& pid_filter = NoPID);

        //!
        //! Destructor.
        //!
        virtual ~SectionDemux();

        // Inherited methods
        virtual void feedPacket(const TSPacket& pkt) override;

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
            uint64_t wrong_crc;        //!< Number of sections with wrong CRC32.

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
        };

        //!
        //! Get the current status of the demux.
        //! @param [out] status The returned status.
        //!
        void getStatus(Status& status) const
        {
            status = _status;
        }

        //!
        //! Check if the demux has errors.
        //! @return True if any error counter is not zero.
        //!
        bool hasErrors() const
        {
            return _status.hasErrors();
        }

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
            uint8_t  version;        // Version of this table
            size_t   sect_expected;  // Number of expected sections in table
            size_t   sect_received;  // Number of received sections in table
            SectionPtrVector sects;  // Array of sections

            // Default constructor:
            ETIDContext() :
                version(0),
                sect_expected(0),
                sect_received(0),
                sects()
            {
            }
        };

        // This internal structure contains the analysis context for one PID.
        struct PIDContext
        {
            uint8_t continuity;                // Last continuity counter
            bool sync;                         // We are synchronous in this PID
            ByteBlock ts;                      // TS payload buffer
            std::map <ETID, ETIDContext> tids; // TID analysis contexts
            PacketCounter pusi_pkt_index;      // Index of last PUSI packet in this PID

            // Default constructor:
            PIDContext() :
                continuity(0),
                sync(false),
                ts(),
                tids(),
                pusi_pkt_index(0)
            {
            }

            // Called when packet synchronization is lost on the pid
            void syncLost()
            {
                sync = false;
                ts.clear();
            }
        };

        // Private members:
        TableHandlerInterface*   _table_handler;
        SectionHandlerInterface* _section_handler;
        std::map<PID,PIDContext> _pids;
        Status                   _status;

        // Inacessible operations
        SectionDemux(const SectionDemux&) = delete;
        SectionDemux& operator=(const SectionDemux&) = delete;
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
