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
//
//  Dump transport stream packets.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTSDumpArgs.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DumpPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(DumpPlugin);
    public:
        // Implementation of plugin API
        DumpPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        TSDumpArgs _dump;
        UString    _outname;

        // Working data.
        std::ofstream _outfile;
        std::ostream* _out;
        bool          _add_endline;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"dump", ts::DumpPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DumpPlugin::DumpPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Dump transport stream packets", u"[options]"),
    _dump(),
    _outname(),
    _outfile(),
    _out(&std::cout),
    _add_endline(false)
{
    _dump.defineArgs(*this);

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"Output file for dumped packets. By default, use the standard output.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::DumpPlugin::getOptions()
{
    bool ok = _dump.loadArgs(duck, *this);
    getValue(_outname, u"output-file");

    if (_dump.log && !_outname.empty()) {
        tsp->error(u"--log and --output-file are mutually exclusive");
        ok = false;
    }
    return ok;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::DumpPlugin::start()
{
    if (_outname.empty()) {
        _out = &std::cout;
    }
    else {
        _outfile.open(_outname.toUTF8().c_str());
        if (!_outfile) {
            tsp->error(u"error creating output file %s", {_outname});
            return false;
        }
        _out = &_outfile;
    }
    _add_endline = false;
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::DumpPlugin::stop()
{
    if (_add_endline) {
        (*_out) << std::endl;
    }
    if (_outfile.is_open()) {
        _outfile.close();
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::DumpPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (_dump.pids.test(pkt.getPID())) {
        if (_dump.log) {
            std::ostringstream strm;
            pkt.display(strm, _dump.dump_flags, 0, _dump.log_size);
            UString str;
            str.assignFromUTF8(strm.str());
            str.trim();
            tsp->info(str);
        }
        else {
            (*_out) << std::endl << "* Packet " << ts::UString::Decimal(tsp->pluginPackets()) << std::endl;
            pkt.display(*_out, _dump.dump_flags, 2, _dump.log_size);
            _add_endline = true;
        }
    }
    return TSP_OK;
}
