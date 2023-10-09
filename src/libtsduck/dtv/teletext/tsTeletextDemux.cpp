//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Parts of this module are inspired from Telxcc, a free open-source Teletext
// extractor from Petr Kutalek (https://github.com/petrkutalek/telxcc).
// Copyright: (c) 2011-2014 Forers, s. r. o.: telxcc
//
//----------------------------------------------------------------------------
//
// Relevant standards:
//
// - ETSI EN 300 472 V1.3.1 (2003-05)
//   Digital Video Broadcasting (DVB);
//   Specification for conveying ITU-R System B Teletext in DVB bitstreams
// - ETSI EN 300 706 V1.2.1 (2003-04)
//   Enhanced Teletext specification
// - ETSI EN 300 708 V1.2.1 (2003-04)
//   Television systems; Data transmission within Teletext
//
//----------------------------------------------------------------------------

#include "tsTeletextDemux.h"
#include "tsTeletextFrame.h"
#include "tsMemory.h"


//-----------------------------------------------------------------------------
// From various original sources.
//-----------------------------------------------------------------------------

namespace {

    // Static table to remove 8/4 Hamming code.
    const uint8_t UNHAM_8_4[256] = {
        0x01, 0xFF, 0x01, 0x01, 0xFF, 0x00, 0x01, 0xFF, 0xFF, 0x02, 0x01, 0xFF, 0x0A, 0xFF, 0xFF, 0x07,
        0xFF, 0x00, 0x01, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x06, 0xFF, 0xFF, 0x0B, 0xFF, 0x00, 0x03, 0xFF,
        0xFF, 0x0C, 0x01, 0xFF, 0x04, 0xFF, 0xFF, 0x07, 0x06, 0xFF, 0xFF, 0x07, 0xFF, 0x07, 0x07, 0x07,
        0x06, 0xFF, 0xFF, 0x05, 0xFF, 0x00, 0x0D, 0xFF, 0x06, 0x06, 0x06, 0xFF, 0x06, 0xFF, 0xFF, 0x07,
        0xFF, 0x02, 0x01, 0xFF, 0x04, 0xFF, 0xFF, 0x09, 0x02, 0x02, 0xFF, 0x02, 0xFF, 0x02, 0x03, 0xFF,
        0x08, 0xFF, 0xFF, 0x05, 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0x02, 0x03, 0xFF, 0x03, 0xFF, 0x03, 0x03,
        0x04, 0xFF, 0xFF, 0x05, 0x04, 0x04, 0x04, 0xFF, 0xFF, 0x02, 0x0F, 0xFF, 0x04, 0xFF, 0xFF, 0x07,
        0xFF, 0x05, 0x05, 0x05, 0x04, 0xFF, 0xFF, 0x05, 0x06, 0xFF, 0xFF, 0x05, 0xFF, 0x0E, 0x03, 0xFF,
        0xFF, 0x0C, 0x01, 0xFF, 0x0A, 0xFF, 0xFF, 0x09, 0x0A, 0xFF, 0xFF, 0x0B, 0x0A, 0x0A, 0x0A, 0xFF,
        0x08, 0xFF, 0xFF, 0x0B, 0xFF, 0x00, 0x0D, 0xFF, 0xFF, 0x0B, 0x0B, 0x0B, 0x0A, 0xFF, 0xFF, 0x0B,
        0x0C, 0x0C, 0xFF, 0x0C, 0xFF, 0x0C, 0x0D, 0xFF, 0xFF, 0x0C, 0x0F, 0xFF, 0x0A, 0xFF, 0xFF, 0x07,
        0xFF, 0x0C, 0x0D, 0xFF, 0x0D, 0xFF, 0x0D, 0x0D, 0x06, 0xFF, 0xFF, 0x0B, 0xFF, 0x0E, 0x0D, 0xFF,
        0x08, 0xFF, 0xFF, 0x09, 0xFF, 0x09, 0x09, 0x09, 0xFF, 0x02, 0x0F, 0xFF, 0x0A, 0xFF, 0xFF, 0x09,
        0x08, 0x08, 0x08, 0xFF, 0x08, 0xFF, 0xFF, 0x09, 0x08, 0xFF, 0xFF, 0x0B, 0xFF, 0x0E, 0x03, 0xFF,
        0xFF, 0x0C, 0x0F, 0xFF, 0x04, 0xFF, 0xFF, 0x09, 0x0F, 0xFF, 0x0F, 0x0F, 0xFF, 0x0E, 0x0F, 0xFF,
        0x08, 0xFF, 0xFF, 0x05, 0xFF, 0x0E, 0x0D, 0xFF, 0xFF, 0x0E, 0x0F, 0xFF, 0x0E, 0x0E, 0xFF, 0x0E
    };

