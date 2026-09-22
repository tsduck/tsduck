//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to schedule packets pass or drop, based on time.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"
#include "tsTSValve.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Plugin to schedule packets pass or drop, based on time.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL TimePlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(TimePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Time event description
        struct TimeEvent
        {
            // Public fields
            PacketProcessStatus status;   // Packet status to return ...
            Time                time;     // ... after this UTC time

            // Constructor
            TimeEvent(const PacketProcessStatus& s, const Time& t) : status (s), time (t) {}

            // Comparison, for sort algorithm
            bool operator<(const TimeEvent& t) const { return time < t.time; }
        };
        using TimeEventVector = std::vector<TimeEvent>;

        // Command line options:
        bool                _relative = false;      // Use relative time from the beginning
        bool                _use_utc = false;       // Use UTC time
        bool                _use_tdt = false;       // Use TDT as time reference
        PacketProcessStatus _first_status = TSP_OK; // Current packet status to return
        TimeEventVector     _events {};             // Sorted list of time events to apply
        TSValveArgs         _valve_args {};         // Command-line arguments for _valve.

        // Working data:
        Time                _last_time {};          // Last measured time
        SectionDemux        _demux {duck, this};    // Section filter
        size_t              _next_index = 0;        // Index of next TimeEvent to apply
        TSValve             _valve {*this};         // Open/close packet flow.
        const Names&        _names {PacketProcessingStatusNames()};

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Get current time.
        Time currentTime() const { return _use_utc ? Time::CurrentUTC() : Time::CurrentLocalTime(); }

        // Add time events in the list for one option. Return false if a time string is invalid
        bool addEvents(const UChar* option, PacketProcessStatus status);
    };
}
