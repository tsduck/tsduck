//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractDescriptor.h"
#include "tsDescriptor.h"
#include "tsDescriptorList.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::AbstractDescriptor::AbstractDescriptor(EDID edid, const UChar* xml_name, const UChar* xml_legacy_name) :
    AbstractSignalization(xml_name, edid.standards(), xml_legacy_name),
    _edid(edid)
{
}

ts::AbstractDescriptor::~AbstractDescriptor()
{
}


//----------------------------------------------------------------------------
// Inherited methods.
//----------------------------------------------------------------------------

ts::Standards ts::AbstractDescriptor::definingStandards(Standards current_standards) const
{
    return _edid.standards();
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
        // Add defining standards of the descriptor to the context.
        duck.addStandards(definingStandards());

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
            (*bbp)[0] = tag();
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

    if (!bin.isValid() || bin.tag() != tag()) {
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
        return deserialize(duck, dlist[index]);
    }
}