    // Static table to swap bits in a byte.
    const uint8_t REVERSE_8[256] = {
        0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
        0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
        0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
        0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
        0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
        0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
        0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
        0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
        0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
        0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
        0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
        0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
        0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
        0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
        0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
        0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
    };

    // Text foreground color codes.
    const ts::UChar* const TELETEXT_COLORS[8] = {
        // 0=black, 1=red,      2=green,    3=yellow,   4=blue,     5=magenta,  6=cyan,     7=white
        u"#000000", u"#FF0000", u"#00FF00", u"#FFFF00", u"#0000FF", u"#FF00FF", u"#00FFFF", u"#FFFFFF"
    };
}


//-----------------------------------------------------------------------------
// Remove 8/4 and 24/18 Hamming code.
//-----------------------------------------------------------------------------

uint8_t ts::TeletextDemux::unham_8_4(uint8_t a)
{
    const uint8_t r = UNHAM_8_4[a];
    return r == 0xFF ? 0x00 : r & 0x0F;
}

uint32_t ts::TeletextDemux::unham_24_18(uint32_t a)
{
    uint8_t test = 0;

    // Tests A-F correspond to bits 0-6 respectively in 'test'.
    for (uint8_t i = 0; i < 23; i++) {
        test ^= ((a >> i) & 0x01) * (i + 33);
    }

    // Only parity bit is tested for bit 24
    test ^= ((a >> 23) & 0x01) * 32;

    if ((test & 0x1F) != 0x1F) {
        // Not all tests A-E correct
        if ((test & 0x20) == 0x20) {
            // F correct: Double error
            return 0xFFFFFFFF;
        }
        // Test F incorrect: Single error
        a ^= 1 << (30 - test);
    }

    return (a & 0x000004) >> 2 | (a & 0x000070) >> 3 | (a & 0x007F00) >> 4 | (a & 0x7F0000) >> 5;
}


//-----------------------------------------------------------------------------
// Convert a page number between binary and BCD.
//-----------------------------------------------------------------------------

int ts::TeletextDemux::pageBcdToBinary(int bcd)
{
    return 100 * ((bcd >> 8) & 0x0F) + 10 * ((bcd >> 4) & 0x0F) + (bcd & 0x0F);
}

int ts::TeletextDemux::pageBinaryToBcd(int bin)
{
    return (((bin / 100) % 10) << 8) | (((bin / 10) % 10) << 4) | (bin % 10);
}


//-----------------------------------------------------------------------------
// Constructors and destructors.
//-----------------------------------------------------------------------------

ts::TeletextDemux::TeletextDemux(DuckContext& duck, TeletextHandlerInterface* handler, const PIDSet& pidFilter) :
    SuperClass(duck, nullptr, pidFilter),
    _txtHandler(handler),
    _pids(),
    _addColors(false)
{
}

ts::TeletextDemux::~TeletextDemux()
{
    flushTeletext();
}

ts::TeletextDemux::TeletextPage::TeletextPage() :
    frameCount(0),
    showTimestamp(0),
    hideTimestamp(0),
    tainted(false),
    charset()
{
    TS_ZERO(text);
}

void ts::TeletextDemux::TeletextPage::reset(MilliSecond timestamp)
{
    showTimestamp = timestamp;
    hideTimestamp = 0;
    tainted = false;
    TS_ZERO(text);
}

ts::TeletextDemux::PIDContext::PIDContext() :
    receivingData(false),
    transMode(TRANSMODE_SERIAL),
    currentPage(0),
    pages()
{
}


//-----------------------------------------------------------------------------
// Reset the analysis context (partially built TELETEXT packets).
//-----------------------------------------------------------------------------

