//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
    //! @ingroup libtsduck mpeg
    //!
    //! On each PID, the continuity counters are automatically updated and synchronized.
    //! It is also possible to force the PID of packets.
    //!
    class TSDUCKDLL TSFileOutputResync: public TSFile
    {
        TS_NOBUILD_NOCOPY(TSFileOutputResync);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit TSFileOutputResync(Report* report);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //!
        explicit TSFileOutputResync(ReporterBase* delegate);

        //!
        //! Destructor.
        //!
        virtual ~TSFileOutputResync() override;

        // Overrides TSFile methods
        virtual bool open(const fs::path& filename, OpenFlags flags, TSPacketFormat format) override;
        virtual bool open(const fs::path& filename, OpenFlags flags) override;

        //!
        //! Write TS packets to the file.
        //! @param [in,out] buffer Address of first packet to write.
        //! The continuity counters of all packets are modified.
        //! @param [in] metadata Optional packet metadata containing time stamps.
        //! If the file format requires time stamps, @a metadata must not be a null
        //! pointer and all packets must have a time stamp.
        //! @param [in] packet_count Number of packets to write.
        //! @return True on success, false on error.
        //!
        bool writePackets(TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count);

        //!
        //! Write TS packets to the file.
        //! @param [in,out] buffer Address of first packet to write.
        //! The continuity counters of all packets are modified.
        //! @param [in] metadata Optional packet metadata containing time stamps.
        //! If the file format requires time stamps, @a metadata must not be a null
        //! pointer and all packets must have a time stamp.
        //! @param [in] packet_count Number of packets to write.
        //! @param [in] pid The PID of all packets is forced to this value.
        //! @return True on success, false on error.
        //!
        bool writePackets(TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count, PID pid);

    private:
        ContinuityAnalyzer _cc_fixer {AllPIDs()};

        // Make openRead() and read-only writePackets() inaccessible.
        bool openRead(const fs::path&, size_t, uint64_t, Report&) = delete;
        bool openRead(const fs::path&, uint64_t, Report&) = delete;
        virtual bool writePackets(const TSPacket* buffer, const TSPacketMetadata* metadata, size_t packet_count) override;
    };
}
