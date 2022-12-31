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

#include "tsECMGClientArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::ECMGClientArgs::ECMGClientArgs() :
    ecmg_address(),
    super_cas_id(0),
    access_criteria(),
    cp_duration(0),
    dvbsim_version(0),
    ecm_channel_id(0),
    ecm_stream_id(0),
    ecm_id(0),
    log_protocol(0),
    log_data(0)
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::ECMGClientArgs::defineArgs(Args& args)
{
    args.option(u"access-criteria", 'a', Args::HEXADATA);
    args.help(u"access-criteria",
              u"Specifies the access criteria for the service as sent to the ECMG. "
              u"The value must be a suite of hexadecimal digits.");

    args.option(u"channel-id", 0, Args::UINT16);
    args.help(u"channel-id", u"Specifies the DVB SimulCrypt ECM_channel_id for the ECMG (default: 1).");

    args.option(u"cp-duration", 'd', Args::POSITIVE);
    args.help(u"cp-duration", u"seconds", u"Specifies the crypto-period duration in seconds (default: 10).");

    args.option(u"ecm-id", 'i', Args::UINT16);
    args.help(u"ecm-id", u"Specifies the DVB SimulCrypt ECM_id for the ECMG (default: 1).");

    args.option(u"ecmg", 'e', Args::STRING);
    args.help(u"ecmg", u"host:port", u"Specify an ECM Generator host name and port.");

    args.option(u"ecmg-scs-version", 'v', Args::INTEGER, 0, 1, 2, 3);
    args.help(u"ecmg-scs-version",
         u"Specifies the version of the ECMG <=> SCS DVB SimulCrypt protocol. "
         u"Valid values are 2 and 3. The default is 2.");

    args.option(u"log-data", 0, ts::Severity::Enums, 0, 1, true);
    args.help(u"log-data", u"level",
         u"Same as --log-protocol but applies to CW_provision and ECM_response "
         u"messages only. To debug the session management without being flooded by "
         u"data messages, use --log-protocol=info --log-data=debug.");

    args.option(u"log-protocol", 0, ts::Severity::Enums, 0, 1, true);
    args.help(u"log-protocol", u"level",
         u"Log all ECMG <=> SCS protocol messages using the specified level. If the "
         u"option is not present, the messages are logged at debug level only. If the "
         u"option is present without value, the messages are logged at info level. "
         u"A level can be a numerical debug level or a name.");

    args.option(u"stream-id", 0, Args::UINT16);
    args.help(u"stream-id", u"Specifies the DVB SimulCrypt ECM_stream_id for the ECMG (default: 1).");

    args.option(u"super-cas-id", 's', Args::UINT32);
    args.help(u"super-cas-id", u"Specify the DVB SimulCrypt Super_CAS_Id. This is required when --ecmg is specified.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::ECMGClientArgs::loadArgs(DuckContext& duck, Args& args)
{
    args.getIntValue(super_cas_id, u"super-cas-id");
    args.getIntValue(ecm_channel_id, u"channel-id", 1);
    args.getIntValue(ecm_stream_id, u"stream-id", 1);
    args.getIntValue(ecm_id, u"ecm-id", 1);
    cp_duration = MilliSecPerSec * args.intValue<MilliSecond>(u"cp-duration", 10);
    log_protocol = args.present(u"log-protocol") ? args.intValue<int>(u"log-protocol", ts::Severity::Info) : ts::Severity::Debug;
    log_data = args.present(u"log-data") ? args.intValue<int>(u"log-data", ts::Severity::Info) : log_protocol;
    args.getIntValue(dvbsim_version, u"ecmg-scs-version", 2);
    args.getHexaValue(access_criteria, u"access-criteria");

    // Decode ECMG socket address.
    const UString ecmg(args.value(u"ecmg"));
    if (ecmg.empty()) {
        ecmg_address.clear();
    }
    else if (!ecmg_address.resolve(ecmg, args)) {
        // Invalid host:port, error message already reported
        return false;
    }
    else if (!ecmg_address.hasAddress() || !ecmg_address.hasPort()) {
        args.error(u"missing ECMG address or port");
    }

    return true;
}