void ts::TeletextDemux::immediateReset()
{
    _pids.clear();
    SuperClass::immediateReset();
}

void ts::TeletextDemux::immediateResetPID(PID pid)
{
    _pids.erase(pid);
    SuperClass::immediateResetPID(pid);
}


//-----------------------------------------------------------------------------
// This hook is invoked when a complete PES packet is available.
//-----------------------------------------------------------------------------

void ts::TeletextDemux::handlePESPacket(const PESPacket& packet)
{
    // Invoke superclass.
    SuperClass::handlePESPacket(packet);

    // Create PID context if non existent.
    const PID pid = packet.sourcePID();
    PIDContext& pc(_pids[pid]);

    // Explore PES payload.
    const uint8_t* pl = packet.payload();
    size_t plSize = packet.payloadSize();

    // The first byte is a data_identifier.
    if (plSize < 1 || *pl < TELETEXT_PES_FIRST_EBU_DATA_ID || *pl > TELETEXT_PES_LAST_EBU_DATA_ID) {
        // Not a valid Teletext PES packet.
        return;
    }
    pl++;
    plSize--;

    // Loop on all data units inside the PES payload.
    while (plSize >= 2) {

        // Data unit header (2 bytes).
        const TeletextDataUnitId unitId = TeletextDataUnitId(pl[0]);
        const uint8_t unitSize = pl[1];
        plSize -= 2;
        pl += 2;

        // Filter Teletext packets.
        if (unitSize <= plSize &&
            unitSize == TELETEXT_PACKET_SIZE &&
            (unitId == TeletextDataUnitId::NON_SUBTITLE || unitId == TeletextDataUnitId::SUBTITLE))
        {
            // Reverse bitwise endianess of each data byte via lookup table, ETS 300 706, chapter 7.1.
            uint8_t pkt[TELETEXT_PACKET_SIZE];
            for (int i = 0; i < unitSize; ++i) {
                pkt[i] = REVERSE_8[pl[i]];
            }
            processTeletextPacket(pid, pc, unitId, pkt);
        }

        // Point to next data unit.
        plSize -= unitSize;
        pl += unitSize;
    }
}


//-----------------------------------------------------------------------------
// Process one Teletext packet.
//-----------------------------------------------------------------------------

