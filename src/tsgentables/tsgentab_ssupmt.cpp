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
//  tsgentab plugin shared library: PMT for System Software Update (SSU)
//
//----------------------------------------------------------------------------

#include "tsGenTabPlugin.h"
#include "tsNames.h"
#include "tsTables.h"
#include "tsStringUtils.h"
#include "tsFormat.h"

#define DEFAULT_OUI OUI_DVB

namespace ts {
    class ThisPlugin: public GenTabPlugin
    {
    public:

        //---------------------------------------------------------------------
        // Constructor
        //---------------------------------------------------------------------

        ThisPlugin() :
            GenTabPlugin("PMT for System Software Update (SSU) service", "[options]")
        {
            option("oui",            'o', INTEGER, 0, UNLIMITED_COUNT, 0, 0x00FFFFFF);
            option("pid-data",       'p', PIDVAL,  1, UNLIMITED_COUNT);
            option("pmt-version",     0,  INTEGER, 0, 1, 0, 31);
            option("selector",        0,  STRING,  0, UNLIMITED_COUNT);
            option("service-id",     's', UINT16,  1, 1);
            option("type-update",    't', INTEGER, 0, UNLIMITED_COUNT, 0, 15);
            option("update-version", 'u', INTEGER, 0, UNLIMITED_COUNT, 0, 31);

            setHelp("Options:\n"
                    "\n"
                    "  --help\n"
                    "      Display this help text.\n"
                    "\n"
                    "  -o value\n"
                    "  --oui value\n"
                    "      Organizationally Unique Identifier (OUI) of SSU provider.\n"
                    "      The default is " + Format ("0x%06X", int (DEFAULT_OUI)) + ", " + names::OUI (DEFAULT_OUI) + ".\n"
                    "      This parameter can be specified more than once if there are several\n"
                    "      SSU data streams in the service. If there are more --pid-data than\n"
                    "      --oui, the last OUI is used for subsequent data streams.\n"
                    "\n"
                    "  -p value\n"
                    "  --pid-data value\n"
                    "      Specifies the PID for the SSU data stream. There is no default,\n"
                    "      this is a mandatory parameter. It can be specified more than once\n"
                    "      if there are more than one SSU data streams in the service.\n"
                    "\n"
                    "  --pmt-version value\n"
                    "      Specifies the version of the PMT section. The default is zero.\n"
                    "\n"
                    "  --selector \"hexa-string\"\n"
                    "      Specifies the selector bytes for the OUI using a string of\n"
                    "      hexadecimal characters. This parameter can be specified more than\n"
                    "      once if there are several SSU data streams in the service. If there\n"
                    "      are more --pid-data than --selector, the last selector is used for\n"
                    "      subsequent data streams.\n"
                    "\n"
                    "  -s value\n"
                    "  --service-id value\n"
                    "      Specifies the service_id for the SSU service. There is no\n"
                    "      default, this is a mandatory parameter.\n"
                    "\n"
                    "  -t value\n"
                    "  --type-update value\n"
                    "      Specifies the update_type in the system_software_update_info\n"
                    "      structure as defined in ETSI TS 102 006. The default is 0x01,\n"
                    "      ie. standard update carousel (no notification) via broadcast.\n"
                    "      This parameter can be specified more than once if there are several\n"
                    "      SSU data streams in the service. If there are more --pid-data than\n"
                    "      --type-update, the last update_type is used for subsequent data streams.\n"
                    "\n"
                    "  -u value\n"
                    "  --update-version value\n"
                    "      Specifies the update_version in the system_software_update_info\n"
                    "      structure as defined in ETSI TS 102 006. By default, there is no\n"
                    "      update_version (this is an optional field). This parameter can be\n"
                    "      specified more than once if there are several SSU data streams in\n"
                    "      the service. If there are more --pid-data than --update-version, the\n"
                    "      subsequent data streams have no update_version.\n"
                    "\n"
                    "  --version\n"
                    "      Display the version number.\n");
        }

        //---------------------------------------------------------------------
        // Table Generation
        //---------------------------------------------------------------------

        virtual void generate(AbstractTablePtr& table)
        {
            // Decode options

            const uint8_t version = intValue<uint8_t>("pmt-version", 0);
            const uint16_t service_id = intValue<uint16_t>("service-id");
            StringVector selector;
            std::vector<PID> pid_data;
            std::vector<uint8_t> update_version;
            std::vector<uint32_t> oui;
            std::vector<uint8_t> update_type;

            getValues(selector, "selector");
            getIntValues(pid_data, "pid-data");
            getIntValues(update_version, "update-version");
            getIntValues(oui, "oui");
            if (oui.empty()) {
                oui.push_back(DEFAULT_OUI); // default value at least once
            }
            getIntValues(update_type, "type-update");
            if (update_type.empty()) {
                update_type.push_back(0x01); // default value at least once
            }

            // Create table

            PMT* pmt = new PMT();
            table = pmt;

            // PMT fixed fields

            pmt->version = version;
            pmt->is_current = true;
            pmt->service_id = service_id;
            pmt->pcr_pid = PID_NULL;

            // Elementary streams

            for (size_t i = 0; i < pid_data.size(); ++i) {

                // Locate/create corresponding elementary stream entry in PMT

                PMT::Stream& stream (pmt->streams[pid_data[i]]);
                stream.stream_type = ST_DSMCC_UN; // 0x0B, DSM-CC User-to-Network messages

                // Create data_broadcast_id_descriptor

                SSUDataBroadcastIdDescriptor desc;
                SSUDataBroadcastIdDescriptor::Entry ent;
                ent.oui = i < oui.size() ? oui[i] : oui.back();
                ent.update_type = i < update_type.size() ? update_type[i] : update_type.back();
                if (i < update_version.size()) {
                    ent.update_version = update_version[i];
                }
                const std::string sel (i < selector.size() ? selector[i] : selector.empty() ? "" : selector.back());
                if (!sel.empty() && !HexaDecode (ent.selector, sel)) {
                    error ("invalid hexadecimal string \"" + sel + "\"");
                    table.clear();
                    return;
                }

                // Add data_broadcast_id_descriptor in stream descriptor

                desc.entries.push_back (ent);
                stream.descs.add (desc);
            }
        }
    };
}

TSGENTAB_DECLARE_PLUGIN(ts::ThisPlugin)
