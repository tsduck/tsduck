//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSocketOp.h"


//----------------------------------------------------------------------------
// Enumeration descriptions of ts::SocketOp.
//----------------------------------------------------------------------------

const ts::Names& ts::SocketOpNames()
{
    static const Names data {
        {u"none",         SocketOp::NONE},
        {u"open",         SocketOp::OPEN},
        {u"set-option",   SocketOp::SET_OPTION},
        {u"get-option",   SocketOp::GET_OPTION},
        {u"connect",      SocketOp::CONNECT},
        {u"listen",       SocketOp::LISTEN},
        {u"accept",       SocketOp::ACCEPT},
        {u"send",         SocketOp::SEND},
        {u"receive",      SocketOp::RECEIVE},
        {u"close-writer", SocketOp::CLOSE_WRITER},
        {u"disconnect",   SocketOp::DISCONNECT},
        {u"close",        SocketOp::CLOSE},

    };
    return data;
}