void ts::TeletextDemux::processTeletextPacket(PID pid, PIDContext& pc, TeletextDataUnitId dataUnitId, const uint8_t* pkt)
{
    // Structure of a Teletext packet. See ETSI 300 706, section 7.1.
    // - Clock run-in: 1 byte
    // - Framing code: 1 byte
    // - Address: 2 bytes
    // - Data: 40 bytes

    // Variable names conform to ETS 300 706, chapter 7.1.2.
    uint8_t address = uint8_t(unham_8_4(pkt[3]) << 4) | unham_8_4(pkt[2]);
    uint8_t m = address & 0x07;
    if (m == 0) {
        m = 8;
    }
    uint8_t y = (address >> 3) & 0x1F;
    const uint8_t* data = pkt + 4;
    uint8_t designationCode = (y > 25) ? unham_8_4(data[0]) : 0x00;

    if (y == 0) {
        // Page number and control bits
        uint16_t pageNumber = uint16_t(uint16_t(m) << 8) | uint16_t(uint16_t(unham_8_4(data[1])) << 4) | unham_8_4(data[0]);
        uint8_t charset = ((unham_8_4(data[7]) & 0x08) | (unham_8_4(data[7]) & 0x04) | (unham_8_4(data[7]) & 0x02)) >> 1;

        // ETS 300 706, chapter 9.3.1.3:
        //
        // When set to '1' the service is designated to be in Serial mode and the transmission
        // of a page is terminated by the next page header with a different page number.
        // When set to '0' the service is designated to be in Parallel mode and the transmission
        // of a page is terminated by the next page header with a different page number but the
        // same magazine number.
        //
        // The same setting shall be used for all page headers in the service.
        //
        // ETS 300 706, chapter 7.2.1: Page is terminated by and excludes the next page header packet
        // having the same magazine address in parallel transmission mode, or any magazine address in
        // serial transmission mode.
        //
        pc.transMode = TransMode(unham_8_4(data[7]) & 0x01);

        // FIXME: Well, this is not ETS 300 706 kosher, however we are interested in TELETEXT_SUBTITLE only
        if (pc.transMode == TRANSMODE_PARALLEL && dataUnitId != TeletextDataUnitId::SUBTITLE) {
            return;
        }

        if (pc.receivingData &&
            ((pc.transMode == TRANSMODE_SERIAL && pageOf(pageNumber) != pageOf(pc.currentPage)) ||
             (pc.transMode == TRANSMODE_PARALLEL && pageOf(pageNumber) != pageOf(pc.currentPage) && m == magazineOf(pc.currentPage))))
        {
            pc.receivingData = false;
        }

        // A new frame starts on a page. If this page had a non-empty frame in progress, flush it now.
        TeletextPage& page(pc.pages[pageNumber]);
        if (page.tainted) {
            // It would not be nice if subtitle hides previous video frame, so we contract 40 ms (1 frame @25 fps)
            page.hideTimestamp = pidDuration(pid) - 40;
            processTeletextPage(pid, pc, pageNumber);
        }

        // Start new page.
        pc.currentPage = pageNumber;
        page.reset(pidDuration(pid));
        page.charset.resetX28(charset);
        pc.receivingData = true;
    }
    else if (m == magazineOf(pc.currentPage) && y >= 1 && y <= 23 && pc.receivingData) {
        // ETS 300 706, chapter 9.4.1: Packets X/26 at presentation Levels 1.5, 2.5, 3.5 are used for addressing
        // a character location and overwriting the existing character defined on the Level 1 page
        // ETS 300 706, annex B.2.2: Packets with Y = 26 shall be transmitted before any packets with Y = 1 to Y = 25;
        // so page_buffer.text[y][i] may already contain any character received
        // in frame number 26, skip original G0 character
        TeletextPage& page(pc.pages[pc.currentPage]);
        for (uint8_t i = 0; i < 40; i++) {
            if (page.text[y][i] == 0x00) {
                page.text[y][i] = page.charset.teletextToUcs2(data[i]);
            }
        }
        page.tainted = true;
    }
    else if (m == magazineOf(pc.currentPage) && y == 26 && pc.receivingData) {
        // ETS 300 706, chapter 12.3.2: X/26 definition
        uint32_t x26Row = 0;
        uint32_t x26Col = 0;

        uint32_t triplets[13] = { 0 };
        for (uint8_t i = 1, j = 0; i < 40; i += 3, j++) {
            triplets[j] = unham_24_18((data[i + 2] << 16) | (data[i + 1] << 8) | data[i]);
        }

        for (uint8_t j = 0; j < 13; j++) {
            if (triplets[j] == 0xffffffff) {
                // invalid data (HAM24/18 uncorrectable error detected), skip group
                continue;
            }

            const uint8_t tdata = uint8_t((triplets[j] & 0x3f800) >> 11);
            const uint8_t tmode = uint8_t((triplets[j] & 0x7c0) >> 6);
            const uint8_t taddr = uint8_t(triplets[j] & 0x3f);
            const bool rowAddressGroup = (taddr >= 40) && (taddr <= 63);

            TeletextPage& page(pc.pages[pc.currentPage]);

            // ETS 300 706, chapter 12.3.1, table 27: set active position
            if (tmode == 0x04 && rowAddressGroup) {
                x26Row = taddr - 40;
                if (x26Row == 0) {
                    x26Row = 24;
                }
                x26Col = 0;
            }

            // ETS 300 706, chapter 12.3.1, table 27: termination marker
            if (tmode >= 0x11 && tmode <= 0x1f && rowAddressGroup) {
                break;
            }

            // ETS 300 706, chapter 12.3.1, table 27: character from G2 set
            if (tmode == 0x0f && !rowAddressGroup) {
                x26Col = taddr;
                if (tdata > 31) {
                    page.text[x26Row][x26Col] = page.charset.g2ToUcs2(tdata);
                }
            }

            // ETS 300 706, chapter 12.3.1, table 27: G0 character with diacritical mark
            if (tmode >= 0x11 && tmode <= 0x1f && !rowAddressGroup) {
                x26Col = taddr;
                page.text[x26Row][x26Col] = page.charset.g2AccentToUcs2(tdata, tmode - 0x11);
            }
        }
    }
    else if (m == magazineOf(pc.currentPage) && y == 28 && pc.receivingData) {
        // TODO:
        //   ETS 300 706, chapter 9.4.7: Packet X/28/4
        //   Where packets 28/0 and 28/4 are both transmitted as part of a page, packet 28/0 takes precedence over 28/4 for all but the colour map entry coding.
        if (designationCode == 0 || designationCode == 4) {
            // ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1
            // ETS 300 706, chapter 9.4.7: Packet X/28/4
            const uint32_t triplet0 = unham_24_18((data[3] << 16) | (data[2] << 8) | data[1]);
            // ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1 only
            if ((triplet0 & 0x0f) == 0x00) {
                pc.pages[pc.currentPage].charset.setG0Charset(triplet0);
                pc.pages[pc.currentPage].charset.setX28(uint8_t((triplet0 & 0x3f80) >> 7));
            }
        }
    }
    else if (m == magazineOf(pc.currentPage) && y == 29) {
        // TODO:
        //   ETS 300 706, chapter 9.5.1 Packet M/29/0
        //   Where M/29/0 and M/29/4 are transmitted for the same magazine, M/29/0 takes precedence over M/29/4.
        if (designationCode == 0 || designationCode == 4) {
            // ETS 300 706, chapter 9.5.1: Packet M/29/0
            // ETS 300 706, chapter 9.5.3: Packet M/29/4
            const uint32_t triplet0 = unham_24_18((data[3] << 16) | (data[2] << 8) | data[1]);
            // ETS 300 706, table 11: Coding of Packet M/29/0
            // ETS 300 706, table 13: Coding of Packet M/29/4
            if ((triplet0 & 0xff) == 0x00) {
                pc.pages[pc.currentPage].charset.setG0Charset(triplet0);
                pc.pages[pc.currentPage].charset.setM29(uint8_t((triplet0 & 0x3f80) >> 7));
            }
        }
    }
    else if ((m == 8) && (y == 30)) {
        // ETS 300 706, chapter 9.8: Broadcast Service Data Packets.
        // We can find here "Programme Identification Data" and absolute data / time stamps.
        // It is not interesting for us.
    }
}


