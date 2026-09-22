//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to schedule packets pass or drop, based on packet numbers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSClock.h"
#include "tsTSValve.h"
#include "tsNames.h"

namespace ts {
    //!
    //! Plugin to schedule packets pass or drop, based on packet numbers.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SlicePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(SlicePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Event description
        struct SliceEvent
        {
            // Public fields
            PacketProcessStatus status;   // Packet status to return ...
            uint64_t            value;    // ... after this packet number or milli-seconds

            // Constructor
            SliceEvent(const PacketProcessStatus& s, const uint64_t& v) : status(s), value(v) {}

            // Comparison, for sort algorithm
            bool operator<(const SliceEvent& e) const { return value < e.value; }
        };
        using SliceEventVector = std::vector<SliceEvent>;

        // Command line options:
        bool                _use_time = false;      // Use milliseconds in SliceEvent::value
        bool                _ignore_pcr = false;    // Do not use PCR's, rely on previous plugins' bitrate
        PacketProcessStatus _first_status = TSP_OK; // Current packet status to return
        uint64_t            _time_factor = 0;       // Factor to apply to get milli-seconds
        SliceEventVector    _events {};             // Sorted list of events to apply
        TSClockArgs         _clock_args {};         // Command-line arguments for _clock (fixed option --pcr-based).
        TSValveArgs         _valve_args {};         // Command-line arguments for _valve.

        // Working data:
        size_t              _next_index = 0;        // Index of next SliceEvent to apply
        TSClock             _clock {duck};          // Transport stream clock.
        TSValve             _valve {*this};         // Open/close packet flow.
        const Names&        _names {PacketProcessingStatusNames()};

        // Add event in the list from one option.
        void addEvents(const UChar* option, PacketProcessStatus status);
    };
}
