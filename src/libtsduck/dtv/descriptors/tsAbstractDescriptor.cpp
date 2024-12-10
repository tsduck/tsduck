//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractDescriptor.h"
#include "tsAbstractTable.h"
#include "tsDescriptor.h"
#include "tsDescriptorList.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsNames.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::AbstractDescriptor::AbstractDescriptor(DID tag, const UChar* xml_name, Standards standards, PDS pds, const UChar* xml_legacy_name) :
    AbstractSignalization(xml_name, standards, xml_legacy_name),
    _tag(tag),
    _required_pds(pds)
{
}

ts::AbstractDescriptor::~AbstractDescriptor()
{
}


//----------------------------------------------------------------------------
// What to do when a descriptor of the same type is added twice in a list.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::AbstractDescriptor::duplicationMode() const
{
    // By default, descriptors are added to the descriptor list.
    return DescriptorDuplication::ADD_ALWAYS;
}

bool ts::AbstractDescriptor::merge(const AbstractDescriptor& desc)
{
    // By default, merging descriptors is not implemented.
    return false;
}


//----------------------------------------------------------------------------
// Get the extended descriptor id.
//----------------------------------------------------------------------------

ts::EDID ts::AbstractDescriptor::edid(const AbstractTable* table) const
{
    const auto& repo(PSIRepository::Instance());
    const auto index = std::type_index(typeid(*this));
    return table == nullptr ? repo.getDescriptor(index).edid : repo.getDescriptor(index, table->tableId(), table->definingStandards()).edid;
}

ts::EDID ts::AbstractDescriptor::edid(TID tid) const
{
    return PSIRepository::Instance().getDescriptor(std::type_index(typeid(*this)), tid).edid;
}


//----------------------------------------------------------------------------
// Get the extended descriptor tag (first byte in payload).
//----------------------------------------------------------------------------

ts::DID ts::AbstractDescriptor::extendedTag() const
{
    return PSIRepository::Instance().getDescriptor(std::type_index(typeid(*this))).edid.didExtension();
}


//----------------------------------------------------------------------------
// Descriptor serialization.
//----------------------------------------------------------------------------

bool ts::AbstractDescriptor::serialize(DuckContext& duck, Descriptor& bin) const
{
    if (!isValid()) {
        // The descriptor is already invalid.
        bin.invalidate();
        return false;
    }
    else {
        // Allocate a byte block of the maximum descriptor size.
        ByteBlockPtr bbp(new ByteBlock(MAX_DESCRIPTOR_SIZE));
        CheckNonNull(bbp.get());

        // Map a serialization buffer over the payload part.
        PSIBuffer buf(duck, bbp->data() + 2, bbp->size() - 2, false);

        // If this is an extension descriptor, add extended tag.
        const DID etag = extendedTag();
        if (etag != XDID_NULL) {
            buf.putUInt8(etag);
        }

        // Let the subclass serialize the payload in the buffer.
        serializePayload(buf);

        if (buf.error()) {
            // Serialization error, not a valid descriptor.
            bin.invalidate();
            return false;
        }
        else {
            // Update the actual descriptor size.
            const size_t size = buf.currentWriteByteOffset();
            (*bbp)[0] = _tag;
            (*bbp)[1] = uint8_t(size);

            // Resize the byte block and store it into the descriptor.
            bbp->resize(2 + size);
            bin = Descriptor(bbp, ShareMode::SHARE);
            return true;
        }
    }
}


//----------------------------------------------------------------------------
// Descriptor deserialization.
//----------------------------------------------------------------------------

bool ts::AbstractDescriptor::deserialize(DuckContext& duck, const Descriptor& bin)
{
    // Make sure the object is cleared before analyzing the binary descriptor.
    clear();

    if (!bin.isValid() || bin.tag() != _tag) {
        // If the binary descriptor is already invalid or has the wrong descriptor tag, this object is invalid too.
        invalidate();
        return false;
    }
    else {
        // Map a deserialization read-only buffer over the payload part.
        PSIBuffer buf(duck, bin.payload(), bin.payloadSize());

        // If this is an extension descriptor, check that the expected extended tag is present in the payload.
        const DID etag = extendedTag();
        if (etag != XDID_NULL && (buf.getUInt8() != etag || buf.error())) {
            invalidate();
            return false;
        }

        // Let the subclass deserialize the payload in the buffer.
        deserializePayload(buf);

        if (buf.error() || !buf.endOfRead()) {
            // Deserialization error or extraneous data, not a valid descriptor.
            clear();
            invalidate();
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Deserialize from a descriptor list.
//----------------------------------------------------------------------------

bool ts::AbstractDescriptor::deserialize(DuckContext& duck, const DescriptorList& dlist, size_t index)
{
    if (index > dlist.count()) {
        invalidate();
        return false;
    }
    else {
        return deserialize(duck, *dlist[index]);
    }
}
