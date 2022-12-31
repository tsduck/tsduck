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
//  Minimal generic DVB SimulCrypt compliant EMMG for CAS head-end integration.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsIntegerUtils.h"
#include "tsEMMGClient.h"
#include "tsUDPSocket.h"
#include "tsPacketizer.h"
#include "tsMonotonic.h"
#include "tsSectionFile.h"
#include "tsTSPacket.h"
TS_MAIN(MainCode);

namespace {
    // Command line default arguments.
    static const uint16_t        DEFAULT_BANDWIDTH      = 100;
    static const size_t          DEFAULT_EMM_SIZE       = 100;
    static const ts::TID         DEFAULT_EMM_MIN_TID    = ts::TID_EMM_FIRST;
    static const ts::TID         DEFAULT_EMM_MAX_TID    = ts::TID_EMM_LAST;
    static const size_t          DEFAULT_BYTES_PER_SEND = 500;
    static const ts::MilliSecond DEFAULT_UDP_END_WAIT   = 100;

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

namespace {
    class EMMGOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(EMMGOptions);
    public:
        EMMGOptions(int argc, char *argv[]);

        ts::DuckContext       duck;                // TSDuck execution context.
        ts::tlv::Logger       logger;              // Message logger.
        ts::UStringVector     inputFiles;          // Input file names.
        ts::SectionPtrVector  sections;            // Loaded sections from input files.
        size_t                maxCycles;           // Maximum number of cycles of section files.
        ts::IPv4SocketAddress tcpMuxAddress;       // TCP server address for MUX.
        ts::IPv4SocketAddress udpMuxAddress;       // UDP server address for MUX.
        bool                  useUDP;              // Use UDP to send data provisions.
        uint32_t              clientId;            // Client id, see EMMG/PDG <=> MUX protocol.
        uint16_t              channelId;           // Data_channel_id, see EMMG/PDG <=> MUX protocol.
        uint16_t              streamId;            // Data_stream_id, see EMMG/PDG <=> MUX protocol.
        uint16_t              dataId;              // Data_id, see EMMG/PDG <=> MUX protocol.
        uint8_t               dataType;            // Data_type, see EMMG/PDG <=> MUX protocol.
        bool                  sectionMode;         // If true, send data in section format.
        uint16_t              sendBandwidth;       // Bandwidth of sent data in kb/s.
        uint16_t              requestedBandwidth;  // Requested bandwidth in kb/s.
        bool                  ignoreAllocatedBW;   // Ignore the returned allocated bandwidth.
        size_t                emmSize;             // Size in bytes of generated EMM's.
        ts::TID               emmMinTableId;       // Minimum table id of generated EMM's.
        ts::TID               emmMaxTableId;       // Maximum table id of generated EMM's.
        uint64_t              maxBytes;            // Stop after injecting that number of bytes.
        ts::BitRate           dataBitrate;         // Actual data bitrate.
        size_t                bytesPerSend;        // Approximate size of each send.
        ts::NanoSecond        sendInterval;        // Interval between two send operations.
        ts::MilliSecond       udpEndWait;          // Number of ms to wait between last UDP message and stream close.

        // Adjust the various rates and delays according to the allocated bandwidth.
        bool adjustBandwidth(uint16_t allocated);
    };
}

