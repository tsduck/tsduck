//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Name resolver for BIOP objects in a DSM-CC Object Carousel.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDSMCCBIOPMessage.h"
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace ts {
    //!
    //! Resolves the absolute path of BIOP objects within a DSM-CC Object Carousel
    //! by tracking the parent/child graph established by ServiceGateway and Directory
    //! bindings. Buffers BIOP messages whose parent has not yet been seen and emits
    //! them once their name becomes resolvable.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL BIOPNameResolver
    {
        TS_NOCOPY(BIOPNameResolver);

    public:
        //!
        //! Identifies an object inside the carousel: (module_id, object_key).
        //!
        using NameKey = std::pair<uint16_t, ByteBlock>;

        //!
        //! Callback invoked once a BIOP message is ready to be reported.
        //! Parameters: module id, resolved path (empty if unresolved), the message.
        //!
        using EmitCallback = std::function<void(uint16_t module_id, const UString& name, const BIOPMessage& msg)>;

        //!
        //! Default constructor.
        //!
        BIOPNameResolver() = default;

        //!
        //! Reset all internal state.
        //!
        void clear();

        //!
        //! Mark a key as a carousel root (a ServiceGateway).
        //! @param [in] key (module_id, object_key) of the ServiceGateway.
        //!
        void addRoot(const NameKey& key);

        //!
        //! Record the parent->child name links carried by a Directory or ServiceGateway.
        //! @param [in] parent Identifier of the directory/SRG containing the bindings.
        //! @param [in] bindings The bindings to record.
        //!
        void recordBindings(const NameKey& parent, const std::vector<BIOPBinding>& bindings);

        //!
        //! Defer a BIOP message for later emission, in arrival order.
        //! @param [in] module_id Module the message was parsed from.
        //! @param [in] msg The parsed message (ownership transferred).
        //!
        void defer(uint16_t module_id, std::unique_ptr<BIOPMessage> msg);

        //!
        //! Emit every deferred message whose name now resolves. Messages that still
        //! cannot be resolved remain deferred.
        //! @param [in] cb Callback invoked for each emitted message.
        //!
        void drain(const EmitCallback& cb);

        //!
        //! Drain, then emit any still-deferred messages with an empty name.
        //! @param [in] cb Callback invoked for each emitted message.
        //!
        void flush(const EmitCallback& cb);

        //!
        //! Resolve the absolute path of an object.
        //! @param [in] key (module_id, object_key) of the object.
        //! @return Joined path starting with '/', or empty if no chain reaches a root.
        //!
        UString resolveName(const NameKey& key) const;

    private:
        struct ParentLink {
            NameKey parent {};
            UString local_name {};
        };

        struct PendingEmit {
            uint16_t module_id {};
            std::unique_ptr<BIOPMessage> msg {};
        };

        std::map<NameKey, ParentLink> _parent_link {};
        std::set<NameKey> _roots {};
        std::vector<PendingEmit> _pending {};
    };
}  // namespace ts
