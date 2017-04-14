//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  tsgentab plugin shared library: create a PAT
//
//----------------------------------------------------------------------------

#include "tsGenTabPlugin.h"
#include "tsTables.h"
#include "tsStringUtils.h"

namespace ts {
    class ThisPlugin: public GenTabPlugin
    {
    public:

        //---------------------------------------------------------------------
        // Constructor
        //---------------------------------------------------------------------

        ThisPlugin() :
            GenTabPlugin("Generic PAT", "[options] [sid/pid ...]")
        {
            option("",             0,  STRING);
            option("nit",         'n', PIDVAL);
            option("ts-id",       't', UINT16);
            option("pat-version", 'v', INTEGER, 0, 1, 0, 31);

            setHelp("Parameters (sid/pid):\n"
                    "\n"
                    "  Add the specified service_id / PMT-PID in the PAT. Several sid/pid pairs\n"
                    "  may be specified to add several services.\n"
                    "\n"
                    "Options:\n"
                    "\n"
                    "  --help\n"
                    "      Display this help text.\n"
                    "\n"
                    "  -n pid\n"
                    "  --nit pid\n"
                    "      Add the specified NIT PID in the PAT.\n"
                    "\n"
                    "  -t id\n"
                    "  --ts-id id\n"
                    "      Specify the transport stream id in the PAT. The default is zero.\n"
                    "\n"
                    "  -v value\n"
                    "  --pat-version value\n"
                    "      Specifies the version of the PAT section. The default is zero.\n"
                    "\n"
                    "  --version\n"
                    "      Display the version number.\n");
        }

        //---------------------------------------------------------------------
        // Table Generation
        //---------------------------------------------------------------------

        virtual void generate(AbstractTablePtr& table)
        {
            // Create table

            PAT* pat = new PAT();
            table = pat;

            // PAT fixed fields

            pat->version = intValue<uint8_t> ("pat-version", 0);
            pat->is_current = true;
            pat->ts_id = intValue<uint16_t> ("ts-id", 0);
            pat->nit_pid = intValue<PID> ("nit", PID_NULL);

            // List of services

            const size_t service_count = count ("");
            std::string sidpid;

            for (size_t n = 0; n < service_count; n++) {
                getValue (sidpid, "", "", n);
                int sid, pid;
                char unused;
                if (::sscanf (sidpid.c_str(), "%i/%i%c", &sid, &pid, &unused) != 2 || sid < 0 || sid > 0xFFFF || pid < 0 || pid >= PID_MAX) {
                    error ("invalid \"service_id/PID\" value \"" + sidpid + "\"");
                    table.clear();
                }
                else {
                    pat->pmts.insert (std::make_pair (uint16_t (sid), PID (pid)));
                }
            }
        }
    };
}

TSGENTAB_DECLARE_PLUGIN (ts::ThisPlugin);
