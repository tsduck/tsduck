//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRingNode.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::RingNode::~RingNode()
{
    ringRemove();
}


//----------------------------------------------------------------------------
// Insert in a ring after or before the specified node
//----------------------------------------------------------------------------

void ts::RingNode::ringInsertAfter(RingNode* o)
{
    ringRemove();
    _ring_previous = o;
    _ring_next = o->_ring_next;
    _ring_next->_ring_previous = this;
    _ring_previous->_ring_next = this;
}

void ts::RingNode::ringInsertBefore(RingNode* o)
{
    ringRemove();
    _ring_next = o;
    _ring_previous = o->_ring_previous;
    _ring_next->_ring_previous = this;
    _ring_previous->_ring_next = this;
}


//----------------------------------------------------------------------------
// Remove a node from the ring it belongs to and creates its own ring.
//----------------------------------------------------------------------------

void ts::RingNode::ringRemove()
{
    if (_ring_next != this) {
        _ring_next->_ring_previous = _ring_previous;
        _ring_previous->_ring_next = _ring_next;
        _ring_next = _ring_previous = this;
    }
}


//----------------------------------------------------------------------------
// Swap this object and another one in their rings.
//----------------------------------------------------------------------------

void ts::RingNode::ringSwap(RingNode* o)
{
    // If the two objects are identical, do nothing.
    if (this != o) {
        // Save previous and next of current object.
        // They will become previous and next of 'o'.
        // Take care that if an object has previous/next to itself, it is alone.
        RingNode* const next = _ring_next == this ? o : _ring_next;
        RingNode* const previous = _ring_previous == this ? o : _ring_previous;

        // Insert current object in same place as 'o'.
        _ring_next = o->_ring_next == o ? this : o->_ring_next;
        _ring_previous = o->_ring_previous == o ? this : o->_ring_previous;

        // Insert 'o' in same place as this object was.
        o->_ring_next = next;
        o->_ring_previous = previous;

        // Fix previous and next in each ring.
        // It also works when nodes are alone in their ring.
        _ring_next->_ring_previous = this;
        _ring_previous->_ring_next = this;
        o->_ring_next->_ring_previous = o;
        o->_ring_previous->_ring_next = o;
    }
}


//----------------------------------------------------------------------------
// Count the number of element in the rings.
//----------------------------------------------------------------------------

size_t ts::RingNode::ringSize() const
{
    size_t count = 1;
    for (const RingNode* r = this; r->_ring_next != this; r = r->_ring_next) {
        count++;
    }
    return count;
}
