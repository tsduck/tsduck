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


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX, class COMPARE>
ts::MessagePriorityQueue<MSG, MUTEX, COMPARE>::MessagePriorityQueue(size_t maxMessages) :
    SuperClass(maxMessages)
{
}


//----------------------------------------------------------------------------
// Placement in the message queue (virtual protected methods).
//----------------------------------------------------------------------------

template <typename MSG, class MUTEX, class COMPARE>
typename ts::MessagePriorityQueue<MSG, MUTEX, COMPARE>::SuperClass::MessageList::iterator
    ts::MessagePriorityQueue<MSG, MUTEX, COMPARE>::enqueuePlacement(const typename SuperClass::MessagePtr& msg, typename SuperClass::MessageList& list)
{
    auto loc = list.end();

    // Null pointers are stored at end (anywhere else would be probably fine).
    if (msg.isNull()) {
        return loc;
    }

    // Loop until the previous element is lower that msg.
    while (loc != list.begin()) {
        const auto cur = loc;
        --loc;
        if (!loc->isNull() && !COMPARE()(*msg, **loc)) {
            return cur;
        }
    }

    // Reached begin of list, all elements are greater than msg.
    return loc;
}
