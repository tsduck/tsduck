//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for objects being part of a ring.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    //!
    //! Base class for objects being part of a ring, ie. a double-linked list with no begin or end.
    //! @ingroup cpp
    //!
    //! This class is not thread-safe. Access to all objects which are part of the same ring
    //! shall be synchronized.
    //!
    class TSDUCKDLL RingNode
    {
        TS_NOCOPY(RingNode);
    public:
        //!
        //! Default constructor.
        //!
        RingNode() = default;

        //!
        //! Destructor.
        //!
        virtual ~RingNode();

        //!
        //! Check if the node is alone in its own ring.
        //! @return True if the node is alone in its own ring.
        //!
        bool ringAlone() const { return _ring_next == this; }

        //!
        //! Remove the node from the ring it belongs to and creates its own ring.
        //!
        void ringRemove();

        //!
        //! Insert this object in a ring after the specified node.
        //! @param [in] o A node of a ring. This object is inserted after @a o in the ring.
        //!
        void ringInsertAfter(RingNode* o);

        //!
        //! Insert this object in a ring before the specified node.
        //! @param [in] o A node of a ring. This object is inserted before @a o in the ring.
        //!
        void ringInsertBefore(RingNode* o);

        //!
        //! Swap this object and another one in their rings.
        //! If the two objects belong to the same ring, their positions are swapped.
        //! If they belong to distinct rings, they also move to each other's ring.
        //! @param [in] o The node to swap with.
        //!
        void ringSwap(RingNode* o);

        //!
        //! Get the next node in the ring.
        //! @tparam T A superclass of RingNode, the expected type of the next object in the ring.
        //! @return Address of the next node in the ring or zero if the next node is not a subclass of @a T.
        //!
        template <typename T>
        T* ringNext() { return dynamic_cast<T*>(_ring_next); }

        //!
        //! Get the next node in the ring.
        //! @tparam T A superclass of RingNode, the expected type of the next object in the ring.
        //! @return Address of the next node in the ring or zero if the next node is not a subclass of @a T.
        //!
        template <typename T>
        const T* ringNext() const { return dynamic_cast<const T*>(_ring_next); }

        //!
        //! Get the previous node in the ring.
        //! @tparam T A superclass of RingNode, the expected type of the previous object in the ring.
        //! @return Address of the previous node in the ring or zero if the previous node is not a subclass of @a T.
        //!
        template <typename T>
        T* ringPrevious() { return dynamic_cast<T*>(_ring_previous); }

        //!
        //! Get the previous node in the ring.
        //! @tparam T A superclass of RingNode, the expected type of the previous object in the ring.
        //! @return Address of the previous node in the ring or zero if the previous node is not a subclass of @a T.
        //!
        template <typename T>
        const T* ringPrevious() const { return dynamic_cast<const T*>(_ring_previous); }

        //!
        //! Count the number of element in the ring.
        //! Warning: This method has a linear response time, avoid using it when possible.
        //! @return The number of nodes in the ring.
        //!
        size_t ringSize() const;

    private:
        RingNode* _ring_previous = this;
        RingNode* _ring_next = this;
    };
}
