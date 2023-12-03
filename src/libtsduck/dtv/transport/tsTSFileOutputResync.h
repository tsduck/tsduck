//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A specialized form of transport stream output file with resynchronized
//!  PID and continuity counters.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSFile.h"
#include "tsContinuityAnalyzer.h"

namespace ts {
    //!
    //! A specialized form of transport stream output file with resynchronized PID and continuity counters.
    //! @ingroup mpeg
    //!
    //! On each PID, the continuity counters are automatically updated and synchronized.
    //! It is also possible to force the PID of packets.
    //!
    class TSDUCKDLL TSFileOutputResync: public TSFile
    {
        TS_NOCOPY(TSFileOutputResync);
    public:
        //!
        //! Default constructor.
        //!
        TSFileOutputResync();

        //!
        //! Destructor.
        //!
        virtual ~TSFileOutputResync() override;

        // Overrides TSFile methods
        virtual bool open(const fs::path& filename, OpenFlags flags, Report& report, TSPacketFormat format = TSPacketFormat::AUTODETECT) override;

        //!
        //! Write TS packets to the file.
        //! @param [in,out] buffer Address of first packet to write.
        //! The continuity counters of all packets are modified.
        //! @param [in] packet_count Number of packets to write.
        //! @param [in,out] report Where to report errors.
        //! @param [in] metadata Optional packet metadata containing time stamps.
        //! If the file format requires time stamps, @a metadata must not be a null
        //! pointer and all packets must have a time stamp.
        //! @return True on success, false on error.
        //!
        bool writePackets(TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count, Report& report);

        //!
        //! Write TS packets to the file.
        //! @param [in,out] buffer Address of first packet to write.
        //! The continuity counters of all packets are modified.
        //! @param [in] packet_count Number of packets to write.
        //! @param [in] pid The PID of all packets is forced to this value.
        //! @param [in,out] report Where to report errors.
        //! @param [in] metadata Optional packet metadata containing time stamps.
        //! If the file format requires time stamps, @a metadata must not be a null
        //! pointer and all packets must have a time stamp.
        //! @return True on success, false on error.
        //!
        bool writePackets(TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count, PID pid, Report& report);

    private:
        ContinuityAnalyzer _ccFixer {AllPIDs};

        // Make openRead() and read-only writePackets() inaccessible.
        bool openRead(const fs::path&, size_t, uint64_t, Report&) = delete;
        bool openRead(const fs::path&, uint64_t, Report&) = delete;
        virtual bool writePackets(const TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count, Report& report) override;
    };
}
