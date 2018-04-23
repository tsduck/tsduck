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
//  Minimal generic DVB SimulCrypt compliant EMMG for CAS head-end integration.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsIntegerUtils.h"
#include "tsIPUtils.h"
#include "tsEMMGClient.h"
#include "tsPacketizer.h"
#include "tsMonotonic.h"
#include "tsVersionInfo.h"
TSDUCK_SOURCE;

namespace {
    // Command line default arguments.
    static const uint16_t DEFAULT_BANDWIDTH      = 100;
    static const size_t   DEFAULT_EMM_SIZE       = 100;
    static const ts::TID  DEFAULT_EMM_MIN_TID    = ts::TID_EMM_FIRST;
    static const ts::TID  DEFAULT_EMM_MAX_TID    = ts::TID_EMM_LAST;
    static const size_t   DEFAULT_BYTES_PER_SEND = 2000;

    // Minimum interval between two send operations.
    static const ts::NanoSecond MIN_SEND_INTERVAL = 4 * ts::NanoSecPerMilliSec; // 4 ms

    // Values for --type option.
    const ts::Enumeration DataTypeEnum({
        {u"emm",          0},
        {u"private-data", 1},
        {u"ecm",          2},
    });
}


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

class EMMGOptions: public ts::Args
{
public:
    EMMGOptions(int argc, char *argv[]);

    ts::SocketAddress muxAddress;          // TCP server address for MUX.
    uint32_t          clientId;            // Client id, see EMMG/PDG <=> MUX protocol.
    uint16_t          channelId;           // Data_channel_id, see EMMG/PDG <=> MUX protocol.
    uint16_t          streamId;            // Data_stream_id, see EMMG/PDG <=> MUX protocol.
    uint16_t          dataId;              // Data_id, see EMMG/PDG <=> MUX protocol.
    uint8_t           dataType;            // Data_type, see EMMG/PDG <=> MUX protocol.
    bool              sectionMode;         // If true, send data in section format.
    uint16_t          sendBandwidth;       // Bandwidth of sent data in kb/s.
    uint16_t          requestedBandwidth;  // Requested bandwidth in kb/s.
    bool              ignoreAllocatedBW;   // Ignore the returned allocated bandwidth.
    size_t            emmSize;             // Size in bytes of generated EMM's.
    ts::TID           emmMinTableId;       // Minimum table id of generated EMM's.
    ts::TID           emmMaxTableId;       // Maximum table id of generated EMM's.
    uint64_t          maxBytes;            // Stop after injecting that number of bytes.
    ts::BitRate       dataBitrate;         // Actual data bitrate.
    size_t            bytesPerSend;        // Approximate size of each send.
    ts::NanoSecond    sendInterval;        // Interval between two send operations.

    // Adjust the various rates and delays according to the allocated bandwidth.
    bool adjustBandwidth(uint16_t allocated);
};