EMMGOptions::EMMGOptions(int argc, char *argv[]) :
    ts::Args(u"Minimal generic DVB SimulCrypt-compliant EMMG", u"[options] [section-file ...]"),
    duck(this),
    logger(ts::Severity::Debug, this),
    inputFiles(),
    sections(),
    maxCycles(0),
    tcpMuxAddress(),
    udpMuxAddress(),
    useUDP(false),
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
    sendInterval(0),
    udpEndWait(0)
{
    option(u"", 0, FILENAME, 0, UNLIMITED_COUNT);
    help(u"",
         u"The parameters are files containing sections in binary or XML format. Several "
         u"files can be specified. All sections are loaded and injected in the MUX using "
         u"the EMMG/PDG <=> MUX protocol. The list of all sections from all files is "
         u"cycled as long as tsemmg is running. The sections can be of any type, not "
         u"only EMM's.\n\n"
         u"If no input file is specified, tsemmg generates fixed-size fake EMM's. See "
         u"options --emm-size, --emm-min-table-id and --emm-max-table-id.");

    option(u"bandwidth", 'b', INTEGER, 0, 1, 1, 0xFFFF);
    help(u"bandwidth",
         u"Specify the bandwidth of the data which are sent to the MUX in kilobits "
         u"per second. Default: " + ts::UString::Decimal(DEFAULT_BANDWIDTH) + u" kb/s.");

    option(u"bytes-per-send", 0, INTEGER, 0, 1, 0x20, 0xEFFF);
    help(u"bytes-per-send",
         u"Specify the average size in bytes of each data provision. The exact value "
         u"depends on sections and packets sizes. Default: " + ts::UString::Decimal(DEFAULT_BYTES_PER_SEND) + u" bytes.");

    option(u"channel-id", 0, INT16);
    help(u"channel-id",
         u"This option sets the DVB SimulCrypt parameter 'data_channel_id'. "
         u"Default: 1.");

    option(u"client-id", 'c', INT32);
    help(u"client-id",
         u"This option sets the DVB SimulCrypt parameter 'client_id'. Default: 0. "
         u"For EMM injection, the most signification 16 bits shall be the "
         u"'CA_system_id' of the corresponding CAS.");

    option(u"cycles", 0, UNSIGNED);
    help(u"cycles",
         u"Inject the sections from the input files the specified number of times. "
         u"By default, inject sections indefinitely.");

    option(u"data-id", 'd', INT16);
    help(u"data-id", u"This option sets the DVB SimulCrypt parameter 'data_id'. Default: 0.");

    option(u"emm-size", 0, INTEGER, 0, 1, ts::MIN_SHORT_SECTION_SIZE, ts::MAX_PRIVATE_SECTION_SIZE);
    help(u"emm-size",
         u"Specify the size in bytes of the automatically generated fake EMM's. The default is 100 bytes.");

    option(u"emm-min-table-id", 0, UINT8);
    help(u"emm-min-table-id",
         u"Specify the minimum table id of the automatically generated fake EMM's. "
         u"The default is " + ts::UString::Hexa(DEFAULT_EMM_MIN_TID) + u".");

    option(u"emm-max-table-id", 0, UINT8);
    help(u"emm-max-table-id",
         u"Specify the maximum table id of the automatically generated fake EMM's. "
         u"When generating fake EMM's, the table ids are cycled from the minimum to "
         u"the maximum value. The default is " + ts::UString::Hexa(DEFAULT_EMM_MAX_TID) + u".");

    option(u"emmg-mux-version", 0, INTEGER, 0, 1, 1, 5);
    help(u"emmg-mux-version",
         u"Specify the version of the EMMG/PDG <=> MUX DVB SimulCrypt protocol. "
         u"Valid values are 1 to 5. The default is 2.");

    option(u"ignore-allocated", 'i');
    help(u"ignore-allocated",
         u"Ignore the allocated bandwidth as returned by the MUX, continue to send "
         u"data at the planned bandwidth, even if it is higher than the allocated bandwidth.");

    option(u"log-data", 0, ts::Severity::Enums, 0, 1, true);
    help(u"log-data",
         u"Same as --log-protocol but applies to data_provision messages only. To "
         u"debug the session management without being flooded by data messages, use "
         u"--log-protocol=info --log-data=debug.");

    option(u"log-protocol", 0, ts::Severity::Enums, 0, 1, true);
    help(u"log-protocol",
         u"Log all EMMG/PDG <=> MUX protocol messages using the specified level. If "
         u"the option is not present, the messages are logged at debug level only. "
         u"If the option is present without value, the messages are logged at info "
         u"level. A level can be a numerical debug level or a name.");

    option(u"max-bytes", 0, UNSIGNED);
    help(u"max-bytes",
         u"Stop after sending the specified number of bytes. By default, send data "
         u"indefinitely.");

    option(u"mux", 'm', STRING);
    help(u"mux", u"address:port",
         u"Specify the IP address (or host name) and TCP port of the MUX. This is a "
         u"required parameter, there is no default.");

    option(u"requested-bandwidth", 0, INT16);
    help(u"requested-bandwidth",
         u"This option sets the DVB SimulCrypt parameter 'bandwidth' in the "
         u"'stream_BW_request' message. The value is in kilobits per second. The "
         u"default is the value of the --bandwidth option. Specifying distinct values "
         u"for --bandwidth and --requested-bandwidth can be used for testing the "
         u"behavior of a MUX.");

    option(u"section-mode", 's');
    help(u"section-mode",
         u"Send EMM's or data in section format. This option sets the DVB SimulCrypt "
         u"parameter 'section_TSpkt_flag' to zero. By default, EMM's and data are "
         u"sent in TS packet format.");

    option(u"stream-id", 0, INT16);
    help(u"stream-id",
         u"This option sets the DVB SimulCrypt parameter 'data_stream_id'. "
         u"Default: 1.");

    option(u"type", 't', DataTypeEnum);
    help(u"type",
         u"This option sets the DVB SimulCrypt parameter 'data_type'. Default: 0 (EMM). "
         u"In addition to integer values, names can be used.");

    option(u"udp", 'u', STRING);
    help(u"udp", u"[address:]port",
         u"Specify that the 'data_provision' messages shall be sent using UDP. By "
         u"default, the 'data_provision' messages are sent over TCP using the same "
         u"TCP connection as the management commands. If the IP address (or host "
         u"name) is not specified, use the same IP address as the --mux option. The "
         u"port number is required, even if it is the same as the TCP port.");

    option(u"udp-end-wait", 'w', UNSIGNED);
    help(u"udp-end-wait", u"milliseconds",
         u"With --udp, specify the number of milliseconds to wait after the last "
         u"data_provision message (UDP) and before the stream_close_request message (TCP). "
         u"This can be necesssary to ensure that the stream_close_request is "
         u"processed after the processing of the last data_provision. "
         u"Default: " + ts::UString::Decimal(DEFAULT_UDP_END_WAIT) + u" ms.");

    analyze(argc, argv);

    getValues(inputFiles);
    getIntValue(maxCycles, u"cycles");
    const ts::UString tcpMux(value(u"mux"));
    const ts::UString udpMux(value(u"udp"));
    getIntValue(clientId, u"client-id", 0);
    getIntValue(dataId, u"data-id", 0);
    getIntValue(channelId, u"channel-id", 1);
    getIntValue(streamId, u"stream-id", 1);
    getIntValue(dataType, u"type", 0);
    sectionMode = present(u"section-mode");
    getIntValue(sendBandwidth, u"bandwidth", DEFAULT_BANDWIDTH);
    dataBitrate = sendBandwidth * 1000;
    getIntValue(requestedBandwidth, u"requested-bandwidth", sendBandwidth);
    ignoreAllocatedBW = present(u"ignore-allocated");
    getIntValue(emmSize, u"emm-size", DEFAULT_EMM_SIZE);
    getIntValue(emmMinTableId, u"emm-min-table-id", DEFAULT_EMM_MIN_TID);
    getIntValue(emmMaxTableId, u"emm-max-table-id", DEFAULT_EMM_MAX_TID);
    getIntValue(maxBytes, u"max-bytes", std::numeric_limits<uint64_t>::max());
    getIntValue(bytesPerSend, u"bytes-per-send", DEFAULT_BYTES_PER_SEND);
    getIntValue(udpEndWait, u"udp-end-wait", DEFAULT_UDP_END_WAIT);
    const ts::tlv::VERSION protocolVersion = intValue<ts::tlv::VERSION>(u"emmg-mux-version", 2);

    // Set logging levels.
    const int log_protocol = present(u"log-protocol") ? intValue<int>(u"log-protocol", ts::Severity::Info) : ts::Severity::Debug;
    const int log_data = present(u"log-data") ? intValue<int>(u"log-data", ts::Severity::Info) : log_protocol;
    logger.setDefaultSeverity(log_protocol);
    logger.setSeverity(ts::emmgmux::Tags::data_provision, log_data);

    // Check validity of some parameters.
    if (emmMaxTableId < emmMinTableId) {
        error(u"--emm-max-table-id 0x%X is less than --emm-min-table-id 0x%X", {emmMaxTableId, emmMinTableId});
    }

    // Resolve MUX address.
    if (tcpMux.empty()) {
        error(u"missing MUX server, use --mux address:port");
    }
    else if (tcpMuxAddress.resolve(tcpMux, *this) && (!tcpMuxAddress.hasAddress() || !tcpMuxAddress.hasPort())) {
        error(u"missing MUX server address or port, use --mux address:port");
    }

    // Check if UDP is used for data provision.
    useUDP = !udpMux.empty();
    if (useUDP && udpMuxAddress.resolve(udpMux, *this)) {
        // Use same address as TCP by default.
        if (!udpMuxAddress.hasAddress()) {
            udpMuxAddress.setAddress(tcpMuxAddress.address());
        }
        // UDP port is mandatory.
        if (!udpMuxAddress.hasPort()) {
            error(u"missing port in --udp [address:]port");
        }
    }

    // Specify which EMMG/PDG <=> MUX version to use.
    ts::emmgmux::Protocol::Instance()->setVersion(protocolVersion);

    // Load sections from input files.
    for (const auto& it : inputFiles) {
        ts::SectionFile file(duck);
        file.setCRCValidation(ts::CRC32::CHECK);
        if (file.load(it)) {
            sections.insert(sections.end(), file.sections().begin(), file.sections().end());
        }
    }

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
    sendInterval = std::max<ts::NanoSecond>(MIN_SEND_INTERVAL, ((bytesPerSend * 8 * ts::NanoSecPerSec) / dataBitrate).toInt());

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
    TS_NOBUILD_NOCOPY(EMMGSectionProvider);
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
    size_t  _nextSection;
    size_t  _cycleCount;
};

