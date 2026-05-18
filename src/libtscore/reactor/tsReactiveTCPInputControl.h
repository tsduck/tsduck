//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Input data control for ReactiveTCPConnection.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! The class describes input data, as provided by a ReactiveTCPConnection to a ReactiveTCPConnectionHandlerInterface.
    //! The handler updates the ReactiveTCPInputControl structure depending on its processing of the input data.
    //!
    //! Sample scenario for ReactiveTCPConnectionHandlerInterface::handleTCPReceive(), using a message format consisting
    //! of a message header, containing the payload size, followed by a payload of that size.
    //!
    //! - If @a data size is less than the header size, set @a used_size to zero, set @a min_next_size to the
    //!   expected header size, and return.
    //! - Otherwise, read the payload size in the header, check if the complete message, header and payload, is
    //!   in the @a data buffer. If not, set @a used_size to zero, set @a min_next_size to the total message size,
    //!   header and payload, then return.
    //! - Otherwise, process the message, set @a used_size to the total size of the processed message, set
    //!   @a min_next_size to the expected header size (for next message), and return.
    //!
    //! Sample scenario for ReactiveTCPConnectionHandlerInterface::handleTCPReceive(), using a text line format where
    //! each line is terminated by LF or a CR/LF combination.
    //!
    //! - Check if @a data contains a LF byte. If not, set @a used_size to zero, set @a min_next_size to @a NPOS,
    //!   set @a next_delimiter to LF, and return.
    //! - Otherwise, process the text line up to and including the LF byte, set @a used_size to the processed line
    //!   size, including the LF, set @a min_next_size to @a NPOS, set @a next_delimiter to LF, and return.
    //!
    //! In all cases, the handler shall check @a error_code and verify that it is equal to SYS_SUCCESS before proceeding.
    //! Otherwise, the connection shall be considered as lost or terminated.
    //!
    class TSCOREDLL ReactiveTCPInputControl
    {
    public:
        //!
        //! Default constructor.
        //!
        ReactiveTCPInputControl() = default;
        //!
        //! On return, the handler sets the number of bytes which were used in @a used_size. If the returned value
        //! of @a used_size is less than the input data size, the remaining unused data stay in the input buffer
        //! and will be present at the beginning of the @a data buffer the next time this handler will be called.
        //!
        //! On input, @a used_size is set to some value equal to or greater than the input data size, meaning that
        //! it is assumed that then handler uses the entire buffer. This means that @a used_size needs to be updated
        //! only if the handler doesn't use all input data in the buffer.
        //!
        size_t used_size = NPOS;
        //!
        //! On return, the handler can specify in @a min_next_size the minimum data size to receive before being
        //! called again. This is the minimum size of the data buffer, including remaining unused data when
        //! @a used_size is set to a value lower that @a data size. This means that if @a min_next_size is less than
        //! the remaining unused bytes in the buffer, the handler will be immediately called again on return.
        //!
        //! On input, @a min_next_size is set to zero, meaning that the handler will be called when any amount of data
        //! is available, including unused bytes if @a used_size is less than @a data size.
        //!
        size_t min_next_size = 0;
        //!
        //! If @a next_min_size is set to @a NPOS, the returned byte value in @a next_delimiter is used as a delimiter
        //! for the next message. This means that the handler will be called again only when input data contains the
        //! corresponding byte value, including unused bytes if @a used_size is less than @a data size.
        //!
        uint8_t next_delimiter = 0;
    };
}
