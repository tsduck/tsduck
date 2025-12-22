//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for plugins which process one single MPE PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsServiceDiscovery.h"
#include "tsMPEDemux.h"

namespace ts {
    //!
    //! Abstract base class for plugins which process one single Multi-Protocol Encapsulation (MPE) PID.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL AbstractSingleMPEPlugin : public ProcessorPlugin, private MPEHandlerInterface
    {
        TS_NOBUILD_NOCOPY(AbstractSingleMPEPlugin);
    public:
        // Implementation of ProcessorPlugin interface.
        // If overridden by subclass, superclass must be explicitly invoked.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

        //!
        //! Handle one MPE packet (to be implemented by subclasses).
        //! @param [in] timestamp Timestamp in PCR unit of the last TS packet for this MPE packet.
        //! @param [in] mpe MPE packet.
        //!
        virtual void handleSingleMPEPacket(PCR timestamp, const MPEPacket& mpe) = 0;

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] tsp Object to communicate with the Transport Stream Processor main executable.
        //! @param [in] description A short one-line description, eg. "DVB-NIP analyzer".
        //! @param [in] syntax A short one-line syntax summary.
        //! @param [in] stream_name A short description of the content of the MPE PID, eg. "DVB-NIP stream".
        //!
        AbstractSingleMPEPlugin(TSP* tsp, const UString& description, const UString& syntax, const UString& stream_name);

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

    private:
        // Command line options.
        PID              _opt_pid = PID_NULL;
        UString          _opt_service {};

        // Plugin private fields.
        bool             _abort = false;             // Error, abort asap.
        bool             _wait_for_service = false;  // Wait for MPE service id to be identified.
        PID              _mpe_pid = PID_NULL;        // Actual MPE PID.
        PCR              _last_timestamp {};         // Last valid timestamp from TS packets.
        ServiceDiscovery _service {duck, nullptr};   // Service containing the MPE PID.
        MPEDemux         _mpe_demux {duck, this};    // MPE demux to extract MPE datagrams.

        // Inherited methods.
        virtual void handleMPENewPID(MPEDemux&, const PMT&, PID) override;
        virtual void handleMPEPacket(MPEDemux&, const MPEPacket&) override;
    };
}