// Constructor.
EMMGSectionProvider::EMMGSectionProvider(const EMMGOptions& opt) :
    _opt(opt),
    _emmTableId(opt.emmMinTableId),
    _payloadData(0),
    _nextSection(0),
    _cycleCount(0)
{
}


//----------------------------------------------------------------------------
// Invoked when a new section is required.
//----------------------------------------------------------------------------

void EMMGSectionProvider::provideSection(ts::SectionCounter counter, ts::SectionPtr& section)
{
    if (_opt.inputFiles.empty()) {
        // There is no input file.
        // Create a fake EMM payload with all bytes containing the same value.
        // This value is incremented in each new fake EMM.
        assert(_opt.emmSize >= ts::MIN_SHORT_SECTION_SIZE);
        ts::ByteBlock payload(_opt.emmSize - ts::MIN_SHORT_SECTION_SIZE, _payloadData++);

        // Create a fake EMM section.
        section = new ts::Section(_emmTableId, true, payload.data(), payload.size());

        // Compute the next EMM table id.
        _emmTableId = _emmTableId >= _opt.emmMaxTableId ? _opt.emmMinTableId : _emmTableId + 1;
    }
    else if (_opt.maxCycles > 0 && _cycleCount >= _opt.maxCycles) {
        // The total number of cycles has been exhausted.
        section.clear();
    }
    else {
        // Get the next loaded section.
        section = _opt.sections[_nextSection];
        if (++_nextSection >= _opt.sections.size()) {
            _nextSection = 0;
            _cycleCount++;
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Command line options.
    EMMGOptions opt(argc, argv);

    // An object to manage the TCP connection with the MUX.
    ts::EMMGClient client(opt.duck);
    ts::emmgmux::ChannelStatus channelStatus;
    ts::emmgmux::StreamStatus streamStatus;

    // UDP socket for the data_provision messages.
    ts::UDPSocket udpSocket;
    if (opt.useUDP && !udpSocket.open(opt)) {
        return EXIT_FAILURE;
    }

    // Connect to the MUX.
    opt.verbose(u"Connecting to MUX at %s", {opt.tcpMuxAddress});
    if (!client.connect(opt.tcpMuxAddress,
                        opt.udpMuxAddress,
                        opt.clientId,
                        opt.channelId,
                        opt.streamId,
                        opt.dataId,
                        opt.dataType,
                        opt.sectionMode,
                        channelStatus,
                        streamStatus,
                        nullptr,
                        opt.logger))
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
    ts::Packetizer packetizer(opt.duck, ts::PID_NULL, &sectionProvider);

    // Start time.
    ts::Monotonic startTime(true);

    // This clock will be our reference.
    ts::Monotonic currentTime(startTime);

    // Send data as long as the maximum is not reached.
    bool ok = true;
    while (ok && client.totalBytes() < opt.maxBytes) {

        // Compute the number of bytes we need to send now.
        // Use microseconds instead of nanoseconds to avoid too frequent overflows
        // (the difference between two Monotonic clock values are in nanoseconds).
        uint64_t targetBytes = 0;
        ts::MicroSecond duration = (currentTime - startTime) / ts::NanoSecPerMicroSec;
        if (duration <= 0) {
            // First interval, send initial burst.
            targetBytes = opt.bytesPerSend;
        }
        else if (!opt.dataBitrate.mulOverflow(duration) && !(opt.dataBitrate * duration).divOverflow(8 * ts::MicroSecPerSec)) {
            // Compute the theoretical number of bytes we should have sent up to now. No overflow.
            const uint64_t allBytes = ((opt.dataBitrate * duration) / (8 * ts::MicroSecPerSec)).toInt();
            // We need to send the difference.
            if (allBytes > client.totalBytes()) {
                targetBytes = allBytes - client.totalBytes();
            }
        }
        else {
            // Overflow if we count from the beginning, restart the count.
            opt.debug(u"overflow in bitrate computation, resetting bitrate accumulation, bitrate: %'d b/s, duration: %'d ns", {opt.dataBitrate, duration});
            startTime = currentTime;
            targetBytes = opt.bytesPerSend;
        }

        // Send the data we need to send now. Split in several send operations if needed.
        while (ok && targetBytes > 0 && client.totalBytes() < opt.maxBytes) {

            // Size of this send operation.
            const uint64_t targetSendSize = std::min<uint64_t>(opt.bytesPerSend, targetBytes);
            uint64_t sendSize = 0;

            // Build a set of data to send.
            if (opt.sectionMode) {
                // Get complete sections from the section provider.
                ts::SectionPtrVector sections;
                while (ok && sendSize < targetSendSize) {
                    // Get one section.
                    ts::SectionPtr sec;
                    sectionProvider.provideSection(0, sec);
                    // Getting a null pointer means end of input.
                    ok = !sec.isNull();
                    if (ok) {
                        sections.push_back(sec);
                        sendSize += sec->size();
                    }
                }

                // Send the sections.
                ok = client.dataProvision(sections) && ok;
            }
            else {
                // Get TS packets from the packetizer.
                sendSize = ts::round_up<uint64_t>(targetSendSize, ts::PKT_SIZE);
                ts::TSPacketVector packets(size_t(sendSize / ts::PKT_SIZE));
                for (size_t i = 0; ok && i < packets.size(); ++i) {
                    ok = packetizer.getNextPacket(packets[i]);
                    if (!ok) {
                        // No more packet, shrink the packet buffer.
                        packets.resize(i);
                    }
                }

                // Send the packets.
                ok = client.dataProvision(packets.data(), packets.size() * ts::PKT_SIZE) && ok;
            }

            // Any data left for another send operation?
            targetBytes = sendSize > targetBytes ? 0 : targetBytes - sendSize;
        }

        // Wait for the next send operation.
        if (ok && client.totalBytes() < opt.maxBytes) {
            currentTime += opt.sendInterval;
            currentTime.wait();
        }
    }

    // With UDP data_provision message, optionally wait before closing the session.
    if (opt.udpMuxAddress.hasPort() && opt.udpEndWait > 0) {
        ts::SleepThread(opt.udpEndWait);
    }

    // Disconnect from the MUX.
    client.disconnect();
    return EXIT_SUCCESS;
}