// Constructor.
EMMGOptions::EMMGOptions(int argc, char *argv[]) :
    ts::Args(u"Minimal generic DVB SimulCrypt-compliant EMMG.", u"[options]"),
    muxAddress(),
    clientId(0),
    channelId(0),
    streamId(0),
    dataId(0),
    dataType(0),
    sectionMode(false),
    sendBandwidth(0),
    requestedBandwidth(0),
    ignoreAllocatedBW(false),
    emmSize(0),
    emmMinTableId(0),
    emmMaxTableId(0),
    maxBytes(0),
    dataBitrate(0),
    bytesPerSend(0),
    sendInterval(0)
{
    option(u"bandwidth",          'b', INTEGER, 0, 1, 1, 0xFFFF);
    option(u"bytes-per-send",      0,  INTEGER, 0, 1, 0x20, 0xEFFF);
    option(u"channel-id",          0,  INT16);
    option(u"client-id",          'c', INT32);
    option(u"data-id",            'd', INT16);
    option(u"emm-size",            0,  INTEGER, 0, 1, ts::MIN_SHORT_SECTION_SIZE, ts::MAX_PRIVATE_SECTION_SIZE);
    option(u"emm-min-table-id",    0,  UINT8);
    option(u"emm-max-table-id",    0,  UINT8);
    option(u"emmg-mux-version",    0,  INTEGER, 0, 1, 1, 5);
    option(u"ignore-allocated",   'i');
    option(u"max-bytes",           0,  UNSIGNED);
    option(u"mux",                'm', STRING);
    option(u"requested-bandwidth", 0,  INT16);
    option(u"section-mode",       's');
    option(u"stream-id",           0,  INT16);
    option(u"type",               't', DataTypeEnum);

    setHelp(u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --bandwidth value\n"
            u"      Specify the bandwidth of the data which are sent to the MUX in kilobits\n"
            u"      per second. Default: " + ts::UString::Decimal(DEFAULT_BANDWIDTH) + u" kb/s.\n"
            u"\n"
            u"  --bytes-per-send value\n"
            u"      Specify the average size in bytes of each data provision. The exact value\n"
            u"      depends on sections and packets sizes. Default: " + ts::UString::Decimal(DEFAULT_BYTES_PER_SEND) + u" bytes.\n"
            u"\n"
            u"  --channel-id value\n"
            u"      This option sets the DVB SimulCrypt parameter 'data_channel_id'.\n"
            u"      Default: 1.\n"
            u"\n"
            u"  -c value\n"
            u"  --client-id value\n"
            u"      This option sets the DVB SimulCrypt parameter 'client_id'. Default: 0.\n"
            u"      For EMM injection, the most signification 16 bits shall be the\n"
            u"      'CA_system_id' of the corresponding CAS.\n"
            u"\n"
            u"  -d value\n"
            u"  --data-id value\n"
            u"      This option sets the DVB SimulCrypt parameter 'data_id'. Default: 0.\n"
            u"\n"
            u"  --emm-max-table-id value\n"
            u"      Specify the maximum table id of the automatically generated fake EMM's.\n"
            u"      When generating fake EMM's, the table ids are cycled from the minimum to\n"
            u"      the maximum value. The default is " + ts::UString::Hexa(DEFAULT_EMM_MAX_TID) + u".\n"
            u"\n"
            u"  --emm-min-table-id value\n"
            u"      Specify the minimum table id of the automatically generated fake EMM's.\n"
            u"      The default is " + ts::UString::Hexa(DEFAULT_EMM_MIN_TID) + u".\n"
            u"\n"
            u"  --emm-size value\n"
            u"      Specify the size in bytes of the automatically generated fake EMM's.\n"
            u"      The default is " + ts::UString::Decimal(DEFAULT_EMM_SIZE) + u" bytes.\n"
            u"\n"
            u"  --emmg-mux-version value\n"
            u"      Specify the version of the EMMG/PDG <=> MUX DVB SimulCrypt protocol.\n"
            u"      Valid values are 1 to 5. The default is 2.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --ignore-allocated\n"
            u"      Ignore the allocated bandwidth as returned by the MUX, continue to send\n"
            u"      data at the planned bandwidth, even if it is higher than the allocated\n"
            u"      bandwidth.\n"
            u"\n"
            u"  --max-bytes value\n"
            u"      Stop after sending the specified number of bytes. By default, send data\n"
            u"      indefinitely.\n"
            u"\n"
            u"  -m address:port\n"
            u"  --mux address:port\n"
            u"      Specify the IP address (or host name) and TCP port of the MUX. This is a\n"
            u"      required parameter, there is no default.\n"
            u"\n"
            u"  --requested-bandwidth value\n"
            u"      This option sets the DVB SimulCrypt parameter 'bandwidth' in the\n"
            u"      'stream_BW_request' message. The value is in kilobits per second. The\n"
            u"      default is the value of the --bandwidth option. Specifying distinct values\n"
            u"      for --bandwidth and --requested-bandwidth can be used for testing the\n"
            u"      behavior of a MUX.\n"
            u"\n"
            u"  -s\n"
            u"  --section-mode\n"
            u"      Send EMM's or data in section format. This option sets the DVB SimulCrypt\n"
            u"      parameter 'section_TSpkt_flag' to zero. By default, EMM's and data are\n"
            u"      sent in TS packet format.\n"
            u"\n"
            u"  --stream-id value\n"
            u"      This option sets the DVB SimulCrypt parameter 'data_stream_id'.\n"
            u"      Default: 1.\n"
            u"\n"
            u"  -t value\n"
            u"  --type value\n"
            u"      This option sets the DVB SimulCrypt parameter 'data_type'. Default: 0\n"
            u"      (EMM). In addition to integer values, the following names can be used:\n"
            u"      'emm' (0), 'private-data' (1), 'ecm' (2).\n"
            u"\n"
            u"  -v\n"
            u"  --verbose\n"
            u"      Produce verbose output.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    analyze(argc, argv);

    const ts::UString mux(value(u"mux"));
    clientId = intValue<uint32_t>(u"client-id", 0);
    dataId = intValue<uint16_t>(u"data-id", 0);
    channelId = intValue<uint16_t>(u"channel-id", 1);
    streamId = intValue<uint16_t>(u"stream-id", 1);
    dataType = intValue<uint8_t>(u"type", 0);
    sectionMode = present(u"section-mode");
    sendBandwidth = intValue<uint16_t>(u"bandwidth", DEFAULT_BANDWIDTH);
    dataBitrate = sendBandwidth * 1000;
    requestedBandwidth = intValue<uint16_t>(u"requested-bandwidth", sendBandwidth);
    ignoreAllocatedBW = present(u"ignore-allocated");
    emmSize = intValue<size_t>(u"emm-size", DEFAULT_EMM_SIZE);
    emmMinTableId = intValue<ts::TID>(u"emm-min-table-id", DEFAULT_EMM_MIN_TID);
    emmMaxTableId = intValue<ts::TID>(u"emm-max-table-id", DEFAULT_EMM_MAX_TID);
    maxBytes = intValue<uint64_t>(u"max-bytes", std::numeric_limits<uint64_t>::max());
    bytesPerSend = intValue<size_t>(u"bytes-per-send", DEFAULT_BYTES_PER_SEND);
    const ts::tlv::VERSION protocolVersion = intValue<ts::tlv::VERSION>(u"emmg-mux-version", 2);

    // Check validity of some parameters.
    if (emmMaxTableId < emmMinTableId) {
        error(u"--emm-max-table-id 0x%X is less than --emm-min-table-id 0x%X", {emmMaxTableId, emmMinTableId});
    }

    // Resolve MUX address.
    if (mux.empty()) {
        error(u"missing MUX server, use --mux address:port");
    }
    else if (muxAddress.resolve(mux, *this) && (!muxAddress.hasAddress() || !muxAddress.hasPort())) {
        error(u"missing MUX server address or port, use --mux address:port");
    }

    // Specify which EMMG/PDG <=> MUX version to use.
    ts::emmgmux::Protocol::Instance()->setVersion(protocolVersion);

    exitOnError();
}


//----------------------------------------------------------------------------
// Adjust the various rates according to the allocated bandwidth.
//----------------------------------------------------------------------------

bool EMMGOptions::adjustBandwidth(uint16_t allocated)
{
    verbose(u"Allocated bandwidth: %'d kb/s", {allocated});

    // Reduce the bandwidth if not enough was allocated.
    if (sendBandwidth > allocated) {
        if (ignoreAllocatedBW) {
            info(u"Allocated bandwidth %'d kb/s but will send data at %'d kbs/s because of --ignore-allocated", {allocated, sendBandwidth});
        }
        else {
            info(u"Reducing bandwidth to %'d kb/s as allocated by the MUX", {allocated});
            sendBandwidth = allocated;
        }
    }

    // Actual data bitrate.
    dataBitrate = sendBandwidth * 1000;

    // When we work in section mode, there is a packetization overhead of approximately 5/183.
    // It could be less, tending to 4/184 with very large sections. It could be much higher
    // if the MUX does not pack the sections. We use 5/183 since EMM's are usually small
    // sections and we expect the MUX to be efficient and avoid stuffing packets.
    // The section bandwidth SBW is related to the packetized bandwidth PSW using
    // PBW = SBW * (1 + 5/183), meaning SBW = PBW * 183/188.
    if (sectionMode) {
        dataBitrate = (dataBitrate * 183) / 188;
    }

    // Now we have our final data bitrate.
    if (dataBitrate == 0) {
        error(u"no bandwidth available");
        return false;
    }
    info(u"Target data bitrate: %'d b/s", {dataBitrate});

    // Compute interval between two send operations in nanoseconds.
    sendInterval = std::max<ts::NanoSecond>(MIN_SEND_INTERVAL, (bytesPerSend * 8 * ts::NanoSecPerSec) / dataBitrate);

    // Make sure we can have that precision from the system if less than 100 ms.
    if (sendInterval < 100 * ts::NanoSecPerMilliSec) {
        const ts::NanoSecond actualInterval = ts::Monotonic::SetPrecision(sendInterval);
        if (actualInterval > sendInterval) {
            // Cannot get that precision from the system.
            debug(u"requesting %'d ns between send, can get only %'d ns", {sendInterval, actualInterval});
            sendInterval = actualInterval;
        }
    }
    info(u"Send interval: %'d milliseconds", {sendInterval / ts::NanoSecPerMilliSec});

    return true;
}


//----------------------------------------------------------------------------
// A class which provides sections to send.
//----------------------------------------------------------------------------

class EMMGSectionProvider : public ts::SectionProviderInterface
{
public:
    // Constructor.
    EMMGSectionProvider(const EMMGOptions& opt);

    // This hook is invoked when a new section is required.
    // Implementation of SectionProviderInterface.
    virtual void provideSection(ts::SectionCounter counter, ts::SectionPtr& section) override;

    // Shall we perform section stuffing.
    // Implementation of SectionProviderInterface.
    virtual bool doStuffing() override { return false; }

private:
    const EMMGOptions& _opt;
    ts::TID _emmTableId;
    uint8_t _payloadData;

    // Inaccessible operations.
    EMMGSectionProvider() = delete;
    EMMGSectionProvider(const EMMGSectionProvider&) = delete;
    EMMGSectionProvider& operator=(const EMMGSectionProvider&) = delete;
};

// Constructor.
EMMGSectionProvider::EMMGSectionProvider(const EMMGOptions& opt) :
    _opt(opt),
    _emmTableId(opt.emmMinTableId),
    _payloadData(0)
{
}


//----------------------------------------------------------------------------
// Invoked when a new section is required.
//----------------------------------------------------------------------------

void EMMGSectionProvider::provideSection(ts::SectionCounter counter, ts::SectionPtr& section)
{
    // Create a fake EMM payload with all bytes containing the same value.
    // This value is incremented in each new fake EMM.
    assert(_opt.emmSize >= ts::MIN_SHORT_SECTION_SIZE);
    ts::ByteBlock payload(_opt.emmSize - ts::MIN_SHORT_SECTION_SIZE, _payloadData++);

    // Create a fake EMM section.
    section = new ts::Section(_emmTableId, true, payload.data(), payload.size());

    // Compute the next EMM table id.
    _emmTableId = _emmTableId >= _opt.emmMaxTableId ? _opt.emmMinTableId : _emmTableId + 1;
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    TSDuckLibCheckVersion();

    // IP initialization.
    if (!ts::IPInitialize()) {
        return EXIT_FAILURE;
    }

    // Command line options.
    EMMGOptions opt(argc, argv);

    // An object to manage the connection with the MUX.
    ts::EMMGClient client;
    ts::emmgmux::ChannelStatus channelStatus;
    ts::emmgmux::StreamStatus streamStatus;

    // Connect to the MUX.
    opt.verbose(u"Connecting to MUX at %s", {opt.muxAddress.toString()});
    if (!client.connect(opt.muxAddress,
                        opt.clientId,
                        opt.channelId,
                        opt.streamId,
                        opt.dataId,
                        opt.dataType,
                        opt.sectionMode,
                        channelStatus,
                        streamStatus,
                        0,
                        &opt))
    {
        return EXIT_FAILURE;
    }

    // Request the bandwidth, get allocated bandwidth as returned by the MUX and adjust our bitrates.
    if (!client.requestBandwidth(opt.requestedBandwidth, true) ||
        !opt.adjustBandwidth(client.allocatedBandwidth()))
    {
        client.disconnect();
        return EXIT_FAILURE;
    }

    // An object which provides sections to send.
    EMMGSectionProvider sectionProvider(opt);

    // When working in packet mode, we need a packetizer.
    ts::Packetizer packetizer(ts::PID_NULL, &sectionProvider);

    // Start time.
    ts::Monotonic startTime;
    startTime.getSystemTime();

    // This clock will be our reference.
    ts::Monotonic currentTime(startTime);

    // Send data as long as the maximum is not reached.
    bool ok = true;
    while (ok && client.totalBytes() < opt.maxBytes) {

        // Compute the number of bytes we need to send now.
        uint64_t targetBytes = 0;
        ts::NanoSecond duration = currentTime - startTime;
        if (duration <= 0) {
            // First interval, send initial burst.
            targetBytes = opt.bytesPerSend;
        }
        else {
            // Compute the theoretical number of bytes we should have sent up to now.
            const uint64_t allBytes = (opt.dataBitrate * duration) / (8 * ts::NanoSecPerSec);
            // We need to send the difference.
            if (allBytes > client.totalBytes()) {
                targetBytes = allBytes - client.totalBytes();
            }
        }
        opt.debug(u"after %'d ms, sending at least %'d bytes for current interval", {duration / ts::NanoSecPerMilliSec, targetBytes});

        // Send the data we need to send now. Split in several send operations if needed.
        while (ok && targetBytes > 0 && client.totalBytes() < opt.maxBytes) {

            // Size of this send operation.
            const uint64_t targetSendSize = std::min(opt.bytesPerSend, targetBytes);
            uint64_t sendSize = 0;
            opt.debug(u"targetBytes = %'d bytes, targetSendSize = %'d", {targetBytes, targetSendSize});

            // Build a set of data to send.
            if (opt.sectionMode) {
                // Get complete sections from the section provider.
                ts::SectionPtrVector sections;
                while (sendSize < targetSendSize) {
                    ts::SectionPtr sec;
                    sectionProvider.provideSection(0, sec);
                    assert(!sec.isNull());
                    sections.push_back(sec);
                    sendSize += sec->size();
                }

                // Send the sections.
                ok = client.dataProvision(sections);
            }
            else {
                // Get TS packets from the packetizer.
                sendSize = ts::RoundUp(targetSendSize, ts::PKT_SIZE);
                ts::TSPacketVector packets(sendSize / ts::PKT_SIZE);
                for (size_t i = 0; i < packets.size(); ++i) {
                    packetizer.getNextPacket(packets[i]);
                }

                // Send the packets.
                ok = client.dataProvision(packets.data(), packets.size() * ts::PKT_SIZE);
            }
            opt.debug(u"sent %'d bytes in one operation", {sendSize});

            // Any data left for another send operation?
            targetBytes = sendSize > targetBytes ? 0 : targetBytes - sendSize;
        }

        // Wait for the next send operation.
        if (ok && client.totalBytes() < opt.maxBytes) {
            currentTime += opt.sendInterval;
            currentTime.wait();
        }
    }

    // Disconnect from the MUX.
    client.disconnect();
    return EXIT_SUCCESS;
}
