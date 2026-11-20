//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2026, Thomas Drapier
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for the HLS output plugin.
//
//----------------------------------------------------------------------------

#include "tsPluginEventHandlerInterface.h"
#include "tsPluginEventData.h"
#include "tsTSProcessor.h"
#include "tsAsyncReport.h"
#include "tsOneShotPacketizer.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tshlsPlayList.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class HLSOutputPluginTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(MaxExtraDuration);
    TSUNIT_DECLARE_TEST(IndependentSegments);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    fs::path _temp_dir_name {};
};

TSUNIT_REGISTER(HLSOutputPluginTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

void HLSOutputPluginTest::beforeTest()
{
    if (_temp_dir_name.empty()) {
        _temp_dir_name = ts::TempFile(u"");
    }
    fs::remove_all(_temp_dir_name, &ts::ErrCodeReport());
    fs::create_directory(_temp_dir_name, &ts::ErrCodeReport(CERR, u"error creating directory %s", _temp_dir_name));
}

void HLSOutputPluginTest::afterTest()
{
    fs::remove_all(_temp_dir_name, &ts::ErrCodeReport());
}


//----------------------------------------------------------------------------
// Synthetic streams: one service, video on PID 0x100 (also PCR PID),
// 25 frames per second, one PES per frame, 10 TS packets per frame.
// The packet rate is constant, so a segment of N packets lasts N / 250 seconds.
// The PAT and PMT use the first two packets of the first frame of each second.
//----------------------------------------------------------------------------

namespace {

    constexpr size_t FRAME_RATE = 25;
    constexpr size_t PACKETS_PER_FRAME = 10;
    constexpr size_t PACKET_RATE = FRAME_RATE * PACKETS_PER_FRAME;
    constexpr size_t STREAM_SECONDS = 12;
    constexpr ts::PID VIDEO_PID = 0x0100;
    constexpr ts::PID PMT_PID = 0x1000;
    constexpr uint16_t SERVICE_ID = 1;

    enum class GOPStart {
        AVC_NO_INTRA,
        AVC_I,
        AVC_IDR,
        MPEG2_OPEN,
        MPEG2_CLOSED,
    };

    bool IsMPEG2(GOPStart start)
    {
        return start == GOPStart::MPEG2_OPEN || start == GOPStart::MPEG2_CLOSED;
    }

    ts::ByteBlock VideoPES(GOPStart start, size_t frame)
    {
        const bool intra = start != GOPStart::AVC_NO_INTRA && frame % FRAME_RATE == 0;
        const uint64_t pts = frame * 3600;

        ts::ByteBlock pes({0x00, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x80, 0x80, 0x05,
                           uint8_t(0x21 | ((pts >> 29) & 0x0E)), uint8_t(pts >> 22), uint8_t(0x01 | ((pts >> 14) & 0xFE)),
                           uint8_t(pts >> 7), uint8_t(0x01 | ((pts << 1) & 0xFE))});

        if (IsMPEG2(start)) {
            if (intra) {
                pes.insert(pes.end(), {0x00, 0x00, 0x01, 0xB3, 0x78, 0x04, 0x38, 0x33, 0xFF, 0xFF, 0xE0, 0x18});
                pes.insert(pes.end(), {0x00, 0x00, 0x01, 0xB8, 0x00, 0x08, 0x00, uint8_t(start == GOPStart::MPEG2_CLOSED ? 0x40 : 0x00)});
            }
            const uint8_t coding_type = intra ? 1 : 2;
            const uint16_t tref = uint16_t(frame % 1024);
            pes.insert(pes.end(), {0x00, 0x00, 0x01, 0x00, uint8_t(tref >> 2), uint8_t(((tref & 3) << 6) | (coding_type << 3) | 0x07), 0xFF, 0xF8});
        }
        else if (!intra) {
            pes.insert(pes.end(), {0x00, 0x00, 0x00, 0x01, 0x09, 0x30, 0x00, 0x00, 0x01, 0x41, 0x9A, 0x84});
        }
        else if (start == GOPStart::AVC_IDR) {
            // A 200-byte SEI moves the IDR slice to the second TS packet, as with real encoders.
            pes.insert(pes.end(), {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0x00, 0x00, 0x01, 0x06, 0x05, 0xC8});
            pes.insert(pes.end(), 200, 0x11);
            pes.insert(pes.end(), {0x80, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84});
        }
        else {
            pes.insert(pes.end(), {0x00, 0x00, 0x00, 0x01, 0x09, 0x10, 0x00, 0x00, 0x01, 0x41, 0x88, 0x84});
        }
        return pes;
    }

    ts::TSPacketVector BuildStream(GOPStart start, size_t first_frame = 0)
    {
        ts::DuckContext duck;

        ts::PAT pat(0, true, 1);
        pat.pmts[SERVICE_ID] = PMT_PID;
        ts::PMT pmt(0, true, SERVICE_ID, VIDEO_PID);
        pmt.streams[VIDEO_PID].stream_type = IsMPEG2(start) ? ts::ST_MPEG2_VIDEO : ts::ST_AVC_VIDEO;

        ts::TSPacketVector psi;
        ts::BinaryTable bin;
        pat.serialize(duck, bin);
        ts::OneShotPacketizer pat_pzer(duck, ts::PID_PAT);
        pat_pzer.addTable(bin);
        pat_pzer.getPackets(psi);
        ts::TSPacketVector pmt_packets;
        pmt.serialize(duck, bin);
        ts::OneShotPacketizer pmt_pzer(duck, PMT_PID);
        pmt_pzer.addTable(bin);
        pmt_pzer.getPackets(pmt_packets);
        psi.insert(psi.end(), pmt_packets.begin(), pmt_packets.end());

        // A null packet before the PAT and PMT does not prevent the first segment from being independent.
        ts::TSPacketVector packets({ts::NullPacket});
        uint8_t psi_cc = 0;
        uint8_t video_cc = 0;
        for (size_t frame = first_frame; frame < STREAM_SECONDS * FRAME_RATE; ++frame) {
            size_t slots = PACKETS_PER_FRAME;
            if (frame % FRAME_RATE == 0) {
                for (const auto& p : psi) {
                    packets.push_back(p);
                    packets.back().setCC(psi_cc);
                }
                psi_cc = (psi_cc + 1) & ts::CC_MASK;
                slots -= psi.size();
            }
            const ts::ByteBlock pes(VideoPES(start, frame));
            size_t pes_done = 0;
            for (size_t i = 0; i < slots; ++i) {
                ts::TSPacket pkt;
                pkt.init(VIDEO_PID, video_cc, 0xFF);
                video_cc = (video_cc + 1) & ts::CC_MASK;
                if (i == 0) {
                    pkt.setPUSI();
                    pkt.setPCR(packets.size() * (ts::SYSTEM_CLOCK_FREQ / PACKET_RATE), true);
                }
                const size_t chunk = std::min(pes.size() - pes_done, pkt.getPayloadSize());
                ts::MemCopy(pkt.getPayload(), pes.data() + pes_done, chunk);
                pes_done += chunk;
                packets.push_back(pkt);
            }
        }
        return packets;
    }

    class Input: public ts::PluginEventHandlerInterface
    {
        TS_NOBUILD_NOCOPY(Input);
    public:
        Input(const ts::TSPacketVector& packets) : _packets(packets) {}
        virtual void handlePluginEvent(const ts::PluginEventContext& context) override
        {
            ts::PluginEventData* data = dynamic_cast<ts::PluginEventData*>(context.pluginData());
            if (data != nullptr && _next < _packets.size()) {
                const size_t count = std::min(_packets.size() - _next, data->maxSize() / ts::PKT_SIZE);
                data->append(&_packets[_next], count * ts::PKT_SIZE);
                _next += count;
            }
        }
    private:
        const ts::TSPacketVector& _packets;
        size_t _next = 0;
    };

    ts::UStringList RunHLS(const ts::TSPacketVector& packets, const fs::path& dir, const ts::UStringVector& args)
    {
        const ts::UString playlist(ts::UString(dir / u"playlist.m3u8"));
        ts::UStringVector hls_args(args);
        hls_args.push_back(u"--playlist");
        hls_args.push_back(playlist);
        hls_args.push_back(ts::UString(dir / u"seg.ts"));

        ts::AsyncReport log(tsunit::Debug::On() ? ts::Severity::Debug : ts::Severity::Warning);
        Input input(packets);
        ts::TSProcessorArgs opt;
        opt.input = {u"memory", {}};
        opt.output = {u"hls", hls_args};

        ts::TSProcessor tsp(log);
        tsp.registerEventHandler(&input, ts::PluginType::INPUT);
        TSUNIT_ASSERT(tsp.start(opt));
        tsp.waitForTermination();

        ts::UStringList lines;
        TSUNIT_ASSERT(ts::UString::Load(lines, playlist));
        for (const auto& line : lines) {
            tsunit::Debug() << "HLSOutputPluginTest: " << line << std::endl;
        }
        return lines;
    }

    bool HasTag(const ts::UStringList& lines, const ts::UString& tag)
    {
        return std::find(lines.begin(), lines.end(), tag) != lines.end();
    }
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Without any intra picture, --max-extra-duration shall close each segment
// on the first video PES packet after --duration + --max-extra-duration.
TSUNIT_DEFINE_TEST(MaxExtraDuration)
{
    RunHLS(BuildStream(GOPStart::AVC_NO_INTRA), _temp_dir_name, {u"--intra-close", u"--duration", u"2", u"--max-extra-duration", u"1"});

    ts::hls::PlayList pl(&CERR);
    TSUNIT_ASSERT(pl.loadFile(ts::UString(_temp_dir_name / u"playlist.m3u8")));

    // 12 seconds cut after 3 seconds plus less than one frame: 4 segments.
    TSUNIT_EQUAL(4, pl.segmentCount());
    for (size_t i = 0; i + 1 < pl.segmentCount(); ++i) {
        const cn::milliseconds duration = pl.segment(i).duration;
        Debug() << "HLSOutputPluginTest::MaxExtraDuration: segment " << i << ": " << duration.count() << " ms" << std::endl;
        TSUNIT_ASSERT(duration >= cn::milliseconds(3000));
        TSUNIT_ASSERT(duration <= cn::milliseconds(3000 + 1000 / FRAME_RATE));
    }
}

// EXT-X-INDEPENDENT-SEGMENTS is only valid when all segments start on a closed random access point.
TSUNIT_DEFINE_TEST(IndependentSegments)
{
    const ts::UStringVector args({u"--intra-close", u"--duration", u"2", u"--max-extra-duration", u"1"});
    const ts::UString tag(u"#EXT-X-INDEPENDENT-SEGMENTS");

    TSUNIT_ASSERT(HasTag(RunHLS(BuildStream(GOPStart::AVC_IDR), _temp_dir_name, args), tag));
    TSUNIT_ASSERT(HasTag(RunHLS(BuildStream(GOPStart::MPEG2_CLOSED), _temp_dir_name, args), tag));
    TSUNIT_ASSERT(!HasTag(RunHLS(BuildStream(GOPStart::AVC_I), _temp_dir_name, args), tag));
    TSUNIT_ASSERT(!HasTag(RunHLS(BuildStream(GOPStart::MPEG2_OPEN), _temp_dir_name, args), tag));
    TSUNIT_ASSERT(!HasTag(RunHLS(BuildStream(GOPStart::AVC_NO_INTRA), _temp_dir_name, args), tag));

    // A live playlist keeps the tag only if no segment ever lacked it, even after that segment is removed.
    const ts::UStringVector live({u"--intra-close", u"--duration", u"2", u"--live", u"2"});
    TSUNIT_ASSERT(HasTag(RunHLS(BuildStream(GOPStart::AVC_IDR), _temp_dir_name, live), tag));
    TSUNIT_ASSERT(!HasTag(RunHLS(BuildStream(GOPStart::AVC_IDR, 5), _temp_dir_name, live), tag));
}
