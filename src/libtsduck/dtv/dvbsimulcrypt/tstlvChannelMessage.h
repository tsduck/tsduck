//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for DVB SimulCrypt TLV messages operating on channels.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvMessage.h"
#include "tstlvMessageFactory.h"

namespace ts {
    namespace tlv {
        //!
        //! Base class for DVB SimulCrypt TLV messages operating on channels.
        //!
        class TSDUCKDLL ChannelMessage : public Message
        {
            TS_RULE_OF_FIVE(ChannelMessage, override);
        protected:
            //!
            //! Alias for the superclass of subclasses.
            //!
            using superclass = ChannelMessage;

        public:
            // Protocol-documented fields:
            uint16_t channel_id = 0;  //!< Channel id.

            //!
            //! Constructor.
            //! @param [in] tag Message tag.
            //! @param [in] ch_id Channel id.
            //!
            ChannelMessage(TAG tag, uint16_t ch_id = 0);

            //!
            //! Constructor.
            //! @param [in] protocol_version Protocol version.
            //! @param [in] tag Message tag.
            //! @param [in] ch_id Channel id.
            //!
            ChannelMessage(VERSION protocol_version, TAG tag, uint16_t ch_id = 0);

            //!
            //! Constructor.
            //! @param [in] fact Message factory containing a binary message.
            //! @param [in] tag_ch_id Message tag for the channel id field.
            //!
            ChannelMessage(const tlv::MessageFactory& fact, TAG tag_ch_id);
        };
    }
}