//-----------------------------------------------------------------------------
// Process one Teletext page.
//-----------------------------------------------------------------------------

void ts::TeletextDemux::processTeletextPage(PID pid, PIDContext& pc, int pageNumber)
{
    // Reference to the page content.
    TeletextPage& page(pc.pages[pageNumber]);

    // Optimization: slicing column by column -- higher probability we could find boxed area start mark sooner
    bool pageIsEmpty = true;
    for (int col = 0; pageIsEmpty && col < 40; col++) {
        for (int row = 1; pageIsEmpty && row < 25; row++) {
            if (page.text[row][col] == 0x0B) {
                pageIsEmpty = false;
            }
        }
    }
    if (pageIsEmpty) {
        return;
    }

    // Adjust frame count and timestamps.
    page.frameCount++;
    if (page.showTimestamp > page.hideTimestamp) {
        page.hideTimestamp = page.showTimestamp;
    }

    // Prepare the Teletext frame.
    TeletextFrame frame(pid, pageBcdToBinary(pageNumber), page.frameCount, page.showTimestamp, page.hideTimestamp);

    // Process page data.
    for (int row = 1; row < 25; row++) {
        UString line;

        // Anchors for string trimming purpose
        int colStart = 40;
        int colStop = 40;

        for (int col = 39; col >= 0; col--) {
            if (page.text[row][col] == 0x0B) {
                colStart = col;
                break;
            }
        }
        if (colStart > 39) {
            // Line is empty
            continue;
        }

        for (int col = colStart + 1; col <= 39; col++) {
            if (page.text[row][col] > 0x20) {
                if (colStop > 39) {
                    colStart = col;
                }
                colStop = col;
            }
            if (page.text[row][col] == 0x0A) {
                break;
            }
        }
        if (colStop > 39) {
            // Line is empty
            continue;
        }

        // ETS 300 706, chapter 12.2: Alpha White ("Set-After") - Start-of-row default condition.
        // used for colour changes _before_ start box mark
        // white is default as stated in ETS 300 706, chapter 12.2
        // black(0), red(1), green(2), yellow(3), blue(4), magenta(5), cyan(6), white(7)
        uint16_t foregroundColor = 0x07;
        bool fontTagOpened = false;

        for (int col = 0; col <= colStop; col++) {
            // v is just a shortcut
            UChar v = page.text[row][col];

            if (col < colStart) {
                if (v <= 0x7) {
                    foregroundColor = v;
                }
            }

            if (col == colStart) {
                if (foregroundColor != 0x7 && _addColors) {
                    line.append(u"<font color=\"");
                    line.append(TELETEXT_COLORS[foregroundColor]);
                    line.append(u"\">");
                    fontTagOpened = true;
                }
            }

            if (col >= colStart) {
                if (v <= 0x7) {
                    // ETS 300 706, chapter 12.2: Unless operating in "Hold Mosaics" mode,
                    // each character space occupied by a spacing attribute is displayed as a SPACE.
                    if (_addColors) {
                        if (fontTagOpened) {
                            line.append(u"</font> ");
                            fontTagOpened = false;
                        }

                        // <font/> tags only when needed
                        if (v > 0x00 && v < 0x07) {
                            line.append(u"<font color=\"");
                            line.append(TELETEXT_COLORS[v]);
                            line.append(u"\">");
                            fontTagOpened = true;
                        }
                    }
                    else {
                        v = 0x20;
                    }
                }

                // Translate some chars into entities, if in colour mode, to replace unsafe HTML tag chars
                if (v >= 0x20 && _addColors) {
                    struct HtmlEntity {
                        UChar character;
                        const UChar* entity;
                    };
                    static const HtmlEntity entities[] = {
                        {u'<', u"&lt;"},
                        {u'>', u"&gt;"},
                        {u'&', u"&amp;"},
                        {0, nullptr}
                    };
                    for (const HtmlEntity* p = entities; p->entity != nullptr; ++p) {
                        if (v == p->character) {
                            line.append(p->entity);
                            v = 0;  // v < 0x20 won't be printed in next block
                            break;
                        }
                    }
                }

                if (v >= 0x20) {
                    line.append(v);
                }
            }
        }

        // No tag will be left opened!
        if (_addColors && fontTagOpened) {
            line.append(u"</font>");
            fontTagOpened = false;
        }

        // Line is now complete.
        frame.addLine(line);
    }

    // Now call the user-specified handler.
    // Note that the super class PESDemux has already placed us in "handler context".
    if (_txtHandler != nullptr) {
        _txtHandler->handleTeletextMessage(*this, frame);
    }
}


