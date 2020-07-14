//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsDescriptorList.h"
#include "tsAbstractDescriptor.h"
#include "tsAbstractTable.h"
#include "tsPrivateDataSpecifierDescriptor.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and assignment.
//----------------------------------------------------------------------------

ts::DescriptorList::DescriptorList(const AbstractTable* table) :
    _table(table),
    _list()
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

void ts::DescriptorList::add(const DescriptorPtr& desc)
{
    PDS pds = 0;

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
}


//----------------------------------------------------------------------------
// Add one descriptor at end of list
//----------------------------------------------------------------------------

void ts::DescriptorList::add(DuckContext& duck, const AbstractDescriptor& desc)
{
    DescriptorPtr pd(new Descriptor);
    CheckNonNull(pd.pointer());
    desc.serialize(duck, *pd);
    if (pd->isValid()) {
        add(pd);
    }
}


//----------------------------------------------------------------------------
// Add descriptors from a memory area
//----------------------------------------------------------------------------

bool ts::DescriptorList::add(const void* data, size_t size)
{
    const uint8_t* desc = reinterpret_cast<const uint8_t*>(data);
    size_t length = 0;

    while (size >= 2 && (length = size_t(desc[1]) + 2) <= size) {
        add(DescriptorPtr(new Descriptor(desc, length)));
        desc += length;
        size -= length;
    }

    return size == 0;
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
// Return the "private data specifier" associated to a descriptor in the list.
//----------------------------------------------------------------------------

ts::PDS ts::DescriptorList::privateDataSpecifier(size_t index) const
{
    assert(index < _list.size());
    return _list[index].pds;
}


//----------------------------------------------------------------------------
// Prepare removal of a private_data_specifier descriptor.
//----------------------------------------------------------------------------

bool ts::DescriptorList::prepareRemovePDS(const ElementVector::iterator& it)
{
    // Eliminate invalid cases
    if (it == _list.end() || it->desc->tag() != DID_PRIV_DATA_SPECIF) {
        return false;
    }

    // Search for private descriptors ahead.
    ElementVector::iterator end;
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
        ::memcpy(addr, _list[i].desc->content(), _list[i].desc->size());
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
// Search a language descriptor for the specified language, starting at
// the specified index. Return the index of the descriptor in the list
// or count() if no such descriptor is found.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::searchLanguage(const UString& language, size_t start_index) const
{
    for (size_t index = start_index; index < _list.size(); index++) {
        if (_list[index].desc->tag() == DID_LANGUAGE) {
            // Got a language descriptor
            const uint8_t* desc = _list[index].desc->payload();
            size_t size = _list[index].desc->payloadSize();
            // The language code uses 3 bytes after the size
            if (size >= 3 && language.similar(desc, 3)) {
                return index;
            }
        }
    }

    return count(); // not found
}


//----------------------------------------------------------------------------
// Search any kind of subtitle descriptor, starting at the specified
// index. Return the index of the descriptor in the list.
// Return count() if no such descriptor is found.
//
// If the specified language is non-empty, look only for a subtitle
// descriptor matching the specified language. In this case, if some
// kind of subtitle descriptor exists in the list but none matches the
// language, return count()+1.
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
            if (node->name().containSimilar(allowedOthers)) {
                others.push_back(node);
            }
            else {
                parent->report().error(u"Illegal <%s> at line %d", {node->name(), node->lineNumber()});
                success = false;
            }
        }
    }
    return success;
}
