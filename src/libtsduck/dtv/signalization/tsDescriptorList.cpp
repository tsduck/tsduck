//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDescriptorList.h"
#include "tsAbstractDescriptor.h"
#include "tsAbstractTable.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructor and assignment.
//----------------------------------------------------------------------------

ts::DescriptorList::DescriptorList(const AbstractTable* table) :
    _table(table)
{
}

ts::DescriptorList::DescriptorList(const AbstractTable* table, const DescriptorList& dl) :
    _table(table),
    _list(dl._list)
{
}

ts::DescriptorList::DescriptorList(const AbstractTable* table, DescriptorList&& dl) noexcept :
    _table(table),
    _list(std::move(dl._list))
{
}

ts::DescriptorList& ts::DescriptorList::operator=(const DescriptorList& dl)
{
    if (&dl != this) {
        // Copy the list of descriptors but preserve the parent table.
        _list = dl._list;
    }
    return *this;
}

ts::DescriptorList& ts::DescriptorList::operator=(DescriptorList&& dl) noexcept
{
    if (&dl != this) {
        // Move the list of descriptors but preserve the parent table.
        _list = std::move(dl._list);
    }
    return *this;
}


//----------------------------------------------------------------------------
// Get the table id of the parent table.
//----------------------------------------------------------------------------

ts::TID ts::DescriptorList::tableId() const
{
    return _table == nullptr ? TID(TID_NULL) : _table->tableId();
}


//----------------------------------------------------------------------------
// Comparison
//----------------------------------------------------------------------------