//-----------------------------------------------------------------------------
// Flush any pending Teletext message.
//-----------------------------------------------------------------------------

void ts::TeletextDemux::flushTeletext()
{
    for (auto& itPid : _pids) {
        for (auto& itPage : itPid.second.pages) {
            if (itPage.second.tainted) {
                // Use the last timestamp (ms) for end of message.
                const MilliSecond ms = pidDuration(itPid.first);

                // This time, we do not subtract any frames, there will be no more frames.
                itPage.second.hideTimestamp = ms;

                beforeCallingHandler();
                try {
                    processTeletextPage(itPid.first, itPid.second, itPage.first);
                }
                catch (...) {
                    afterCallingHandler(false);
                    throw;
                }
                afterCallingHandler(true);

                itPage.second.reset(ms);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Get the number of Teletext frames found in a given page.
//-----------------------------------------------------------------------------

int ts::TeletextDemux::frameCount(int page, PID pid) const
{
    // Internally, Teletext page numbers are stored in Binary-Coded Decimal.
    const int bcdPage = pageBinaryToBcd(page);

    if (pid != PID_NULL) {
        const auto itPid = _pids.find(pid);
        if (itPid != _pids.end()) {
            const auto itPage = itPid->second.pages.find(bcdPage);
            return itPage == itPid->second.pages.end() ? 0 : itPage->second.frameCount;
        }
    }
    else {
        for (const auto& itPid : _pids) {
            const auto itPage = itPid.second.pages.find(bcdPage);
            if (itPage != itPid.second.pages.end() && itPage->second.frameCount > 0) {
                return itPage->second.frameCount;
            }
        }
    }
    return 0;
}
