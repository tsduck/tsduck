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
//
//  Transport stream processor shared library:
//  Generic / sample / reference descrambler.
//  Can be used as a template for real conditional access systems.
//
//----------------------------------------------------------------------------

#include "tsAbstractDescrambler.h"
#include "tsPluginRepository.h"
#include "tsDuckProtocol.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DescramblerPlugin: public AbstractDescrambler
    {
    public:
        DescramblerPlugin(TSP*);

    protected:
        // Implementation of AbstractDescrambler.
        virtual bool checkCADescriptor(uint16_t cas_id, const ByteBlock& priv) override;
        virtual bool checkECM(const Section& ecm) override;
        virtual bool decipherECM(const Section& ecm, ByteBlock& cw_even, ByteBlock& cw_odd) override;

    private:
        // Inaccessible operations
        DescramblerPlugin() = delete;
        DescramblerPlugin(const DescramblerPlugin&) = delete;
        DescramblerPlugin& operator=(const DescramblerPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(descrambler, ts::DescramblerPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DescramblerPlugin::DescramblerPlugin(TSP* tsp_) :
    AbstractDescrambler(tsp_, u"Generic DVB descrambler.")
{
    setHelp(u"This plugin descrambles fixed PID's with fixed control words. As a demo, it can\n"
            u"also descramble services for which clear ECM's were generated using the utility\n"
            u"named tsecmg, a DVB SimulCrypt-compliant ECMG for test and demo.\n"
            u"\n" +
            getHelp());
}



//----------------------------------------------------------------------------
// Check a CA_descriptor from a PMT.
//----------------------------------------------------------------------------

bool ts::DescramblerPlugin::checkCADescriptor(uint16_t cas_id, const ByteBlock& priv)
{
    // In this demo descrambler, we accept all CAS id.
    return true;
}


//----------------------------------------------------------------------------
// Check if the descrambler may decipher an ECM.
//----------------------------------------------------------------------------

bool ts::DescramblerPlugin::checkECM(const Section& ecm)
{
    // In this demo descrambler, we do not add any further check.
    return true;
}


//----------------------------------------------------------------------------
// Decipher an ECM.
//----------------------------------------------------------------------------

bool ts::DescramblerPlugin::decipherECM(const Section& ecm, ByteBlock& cw_even, ByteBlock& cw_odd)
{
    // Clear returned ECM's.
    cw_even.clear();
    cw_odd.clear();

    // The ECM content is the section payload.
    const uint8_t* const ecmData = ecm.payload();
    const size_t ecmSize = ecm.payloadSize();

    // Analyze ECM as a TLV message.
    tlv::MessageFactory mf(ecmData, ecmSize, duck::Protocol::Instance());
    tlv::MessagePtr msg;
    if (mf.errorStatus() == tlv::OK) {
        mf.factory(msg);
    }

    // If this a valid ECM?
    SafePtr<duck::ClearECM> clearECM(msg.downcast<duck::ClearECM>());
    if (clearECM.isNull()) {
        // Not a valid ECM as generated by tsecmg.
        const size_t dumpSize = std::min<size_t>(ecmSize, 16);
        tsp->error(u"received invalid ECM (%d bytes): %s%s", {ecmSize, UString::Dump(ecmData, dumpSize, UString::SINGLE_LINE), dumpSize < ecmSize ? u" ..." : u""});
        return false;
    }
    else {
        cw_even = clearECM->cw_even;
        cw_odd = clearECM->cw_odd;
        return true;
    }
}
