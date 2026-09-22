//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to identify PID's based on various criteria.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSignalizationDemux.h"

namespace ts {
    //!
    //! Plugin to identify PID's based on various criteria.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL IdentifyPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(IdentifyPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool              _log = false;
        bool              _pmt = false;
        bool              _audio = false;
        bool              _video = false;
        bool              _subtitles = false;
        bool              _scte35 = false;
        bool              _all_service_components = false;
        UString           _service_name {};
        UString           _language {};
        UString           _env_variable {};
        std::set<uint8_t> _stream_types {};
        std::set<REGID>   _registrations {};
        TSPacketLabelSet  _set_labels {};
        TSPacketLabelSet  _all_set_labels {};

        // Working data:
        uint16_t           _service_id = INVALID_SERVICE_ID;
        PIDSet             _identified_pids {};
        SignalizationDemux _sig_demux {duck, this};

        // Implementation of interfaces.
        virtual void handlePAT(const PAT&, PID) override;
        virtual void handlePMT(const PMT&, PID) override;
        virtual void handleService(uint16_t ts_id, const Service&, const PMT&, bool removed) override;

        // Identify a PID, return true if new.
        bool identifyPID(PID);

        // Identify a new PID with formatted string message.
        template <class... Args>
        void identifyPID(PID pid, const UChar* format, Args&&... args)
        {
            if (identifyPID(pid) && _log) {
                UString fmt;
                fmt.format(u"PID %n: %s", pid, format);
                info(fmt, std::forward<ArgMixIn>(args)...);
            }
        }
    };
}