bool ts::DescriptorList::operator==(const DescriptorList& other) const
{
    if (_list.size() != other._list.size()) {
        return false;
    }
    for (size_t i = 0; i < _list.size(); ++i) {
        const DescriptorPtr& desc1(_list[i].desc);
        const DescriptorPtr& desc2(other._list[i].desc);
        if (desc1.isNull() || desc2.isNull() || *desc1 != *desc2) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Add one descriptor at end of list
//----------------------------------------------------------------------------

bool ts::DescriptorList::add(const DescriptorPtr& desc)
{
    PDS pds = 0;

    if (desc.isNull() || !desc->isValid()) {
        return false;
    }

    // Determine which PDS to associate with the descriptor
    if (desc->tag() == DID_PRIV_DATA_SPECIF) {
        // This descriptor defines a new "private data specifier".
        // The PDS is the only thing in the descriptor payload.
        pds = desc->payloadSize() < 4 ? 0 : GetUInt32(desc->payload());
    }
    else if (_list.empty()) {
        // First descriptor in the list
        pds = 0;
    }
    else {
        // Use same PDS as previous descriptor
        pds = _list[_list.size()-1].pds;
    }

    // Add the descriptor in the list
    _list.push_back(Element(desc, pds));
    return true;
}


//----------------------------------------------------------------------------
// Add one descriptor at end of list
//----------------------------------------------------------------------------

bool ts::DescriptorList::add(DuckContext& duck, const AbstractDescriptor& desc)
{
    DescriptorPtr pd(new Descriptor);
    CheckNonNull(pd.pointer());
    return desc.serialize(duck, *pd) && add(pd);
}


//----------------------------------------------------------------------------
// Add descriptors from a memory area
//----------------------------------------------------------------------------

bool ts::DescriptorList::add(const void* data, size_t size)
{
    const uint8_t* desc = reinterpret_cast<const uint8_t*>(data);
    size_t length = 0;
    bool success = true;

    while (size >= 2 && (length = size_t(desc[1]) + 2) <= size) {
        success = add(DescriptorPtr(new Descriptor(desc, length))) && success;
        desc += length;
        size -= length;
    }

    return success && size == 0;
}


//----------------------------------------------------------------------------
// Merge one descriptor in the list.
//----------------------------------------------------------------------------

void ts::DescriptorList::merge(DuckContext& duck, const AbstractDescriptor& desc)
{
    // Serialize the new descriptor. In case of error, there is nothing we can add.
    DescriptorPtr bindesc(new Descriptor);
    CheckNonNull(bindesc.pointer());
    desc.serialize(duck, *bindesc);
    if (!bindesc->isValid()) {
        return;
    }

    const PDS pds = desc.requiredPDS();
    const DescriptorDuplication mode = desc.duplicationMode();

    // We need to search for a descriptor of same type only if the duplication mode is not simply ADD_ALWAYS.
    if (mode != DescriptorDuplication::ADD_ALWAYS) {
        const size_t index = search(desc.edid());
        if (index < count()) {
            // A descriptor of same type has been found.
            switch (mode) {
                case DescriptorDuplication::IGNORE: {
                    // New descriptor shall be ignored.
                    return;
                }
                case DescriptorDuplication::REPLACE: {
                    // New descriptor shall replace the previous one.
                    _list[index].desc = bindesc;
                    return;
                }
                case DescriptorDuplication::MERGE: {
                    // New descriptor shall be merged into old one.
                    // We need to deserialize the previous descriptor first.
                    const AbstractDescriptorPtr dp(_list[index].desc->deserialize(duck, pds, _table));
                    if (!dp.isNull() && dp->merge(desc)) {
                        // Descriptor successfully merged. Reserialize it and replace it.
                        DescriptorPtr newdesc(new Descriptor);
                        CheckNonNull(newdesc.pointer());
                        dp->serialize(duck, *newdesc);
                        if (newdesc->isValid()) {
                            _list[index].desc = newdesc;
                            return;
                        }
                    }
                    // In case of merge failure, apply default processing later.
                    break;
                }
                case DescriptorDuplication::ADD_OTHER: {
                    // In case the two binary descriptors are exactly identical, do nothing.
                    if (*_list[index].desc == *bindesc) {
                        return;
                    }
                    break;
                }
                case DescriptorDuplication::ADD_ALWAYS:
                default: {
                    // Default processing by the end of this method.
                    break;
                }
            }
        }
    }

    // The default action is to add the descriptor in the list.
    // Insert a private_data_specifier_descriptor is necessary.
    addPrivateDataSpecifier(pds);
    add(bindesc);
}



//----------------------------------------------------------------------------
// Merge another descriptor list in this list.
//----------------------------------------------------------------------------

void ts::DescriptorList::merge(DuckContext& duck, const DescriptorList& other)
{
    if (&other != this) {
        for (size_t index = 0; index < other._list.size(); ++index) {
            // The descriptor from the other list must be deserialized to be merged.
            const AbstractDescriptorPtr dp(other._list[index].desc->deserialize(duck, other._list[index].pds, other._table));
            if (dp.isNull() || dp->duplicationMode() == DescriptorDuplication::ADD_ALWAYS) {
                // Cannot be deserialized or simply add the descriptor.
                addPrivateDataSpecifier(other._list[index].pds);
                add(other._list[index].desc);
            }
            else {
                // Merge the descriptor.
                merge(duck, *dp);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Get a reference to the descriptor at a specified index.
//----------------------------------------------------------------------------

const ts::DescriptorPtr& ts::DescriptorList::operator[](size_t index) const
{
    assert(index < _list.size());
    return _list[index].desc;
}


//----------------------------------------------------------------------------
// Get the extended descriptor id of a descriptor in the list.
//----------------------------------------------------------------------------

ts::EDID ts::DescriptorList::edid(size_t index) const
{
    // Eliminate invalid descriptor, index out of range.
    if (index >= _list.size() || _list[index].desc.isNull() || !_list[index].desc->isValid()) {
        return EDID(); // invalid value
    }
    else {
        return _list[index].desc->edid(_list[index].pds, _table);
    }
}


//----------------------------------------------------------------------------
// Return the "private data specifier" associated to a descriptor in the list.
//----------------------------------------------------------------------------

ts::PDS ts::DescriptorList::privateDataSpecifier(size_t index) const
{
    return index < _list.size() ? _list[index].pds : PDS_NULL;
}


//----------------------------------------------------------------------------
// Prepare removal of a private_data_specifier descriptor.
//----------------------------------------------------------------------------

bool ts::DescriptorList::prepareRemovePDS(ElementVector::iterator it)
{
    // Eliminate invalid cases
    if (it == _list.end() || it->desc->tag() != DID_PRIV_DATA_SPECIF) {
        return false;
    }

    // Search for private descriptors ahead.
    decltype(it) end;
    for (end = it + 1; end != _list.end(); ++end) {
        DID tag = end->desc->tag();
        if (tag >= 0x80) {
            // This is a private descriptor, the private_data_specifier descriptor
            // is necessary and cannot be removed.
            return false;
        }
        if (tag == DID_PRIV_DATA_SPECIF) {
            // Found another private_data_specifier descriptor with no private
            // descriptor between the two => the first one can be removed.
            break;
        }
    }

    // Update the current PDS after removed private_data_specifier descriptor
    uint32_t previous_pds = it == _list.begin() ? 0 : (it-1)->pds;
    while (--end != it) {
        end->pds = previous_pds;
    }

    return true;
}


//----------------------------------------------------------------------------
// Add a private_data_specifier if necessary at end of list
//----------------------------------------------------------------------------

void ts::DescriptorList::addPrivateDataSpecifier(PDS pds)
{
    if (pds != 0 && (_list.size() == 0 || _list[_list.size() - 1].pds != pds)) {
        // Build a private_data_specifier_descriptor
        uint8_t data[6];
        data[0] = DID_PRIV_DATA_SPECIF;
        data[1] = 4;
        PutUInt32(data + 2, pds);
        add(DescriptorPtr(new Descriptor(data, sizeof(data))));
    }
}


//----------------------------------------------------------------------------
// Remove all private descriptors without preceding
// private_data_specifier_descriptor.
// Return the number of removed descriptors.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::removeInvalidPrivateDescriptors()
{
    size_t count = 0;

    for (size_t n = 0; n < _list.size(); ) {
        if (_list[n].pds == 0 && !_list[n].desc.isNull() && _list[n].desc->isValid() && _list[n].desc->tag() >= 0x80) {
            _list.erase(_list.begin() + n);
            count++;
        }
        else {
            n++;
        }
    }

    return count;
}


//----------------------------------------------------------------------------
// Remove the descriptor at the specified index in the list.
//----------------------------------------------------------------------------

bool ts::DescriptorList::removeByIndex(size_t index)
{
    // Check index validity
    if (index >= _list.size()) {
        return false;
    }

    // Private_data_specifier descriptor can be removed under certain conditions only
    if (_list[index].desc->tag() == DID_PRIV_DATA_SPECIF && !prepareRemovePDS(_list.begin() + index)) {
        return false;
    }

    // Remove the specified descriptor
    _list.erase(_list.begin() + index);
    return true;
}


//----------------------------------------------------------------------------
// Remove all descriptors with the specified tag.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::removeByTag(DID tag, PDS pds)
{
    const bool check_pds = pds != 0 && tag >= 0x80;
    size_t removed_count = 0;

    for (auto it = _list.begin(); it != _list.end(); ) {
        const DID itag = it->desc->tag();
        if (itag == tag && (!check_pds || it->pds == pds) && (itag != DID_PRIV_DATA_SPECIF || prepareRemovePDS (it))) {
            it = _list.erase (it);
            ++removed_count;
        }
        else {
            ++it;
        }
    }

    return removed_count;
}


//----------------------------------------------------------------------------
// Total number of bytes that is required to serialize the list of descriptors.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::binarySize(size_t start, size_t count) const
{
    start = std::min(start, _list.size());
    count = std::min(count, _list.size() - start);
    size_t size = 0;

    for (size_t i = start; i < start + count; ++i) {
        size += _list[i].desc->size();
    }

    return size;
}


//----------------------------------------------------------------------------
// Serialize the content of the descriptor list.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::serialize(uint8_t*& addr, size_t& size, size_t start) const
{
    size_t i;

    for (i = start; i < _list.size() && _list[i].desc->size() <= size; ++i) {
        std::memcpy(addr, _list[i].desc->content(), _list[i].desc->size());
        addr += _list[i].desc->size();
        size -= _list[i].desc->size();
    }

    return i;
}


//----------------------------------------------------------------------------
// Serialize the content of the descriptor list in a byte block.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::serialize(ByteBlock& bb, size_t start) const
{
    // Keep track of byte block size before serializing the descriptor list.
    const size_t previous_size = bb.size();

    // Increase the byte block size by the max size of a descriptor list.
    size_t added_size = 0xFFFF;
    bb.resize(previous_size + added_size);

    // Serialize the descriptor list into the extended area.
    uint8_t* data = bb.data() + previous_size;
    size_t size = added_size;
    serialize(data, size, start);

    // 'size' contains the remaining size, from 'added_size'.
    // Update 'added_size' to contain the size of the serialized descriptor list.
    assert(size <= added_size);
    added_size -= size;

    // Shrink the byte block to the end of descriptor list.
    bb.resize(previous_size + added_size);
    return added_size;
}


//----------------------------------------------------------------------------
// Same as Serialize, but prepend a 2-byte length field before the list.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::lengthSerialize(uint8_t*& addr, size_t& size, size_t start, uint16_t reserved_bits, size_t length_bits) const
{
    assert(size >= 2);

    // Not more than 16 bits in the length field.
    length_bits = std::min<size_t>(length_bits, 16);

    // Reserve space for descriptor list length
    uint8_t* length_addr = addr;
    addr += 2;
    size -= 2;

    // Insert descriptor list
    size_t result = serialize(addr, size, start);

    // Update length
    size_t length = addr - length_addr - 2;
    PutUInt16(length_addr, uint16_t(length | (reserved_bits << length_bits)));

    return result;
}


//----------------------------------------------------------------------------
// Search a descriptor with the specified tag, starting at the
// specified index.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::search(DID tag, size_t start_index, PDS pds) const
{
    bool check_pds = pds != 0 && tag >= 0x80;
    size_t index = start_index;

    while (index < _list.size() && (_list[index].desc->tag() != tag || (check_pds && _list[index].pds != pds))) {
        index++;
    }

    return index;
}


//----------------------------------------------------------------------------
// Search a descriptor with the specified extended tag.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::search(const ts::EDID& edid, size_t start_index) const
{
    // If the EDID is table-specific, check that we are in the same table.
    // In the case the table of the descriptor list is unknown, assume that the table matches.
    const TID tid = edid.tableId();
    if (edid.isTableSpecific() && _table != nullptr && _table->tableId() != tid) {
        // No the same table, cannot match.
        return _list.size();
    }

    // Now search in the list.
    size_t index = start_index;
    while (index < _list.size() && _list[index].desc->edid(_list[index].pds, tid) != edid) {
        index++;
    }
    return index;
}


//----------------------------------------------------------------------------
// Search a descriptor for the specified language.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::searchLanguage(const DuckContext& duck, const UString& language, size_t start_index) const
{
    // Check that an actual language code was provided.
    if (language.size() != 3) {
        return count(); // not found
    }

    // Standards of the context and the parent table.
    const Standards standards = duck.standards() | (_table == nullptr ? Standards::NONE : _table->definingStandards());
    const bool atsc = bool(standards & Standards::ATSC);
    const bool isdb = bool(standards & Standards::ISDB);

    // Seach all known types of descriptors containing languages.
    for (size_t index = start_index; index < _list.size(); index++) {
        const DescriptorPtr& desc(_list[index].desc);
        if (!desc.isNull() && desc->isValid()) {

            const DID tag = desc->tag();
            const PDS pds = _list[index].pds;
            const uint8_t* data = desc->payload();
            size_t size = desc->payloadSize();

            if (tag == DID_LANGUAGE) {
                while (size >= 4) {
                    if (language.similar(data, 3)) {
                        return index;
                    }
                    data += 4; size -= 4;
                }
            }
            else if (tag == DID_COMPONENT && size >= 6 && language.similar(data + 3, 3)) {
                return index;
            }
            else if (tag == DID_SUBTITLING) {
                while (size >= 8) {
                    if (language.similar(data, 3)) {
                        return index;
                    }
                    data += 8; size -= 8;
                }
            }
            else if (tag == DID_TELETEXT || tag == DID_VBI_TELETEXT) {
                while (size >= 5) {
                    if (language.similar(data, 3)) {
                        return index;
                    }
                    data += 5; size -= 5;
                }
            }
            else if (tag == DID_MLINGUAL_COMPONENT || tag == DID_MLINGUAL_BOUQUET || tag == DID_MLINGUAL_NETWORK) {
                if (tag == DID_MLINGUAL_COMPONENT && size > 0) {
                    // Skip leading component_tag in multilingual_component_descriptor.
                    data++; size--;
                }
                while (size >= 4) {
                    if (language.similar(data, 3)) {
                        return index;
                    }
                    const size_t len = std::min<size_t>(4 + data[3], size);
                    data += len; size -= len;
                }
            }
            else if (tag == DID_MLINGUAL_SERVICE) {
                while (size >= 4) {
                    if (language.similar(data, 3)) {
                        return index;
                    }
                    size_t len = std::min<size_t>(4 + data[3], size);
                    if (len < size) {
                        len = std::min<size_t>(len + 1 + data[len], size);
                    }
                    data += len; size -= len;
                }
            }
            else if (tag == DID_SHORT_EVENT && size >= 3 && language.similar(data, 3)) {
                return index;
            }
            else if (tag == DID_EXTENDED_EVENT && size >= 4 && language.similar(data + 1, 3)) {
                return index;
            }
            else if ((atsc || pds == PDS_ATSC) && tag == DID_ATSC_CAPTION && size > 0) {
                data++; size--;
                while (size >= 6) {
                    if (language.similar(data, 3)) {
                        return index;
                    }
                    data += 6; size -= 6;
                }
            }
            else if ((isdb || pds == PDS_ISDB) && tag == DID_ISDB_AUDIO_COMP) {
                if (size >= 9 && language.similar(data + 6, 3)) {
                    return index;
                }
                if (size >= 12 && (data[5] & 0x80) != 0 && language.similar(data + 9, 3)) {
                    return index;
                }
            }
            else if ((isdb || pds == PDS_ISDB) && tag == DID_ISDB_DATA_CONTENT && size >= 4) {
                size_t len = std::min<size_t>(4 + data[3], size);
                if (len < size) {
                    len = std::min<size_t>(len + 1 + data[len], size);
                }
                if (len + 3 <= size && language.similar(data + len, 3)) {
                    return index;
                }
            }
        }
    }
    return count(); // not found
}


//----------------------------------------------------------------------------
// Search any kind of subtitle descriptor.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::searchSubtitle(const UString& language, size_t start_index) const
{
    // Value to return if not found
    size_t not_found = count();

    for (size_t index = start_index; index < _list.size(); index++) {

        const DID tag =_list[index].desc->tag();
        const uint8_t* desc = _list[index].desc->payload();
        size_t size = _list[index].desc->payloadSize();

        if (tag == DID_SUBTITLING) {
            // DVB Subtitling Descriptor, always contain subtitles
            if (language.empty()) {
                return index;
            }
            else {
                not_found = count() + 1;
                while (size >= 8) {
                    if (language.similar(desc, 3)) {
                        return index;
                    }
                    desc += 8;
                    size -= 8;
                }
            }
        }
        else if (tag == DID_TELETEXT) {
            // DVB Teletext Descriptor, may contain subtitles
            while (size >= 5) {
                // Get teletext type:
                //   0x02: Teletext subtitles
                //   0x05: Teletext subtitles for hearing impaired
                uint8_t tel_type = desc[3] >> 3;
                if (tel_type == 0x02 || tel_type == 0x05) {
                    // This is a teletext containing subtitles
                    if (language.empty()) {
                        return index;
                    }
                    else {
                        not_found = count() + 1;
                        if (language.similar(desc, 3)) {
                            return index;
                        }
                    }
                }
                desc += 5;
                size -= 5;
            }
        }
    }

    return not_found;
}


//----------------------------------------------------------------------------
// This method converts a descriptor list to XML.
//----------------------------------------------------------------------------

bool ts::DescriptorList::toXML(DuckContext& duck, xml::Element* parent) const
{
    bool success = true;
    for (size_t index = 0; index < _list.size(); ++index) {
        if (_list[index].desc.isNull() || _list[index].desc->toXML(duck, parent, duck.actualPDS(_list[index].pds), tableId() , false) == nullptr) {
            success = false;
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// These methods decode an XML list of descriptors.
//----------------------------------------------------------------------------

bool ts::DescriptorList::fromXML(DuckContext& duck, xml::ElementVector& others, const xml::Element* parent, const UString& allowedOthers)
{
    UStringList allowed;
    allowedOthers.split(allowed);
    return fromXML(duck, others, parent, allowed);
}

bool ts::DescriptorList::fromXML(DuckContext& duck, const xml::Element* parent)
{
    xml::ElementVector others;
    return fromXML(duck, others, parent, UStringList());
}

bool ts::DescriptorList::fromXML(DuckContext& duck, xml::ElementVector& others, const xml::Element* parent, const UStringList& allowedOthers)
{
    bool success = true;
    clear();
    others.clear();

    // Analyze all children nodes.
    for (const xml::Element* node = parent == nullptr ? nullptr : parent->firstChildElement(); node != nullptr; node = node->nextSiblingElement()) {

        DescriptorPtr bin = new Descriptor;
        CheckNonNull(bin.pointer());

        // Try to analyze the XML element.
        if (bin->fromXML(duck, node, tableId())) {
            // The XML tag is a valid descriptor name.
            if (bin->isValid()) {
                add(bin);
            }
            else {
                // The XML name is correct but the XML structure failed to produce a valid descriptor.
                parent->report().error(u"Error in descriptor <%s> at line %d", {node->name(), node->lineNumber()});
                success = false;
            }
        }
        else {
            // The tag is not a descriptor name, check if this is one of the allowed node.
            if (node->name().isContainedSimilarIn(allowedOthers)) {
                others.push_back(node);
            }
            else if (node->name().similar(u"metadata")) {
                // Always ignore <metadata> nodes.
            }
            else {
                parent->report().error(u"Illegal <%s> at line %d", {node->name(), node->lineNumber()});
                success = false;
            }
        }
    }
    return success;
}
