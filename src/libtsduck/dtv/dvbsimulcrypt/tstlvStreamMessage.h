//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for DVB SimulCrypt TLV messages operating on streams.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvChannelMessage.h"

namespace ts {
    namespace tlv {
        //!
        //! Base class for DVB SimulCrypt TLV messages operating on streams.
        //! @ingroup tlv
        //!
        class TSDUCKDLL StreamMessage : public ChannelMessage
        {
            TS_RULE_OF_FIVE(StreamMessage, override);
        protected:
            //!
            //! Alias for the superclass of subclasses.
            //!
            using superclass = StreamMessage;

        public:
            // Protocol-documented fields:
            // uint16_t channel_id;
            uint16_t stream_id = 0;  //!< Stream id.

            //!
            //! Constructor.
            //! @param [in] tag Message tag.
            //! @param [in] ch_id Channel id.
            //! @param [in] st_id Stream id.
            //!
            StreamMessage(TAG tag, uint16_t ch_id = 0, uint16_t st_id = 0);

            //!
            //! Constructor.
            //! @param [in] protocol_version Protocol version.
            //! @param [in] tag Message tag.
            //! @param [in] ch_id Channel id.
            //! @param [in] st_id Stream id.
            //!
            StreamMessage(VERSION protocol_version, TAG tag, uint16_t ch_id = 0, uint16_t st_id = 0);

            //!
            //! Constructor.
            //! @param [in] fact Message factory containing a binary message.
            //! @param [in] tag_ch_id Message tag for the channel id field.
            //! @param [in] tag_st_id Message tag for the stream id field.
            //!
            StreamMessage(const tlv::MessageFactory& fact, TAG tag_ch_id, TAG tag_st_id);
        };
    }
}
