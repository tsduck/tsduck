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
//!  Digital TV tuner emulator.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerBase.h"
#include "tsModulationArgs.h"
#include "tsAbortInterface.h"
#include "tsTSFile.h"
#include "tsTSForkPipe.h"

namespace ts {
    //!
    //! Digital TV tuner emulator.
    //! @ingroup hardware
    //!
    //! A tuner emulator implements the TunerBase interface without physical tuner.
    //! The "device name" is the name of an XML file which describes the tuner configuration.
    //!
    //! A tuner configuration contains a list of frequencies. The "tuner" can tune around each
    //! of these frequencies (center frequency with a given bandwidth). Each frequency is
    //! associated with a TS file. When the emulated tuner is tuned to a valid frequency, the
    //! reception is emulated by reading packets from the associated TS file again and again.
    //!
    //! Sample XML tuner emulator configuration:
    //! @code
    //! <?xml version="1.0" encoding="UTF-8"?>
    //! <tsduck>
    //!   <defaults delivery="DVB-T" bandwidth="4,000,000" directory="/home/user/streams"/>
    //!   <channel frequency="474,000,000" file="mux1.ts"/>
    //!   <channel frequency="482,000,000" file="mux2.ts"/>
    //!   <channel frequency="490,000,000" file="mux3.ts"/>
    //!   <channel frequency="498,000,000" file="mux4.ts"/>
    //!   <channel frequency="506,000,000" file="mux5.ts"/>
    //!   <channel frequency="514,000,000" file="mux6.ts"/>
    //!   <channel frequency="522,000,000" file="mux7.ts"/>
    //!   <channel frequency="530,000,000" file="mux8.ts" delivery="DVB-T2" bandwidth="6,000,000"/>
    //! </tsduck>
    //! @endcode
    //!
    //! Sample tuning test using this tuner emulator. The various TS files are read as if they were actual muxes:
    //! @code
    //! tsscan -d etuner.xml --uhf-band --first-channel 21 --last-channel 28 --service-list
    //! @endcode
    //!
    class TSDUCKDLL TunerEmulator: public TunerBase
    {
        TS_NOBUILD_NOCOPY(TunerEmulator);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //!
        TunerEmulator(DuckContext& duck);

        //!
        //! Destructor.
        //!
        virtual ~TunerEmulator() override;

        // Implementation of TunerBase.
        // Some methods are not implemented, left to the default method in base class.
        virtual bool open(const UString& device_name, bool info_only) override;
        virtual bool close(bool silent = false) override;
        virtual bool isOpen() const override;
        virtual bool infoOnly() const override;
        virtual const DeliverySystemSet& deliverySystems() const override;
        virtual UString deviceName() const override;
        virtual UString deviceInfo() const override;
        virtual UString devicePath() const override;
        virtual bool getSignalState(SignalState& state) override;
        virtual bool tune(ModulationArgs& params) override;
        virtual bool start() override;
        virtual bool stop(bool silent = false) override;
        virtual size_t receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort = nullptr) override;
        virtual bool getCurrentTuning(ModulationArgs& params, bool reset_unknown) override;
        virtual std::ostream& displayStatus(std::ostream& strm, const UString& margin = UString(), bool extended = false) override;

    private:
        // Possible states of the tuner emulator.
        enum class State {CLOSED, OPEN, TUNED, STARTED};

        // Description of a channel.
        class Channel
        {
        public:
            uint64_t       frequency;   // Center frequency in Hz.
            uint64_t       bandwidth;   // Bandwidth in Hz, over which reception is possible.
            DeliverySystem delivery;    // Delivery system for this frequency.
            UString        file;        // TS file name.
            UString        pipe;        // Command line to pipe output in terminal emulator.

            // Constructor.
            Channel();

            // Compute the distance of a frequency from the center one.
            uint64_t distance(uint64_t frequency) const;

            // Check if a frequency is in the channel.
            bool inBand(uint64_t frequency) const;

            // Compute the virtual signal strength for a given frequency.
            int strength(uint64_t frequency) const;
        };

        // Tuner emulator private members.
        DeliverySystemSet    _delivery_systems;  // Collection of all delivery systems.
        UString              _xml_file_path;     // Main XML file path.
        bool                 _info_only;         // Open mode, useless here, just informational.
        State                _state;             // Current state.
        TSFile               _file;              // Current TS file.
        TSForkPipe           _pipe;              // Current pipe process.
        std::vector<Channel> _channels;          // Map of channels.
        size_t               _tune_index;        // Currently tuned channel.
        uint64_t             _tune_frequency;    // Requested frequency.
        int                  _strength;          // Signal strength in percent.
    };
}
