//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to resynchronize the clock of a service from another service.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSignalizationDemux.h"

namespace ts {
    //!
    //! Plugin to resynchronize the clock of a service using the clock of another service.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SVResyncPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SVResyncPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        UString          _target_service {};  // Target service to resync.
        UString          _ref_service {};     // Reference service.
        PID              _ref_pid = PID_NULL; // Reference PID.
        TSPacketLabelSet _set_labels {};      // Labels to set on modified packets

        // Working data:
        PID                _cur_ref_pid = PID_NULL;     // Current reference PID.
        uint64_t           _last_ref_pcr = INVALID_PCR; // Last PCR value in the reference PID.
        PacketCounter      _last_ref_packet = 0;        // Packet index for _last_ref_pcr.
        uint64_t           _delta_pts = 0;              // Value to add in target PTS and DTS (modulo PTS_DTS_SCALE).
        bool               _bitrate_error = false;      // PCR adjustment does not take into account packet distance between ref and target PCR.
        PacketCounter      _pcr_adjust_count = 0;       // Number of adjusted PCR.
        PacketCounter      _pts_adjust_count = 0;       // Number of adjusted PTS.
        PacketCounter      _dts_adjust_count = 0;       // Number of adjusted DTS.
        PID                _target_pcr_pid = PID_NULL;  // Main PCR PID of target service, just to detect change.
        PIDSet             _target_pids {};             // Components of the target service, where to adjust PCR, PTS, DTS.
        PIDSet             _modified_pids {};           // PID's with actually modified packets.
        SignalizationDemux _demux {duck, this};         // Analyze the transport stream.

        // Implementation of SignalizationHandlerInterface
        virtual void handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed) override;
    };
}
