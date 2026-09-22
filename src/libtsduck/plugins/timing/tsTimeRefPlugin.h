//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to modify the time reference of a TS (update TDT and TOT).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsEITProcessor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Plugin to modify the time reference of a TS (update TDT and TOT).
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL TimeRefPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(TimeRefPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool              _update_tdt = false;      // Update the TDT
        bool              _update_tot = false;      // Update the TOT
        bool              _update_eit = false;      // Update the EIT's
        bool              _eit_date_only = false;   // Update date field only in EIT
        bool              _use_timeref = false;     // Use a new time reference
        bool              _system_sync = false;     // Synchronous with system clock.
        bool              _update_local = false;    // Update local time info, not only UTC
        cn::milliseconds  _add_milliseconds {};     // Add this to all time values
        Time              _startref {};             // Starting value of new time reference
        int               _local_offset = INT_MAX;  // Local time offset in minutes (INT_MAX if unspecified)
        int               _next_offset = INT_MAX;   // Next time offset after DST change, in minutes (INT_MAX if unspecified)
        Time              _next_change {};          // Next DST time
        std::set<UString> _only_countries {};       // Countries for TOT local time modification
        std::set<int>     _only_regions {};         // Regions for TOT local time modification

        // Processing data:
        Time              _timeref {};              // Current value of new time reference
        PacketCounter     _timeref_pkt = 0;         // Packet number for _timeref
        EITProcessor      _eit_processor {duck};    // Modify EIT's
        bool              _eit_active = false;      // Update EIT's now (disabled during init phase with --start)

        // Process a TDT or TOT section.
        void processSection(uint8_t* section, size_t size);

        // Process a local_time_offset_descriptor.
        void processLocalTime(uint8_t* desc, size_t size);
    };
}
