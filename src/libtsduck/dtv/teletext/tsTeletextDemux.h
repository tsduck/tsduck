//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Extract Teletext subtitles from TS packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPESDemux.h"
#include "tsTeletext.h"
#include "tsTeletextCharset.h"
#include "tsTeletextHandlerInterface.h"

namespace ts {
    //!
    //! This class extracts Teletext subtitles from TS packets.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TeletextDemux : public PESDemux
    {
        TS_NOBUILD_NOCOPY(TeletextDemux);
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef PESDemux SuperClass;

        //!
        //! Constructor
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] handler User handler for Teletext notification.
        //! @param [in] pids The set of PID's to demux.
        //!
        explicit TeletextDemux(DuckContext& duck, TeletextHandlerInterface* handler = nullptr, const PIDSet& pids = NoPID);

        //!
        //! Destructor.
        //!
        virtual ~TeletextDemux() override;

        //!
        //! Activate or deactivate the font color tags in the output text.
        //! @param [in] addColors If true, font colors tags will be inserted in the output text.
        //!
        void setAddColors(bool addColors)
        {
            _addColors = addColors;
        }

        //!
        //! Check if font colors tags are inserted in the output text.
        //! @return True when font colors tags are inserted in the output text.
        //!
        bool getAddColors() const
        {
            return _addColors;
        }

        //!
        //! Flush any pending Teletext message.
        //! Useful only after receiving the last packet in the stream.
        //! Implicitly called by destructor.
        //!
        void flushTeletext();

        //!
        //! Replace the Teletext handler.
        //! @param [in] handler The Teletext handler.
        //!
        void setTeletextHandler(TeletextHandlerInterface* handler)
        {
            _txtHandler = handler;
        }

        //!
        //! Get the number of Teletext frames found in a given page.
        //! @param [in] page Teletext page number.
        //! @param [in] pid Teletext PID. If omitted, use the first PID containing frames from the specified @a page.
        //! @return Number of Teletext frames found so far on @a page.
        //!
        int frameCount(int page, PID pid = PID_NULL) const;

    protected:
        // Inherited methods
        virtual void immediateReset() override;
        virtual void immediateResetPID(PID pid) override;
        virtual void handlePESPacket(const PESPacket& packet) override;

    private:
        //!
        //! Teletext transmission mode.
        //! Don't change values, they must match the binary format.
        //!
        enum TransMode {
            TRANSMODE_PARALLEL = 0,  //!< Parallel mode.
            TRANSMODE_SERIAL   = 1   //!< Serial mode.
        };

        //!
        //! Structure of a Teletext page.
        //!
        class TeletextPage
        {
        public:
            uint32_t        frameCount;    //!< Number of produced frames in this page.
            MilliSecond     showTimestamp; //!< Show at timestamp (in ms).
            MilliSecond     hideTimestamp; //!< Hide at timestamp (in ms).
            bool            tainted;       //!< True if text variable contains any data.
            TeletextCharset charset;       //!< Charset to use.
            UChar           text[25][40];  //!< 25 lines x 40 cols (1 screen/page) of wide chars.
            //!
            //! Default constructor.
            //!
            TeletextPage();
            //!
            //! Reset to a new page with a new starting timestamp.
            //! @param [in] timestamp New starting timestamp.
            //!
            void reset(MilliSecond timestamp);
        };

        //!
        //! Map of TeletextPage, indexed by page number.
        //!
        typedef std::map<int,TeletextPage> TeletextPageMap;

        //!
        //! This internal structure contains the analysis context for one PID.
        //!
        class PIDContext
        {
        public:
            bool            receivingData;  //!< Incoming data should be processed or ignored.
            TransMode       transMode;      //!< Teletext transmission mode.
            int             currentPage;    //!< Current Teletext page number.
            TeletextPageMap pages;          //!< Working Teletext page buffers, indexed by page number.
            //!
            //! Default constructor.
            //!
            PIDContext();
        };

        //!
        //! Map of PID analysis contexts, indexed by PID value.
        //!
        typedef std::map<PID, PIDContext> PIDContextMap;

        //!
        //! Process one Teletext packet.
        //! @param [in] pid PID number.
        //! @param [in,out] pc PID context.
        //! @param [in] dataUnitId Teletext packet data unit id.
        //! @param [in] pkt Address of Teletext packet (44 bytes, TELETEXT_PACKET_SIZE).
        //!
        void processTeletextPacket(PID pid, PIDContext& pc, TeletextDataUnitId dataUnitId, const uint8_t* pkt);

        //!
        //! Process one Teletext page.
        //! @param [in] pid PID number.
        //! @param [in,out] pc PID context.
        //! @param [in] pageNumber Page number.
        //!
        void processTeletextPage(PID pid, PIDContext& pc, int pageNumber);

        //!
        //! Remove 8/4 Hamming code.
        //! @param [in] a Hamming-encoded byte.
        //! @return Decoded byte.
        //! @see ETSI 300 706, section 8.2.
        //!
        static uint8_t unham_8_4(uint8_t a);

        //!
        //! Remove 24/18 Hamming code.
        //! @param [in] a Hamming-encoded word.
        //! @return Decoded word.
        //! @see ETSI 300 706, section 8.3.
        //!
        static uint32_t unham_24_18(uint32_t a);

        //!
        //! Extract Teletext magazine number from Teletext page.
        //! @param [in] page Teletext page.
        //! @return The Teletext magazine number.
        //!
        static int magazineOf(int page)
        {
            return (page >> 8) & 0x0F;
        }

        //!
        //! Extract Teletext page number from Teletext page.
        //! @param [in] page Teletext page.
        //! @return The Teletext page number.
        //!
        static int pageOf(int page)
        {
            return page & 0xFF;
        }

        //!
        //! Converts a page number from BCD to binary.
        //! Teletext page numbers are stored in Binary-Coded Decimal.
        //! @param [in] bcd BCD page number.
        //! @return Binary page number.
        //!
        static int pageBcdToBinary(int bcd);

        //!
        //! Converts a page number from binary to BCD.
        //! Teletext page numbers are stored in Binary-Coded Decimal.
        //! @param [in] bin Binary page number.
        //! @return BCD page number.
        //!
        static int pageBinaryToBcd(int bin);

        // Private members:
        TeletextHandlerInterface* _txtHandler;    //!< User handler.
        PIDContextMap             _pids;          //!< Map of PID analysis contexts.
        bool                      _addColors;     //!< Add font color tags.
    };
}
