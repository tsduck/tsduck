//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDescriptorList.h"
#include "tsAbstractDescriptor.h"
#include "tsAbstractTable.h"
#include "tsDuckContext.h"
#include "tsPSIRepository.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and assignment.
//----------------------------------------------------------------------------

ts::DescriptorList::DescriptorList(const AbstractTable* table) :
    AbstractTableAttachment(table)
{
}

ts::DescriptorList::DescriptorList(const AbstractTable* table, const DescriptorList& dl) :
    AbstractTableAttachment(table),
    _list(dl._list)
{
}

ts::DescriptorList::DescriptorList(const AbstractTable* table, DescriptorList&& dl) noexcept :
    AbstractTableAttachment(table),
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
// Comparison
//----------------------------------------------------------------------------

bool ts::DescriptorList::operator==(const DescriptorList& other) const
{
    if (_list.size() != other._list.size()) {
        return false;
    }
    for (size_t i = 0; i < _list.size(); ++i) {
        const DescriptorPtr& desc1(_list[i]);
        const DescriptorPtr& desc2(other._list[i]);
        if (desc1 == nullptr || desc2 == nullptr || *desc1 != *desc2) {
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
    if (desc == nullptr || !desc->isValid()) {
        return false;
    }
    else {
        _list.push_back(desc);
        return true;
    }
}

bool ts::DescriptorList::add(const Descriptor& desc)
{
    if (!desc.isValid()) {
        return false;
    }
    else {
        _list.push_back(std::make_shared<Descriptor>(desc.content(), desc.size()));
        return true;
    }
}

bool ts::DescriptorList::add(DuckContext& duck, const AbstractDescriptor& desc)
{
    DescriptorPtr pd = std::make_shared<Descriptor>();
    if (!desc.serialize(duck, *pd)) {
        return false;
    }
    if (duck.fixPDS()) {
        addPrivateIdentifier(desc.edid());
    }
    return add(pd);
}


//----------------------------------------------------------------------------
// Add descriptors from a memory area
//----------------------------------------------------------------------------

bool ts::DescriptorList::add(const void* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }

    const uint8_t* desc = reinterpret_cast<const uint8_t*>(data);
    size_t length = 0;
    bool success = true;

    while (size >= 2 && (length = size_t(desc[1]) + 2) <= size) {
        success = add(std::make_shared<Descriptor>(desc, length)) && success;
        desc += length;
        size -= length;
    }

    return success && size == 0;
}

bool ts::DescriptorList::add(const void* addr)
{
    const uint8_t* data(reinterpret_cast<const uint8_t*>(addr));
    return data != nullptr && add(data, size_t(data[1]) + 2);
}


//----------------------------------------------------------------------------
// Merge one descriptor in the list.
//----------------------------------------------------------------------------

bool ts::DescriptorList::merge(DuckContext& duck, const AbstractDescriptor& desc)
{
    // Serialize the new descriptor. In case of error, there is nothing we can add.
    DescriptorPtr bindesc = std::make_shared<Descriptor>();
    desc.serialize(duck, *bindesc);
    if (!bindesc->isValid()) {
        return false;
    }

    const EDID edid = desc.edid();
    const DescriptorDuplication mode = desc.duplicationMode();

    // We need to search for a descriptor of same type only if the duplication mode is not simply ADD_ALWAYS.
    if (mode != DescriptorDuplication::ADD_ALWAYS) {
        const size_t index = search(edid);
        if (index < count()) {
            // A descriptor of same type has been found.
            switch (mode) {
                case DescriptorDuplication::IGNORE: {
                    // New descriptor shall be ignored.
                    return true;
                }
                case DescriptorDuplication::REPLACE: {
                    // New descriptor shall replace the previous one.
                    _list[index] = bindesc;
                    return true;
                }
                case DescriptorDuplication::MERGE: {
                    // New descriptor shall be merged into old one.
                    // We need to deserialize the previous descriptor first.
                    const AbstractDescriptorPtr dp(_list[index]->deserialize(duck, edid));
                    if (dp != nullptr && dp->merge(desc)) {
                        // Descriptor successfully merged. Reserialize it and replace it.
                        DescriptorPtr newdesc = std::make_shared<Descriptor>();
                        dp->serialize(duck, *newdesc);
                        if (newdesc->isValid()) {
                            _list[index] = std::move(newdesc);
                            return true;
                        }
                    }
                    // In case of merge failure, apply default processing later.
                    break;
                }
                case DescriptorDuplication::ADD_OTHER: {
                    // In case the two binary descriptors are exactly identical, do nothing.
                    if (*_list[index] == *bindesc) {
                        return true;
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
    // Insert a registration_descriptor or a private_data_specifier_descriptor if necessary.
    addPrivateIdentifier(edid);
    add(bindesc);
    return true;
}



//----------------------------------------------------------------------------
// Merge another descriptor list in this list.
//----------------------------------------------------------------------------

void ts::DescriptorList::merge(DuckContext& duck, const DescriptorList& other)
{
    // Can't merge on ourselves.
    if (&other != this) {
        // Loop on all descriptors of the other list.
        for (size_t index = 0; index < other._list.size(); ++index) {
            const auto& bindesc(other._list[index]);
            assert(bindesc != nullptr);
            if (bindesc->isValid()) {
                // The descriptor from the other list must be deserialized to be merged.
                DescriptorContext context(duck, other, index);
                const AbstractDescriptorPtr dp(bindesc->deserialize(duck, context));
                if (dp == nullptr || dp->duplicationMode() == DescriptorDuplication::ADD_ALWAYS) {
                    // Cannot be deserialized or simply add the descriptor.
                    addPrivateIdentifier(dp->edid());
                    add(bindesc);
                }
                else {
                    // Merge the descriptor.
                    merge(duck, *dp);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Get a reference to the descriptor at a specified index.
//----------------------------------------------------------------------------

ts::Descriptor& ts::DescriptorList::operator[](size_t index)
{
    assert(index < _list.size());
    assert(_list[index] != nullptr);
    return *_list[index];
}

const ts::Descriptor& ts::DescriptorList::operator[](size_t index) const
{
    assert(index < _list.size());
    assert(_list[index] != nullptr);
    return *_list[index];
}


//----------------------------------------------------------------------------
// Get the extended descriptor id of a descriptor in the list.
//----------------------------------------------------------------------------

ts::EDID ts::DescriptorList::edid(const DuckContext& duck, size_t index) const
{
    // Eliminate invalid descriptor, index out of range.
    if (index >= _list.size() || _list[index] == nullptr || !_list[index]->isValid()) {
        return EDID(); // invalid value
    }
    else {
        DescriptorContext context(duck, *this, index);
        return PSIRepository::Instance().getDescriptor(_list[index]->xdid(), context).edid;
    }
}


//----------------------------------------------------------------------------
// Check if the descriptor list contains a MPEG registration descriptor with the specified id.
//----------------------------------------------------------------------------

bool ts::DescriptorList::containsRegistration(REGID regid) const
{
    for (const auto& dp : _list) {
        assert(dp != nullptr);
        if (dp->isValid() && dp->tag() == DID_MPEG_REGISTRATION && dp->payloadSize() >= 4 && GetUInt32(dp->payload()) == regid) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Get a list of all registration ids, in all MPEG registration descriptors.
//----------------------------------------------------------------------------

void ts::DescriptorList::getAllRegistrations(const DuckContext& duck, REGIDVector& regids) const
{
    regids.clear();

    // Start with the default registration ids from command line.
    duck.updateREGIDs(regids);

    // Then add registration ids from the descriptor list.
    for (const auto& dp : _list) {
        assert(dp != nullptr);
        if (dp->isValid() && dp->tag() == DID_MPEG_REGISTRATION && dp->payloadSize() >= 4) {
            regids.push_back(GetUInt32(dp->payload()));
        }
    }
}


//----------------------------------------------------------------------------
// Update a REGID or PDS value if the descriptor is the right descriptor.
//----------------------------------------------------------------------------

void ts::DescriptorList::UpdateREGID(REGID& regid, const DescriptorPtr& desc)
{
    if (desc != nullptr && desc->isValid() && desc->tag() == DID_MPEG_REGISTRATION && desc->payloadSize() >= 4) {
        regid = GetUInt32(desc->payload());
    }
}

void ts::DescriptorList::UpdatePDS(PDS& pds, const DescriptorPtr& desc)
{
    if (desc != nullptr && desc->isValid() && desc->tag() == DID_DVB_PRIV_DATA_SPECIF && desc->payloadSize() >= 4) {
        pds = GetUInt32(desc->payload());
    }
}


//----------------------------------------------------------------------------
// Return the MPEG "registration id" associated to a descriptor in the list.
//----------------------------------------------------------------------------

ts::REGID ts::DescriptorList::registrationId(size_t index) const
{
    REGID regid = REGID_NULL;
    index = std::min(index, _list.size());

    // Loop on current descriptor list and top-level descriptor list for REGID.
    while (index-- > 0 && regid == REGID_NULL) {
        UpdateREGID(regid, _list[index]);
    }
    if (regid == REGID_NULL && hasTable()) {
        const DescriptorList* dlist = table()->topLevelDescriptorList();
        if (dlist != nullptr && dlist != this) {
            index = dlist->_list.size();
            while (index-- > 0 && regid == REGID_NULL) {
                UpdateREGID(regid, dlist->_list[index]);
            }
        }
    }
    return regid;
}


//----------------------------------------------------------------------------
// Return the "private data specifier" associated to a descriptor in the list.
//----------------------------------------------------------------------------

ts::PDS ts::DescriptorList::privateDataSpecifier(size_t index) const
{
    PDS pds = PDS_NULL;
    index = std::min(index, _list.size());
    while (index-- > 0 && pds == PDS_NULL) {
        UpdatePDS(pds, _list[index]);
    }
    return pds;
}


//----------------------------------------------------------------------------
// Prepare removal of a private_data_specifier descriptor.
//----------------------------------------------------------------------------

bool ts::DescriptorList::canRemovePDS(std::vector<DescriptorPtr>::iterator it)
{
    // Eliminate invalid cases
    if (it == _list.end() || *it == nullptr || (*it)->tag() != DID_DVB_PRIV_DATA_SPECIF) {
        return false;
    }

    // Search for private descriptors ahead.
    decltype(it) end;
    for (end = it + 1; end != _list.end(); ++end) {
        assert(*end != nullptr);
        const DID tag = (*end)->tag();
        if (tag >= 0x80) {
            // This is a private descriptor, the private_data_specifier descriptor is necessary and cannot be removed.
            return false;
        }
        if (tag == DID_DVB_PRIV_DATA_SPECIF) {
            // Found another private_data_specifier descriptor with no private
            // descriptor between the two => the first one can be removed.
            return true;
        }
    }

    // Nothing special found, we can remove.
    return true;
}


//----------------------------------------------------------------------------
// Add a MPEG registration_descriptor or a DVB PDS_descriptor if necessary.
//----------------------------------------------------------------------------

void ts::DescriptorList::add32BitDescriptor(DID did, uint32_t payload)
{
    uint8_t data[6];
    data[0] = did;
    data[1] = 4;
    PutUInt32(data + 2, payload);
    add(std::make_shared<Descriptor>(data, sizeof(data)));
}

void ts::DescriptorList::addRegistration(REGID regid)
{
    if (regid != REGID_NULL && registrationId(_list.size()) != regid) {
        add32BitDescriptor(DID_MPEG_REGISTRATION, regid);
    }
}

void ts::DescriptorList::addPrivateDataSpecifier(PDS pds)
{
    if (pds != 0 && pds != PDS_NULL && privateDataSpecifier(_list.size()) != pds) {
        add32BitDescriptor(DID_DVB_PRIV_DATA_SPECIF, pds);
    }
}

void ts::DescriptorList::addPrivateIdentifier(EDID edid)
{
    if (edid.isPrivateDVB() && privateDataSpecifier(_list.size()) != edid.pds()) {
        add32BitDescriptor(DID_DVB_PRIV_DATA_SPECIF, edid.pds());
    }
    else if (edid.isPrivateMPEG() && registrationId(_list.size()) != edid.regid()) {
        add32BitDescriptor(DID_MPEG_REGISTRATION, edid.regid());
    }
}


//----------------------------------------------------------------------------
// Remove all private descriptors without preceding PDS descriptor.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::removeInvalidPrivateDescriptors()
{
    size_t count = 0;
    PDS pds = 0;

    for (auto it = _list.begin(); it != _list.end(); ) {
        assert(*it != nullptr);
        if (!(*it)->isValid()) {
            // Invalid descriptor, remove it.
            it = _list.erase(it);
            count++;
        }
        else if ((*it)->tag() == DID_DVB_PRIV_DATA_SPECIF) {
            // Got a private data specifier descriptor.
            UpdatePDS(pds, *it);
            ++it;
        }
        else if ((pds == 0 || pds == PDS_NULL) && (*it)->tag() >= 0x80) {
            // Private descriptor without preceding PDS, remove it.
            it = _list.erase(it);
            count++;
        }
        else {
            ++it;
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
    assert(_list[index] != nullptr);
    if (_list[index]->tag() == DID_DVB_PRIV_DATA_SPECIF && !canRemovePDS(_list.begin() + index)) {
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
    const bool check_pds = pds != 0 && pds != PDS_NULL && tag >= 0x80;
    PDS current_pds = 0;
    size_t removed_count = 0;

    for (auto it = _list.begin(); it != _list.end(); ) {
        assert(*it != nullptr);
        const DID itag = (*it)->tag();
        if (itag == tag && (!check_pds || current_pds == pds) && (itag != DID_DVB_PRIV_DATA_SPECIF || canRemovePDS(it))) {
            it = _list.erase(it);
            ++removed_count;
        }
        else {
            if (check_pds) {
                UpdatePDS(current_pds, *it);
            }
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
        size += _list[i]->size();
    }

    return size;
}


//----------------------------------------------------------------------------
// Serialize the content of the descriptor list.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::serialize(uint8_t*& addr, size_t& size, size_t start) const
{
    size_t i;

    for (i = start; i < _list.size() && _list[i]->size() <= size; ++i) {
        MemCopy(addr, _list[i]->content(), _list[i]->size());
        addr += _list[i]->size();
        size -= _list[i]->size();
    }

    return i;
}


//----------------------------------------------------------------------------
// Serialize the content of the descriptor list in a byte block.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::serialize(ByteBlock& bb, size_t start) const
{
    // Remember block size before serializing the descriptor list.
    const size_t previous_size = bb.size();

    // Increase the byte block size by the size of the descriptor list.
    const size_t added_size = binarySize();
    bb.resize(previous_size + added_size);

    // Serialize the descriptor list into the extended area.
    uint8_t* data = bb.data() + previous_size;
    size_t size = added_size;
    serialize(data, size, start);
    return added_size;
}


//----------------------------------------------------------------------------
// Same as serialize(), but prepend a 2-byte length field before the list.
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
// Search a descriptor with the specified tag.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::search(DID tag, size_t start_index, PDS pds) const
{
    bool check_pds = pds != 0 && pds != PDS_NULL && tag >= 0x80;
    PDS current_pds = check_pds ? privateDataSpecifier(start_index) : PDS_NULL;
    size_t index = start_index;

    while (index < _list.size() && (_list[index] == nullptr || _list[index]->tag() != tag || (check_pds && current_pds != pds))) {
        if (check_pds) {
            UpdatePDS(current_pds, _list[index]);
        }
        index++;
    }

    return index;
}


//----------------------------------------------------------------------------
// Search a descriptor with the specified extended tag.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::search(const ts::EDID& edid, size_t start_index) const
{
    const DID did = edid.did();
    const XDID xdid = edid.xdid();

    // If the EDID is table-specific, check that we are in the same table.
    // In the case the table of the descriptor list is unknown, assume that the table matches.
    if (edid.isTableSpecific() && hasTable() && !edid.matchTableSpecific(tableId(), tableStandards())) {
        // No the same table, cannot match.
        return _list.size();
    }

    // Track REGID and PDS if necessary.
    REGID regid = edid.isPrivateMPEG() ? registrationId(start_index) : REGID_NULL;
    PDS pds = edid.isPrivateDVB() ? privateDataSpecifier(start_index) : PDS_NULL;

    // Now search in the list.
    for (size_t index = start_index; index < _list.size(); ++index) {
        UpdateREGID(regid, _list[index]);
        UpdatePDS(pds, _list[index]);
        // First, filter on descriptor id (no need to search more if does not match).
        assert(_list[index] != nullptr);
        if (_list[index]->isValid() && _list[index]->tag() == did) {
            // Now, it's worth having a look.
            if (edid.isRegular() ||
                edid.isTableSpecific() ||
                (edid.isExtension() && _list[index]->xdid() == xdid) ||
                (edid.isPrivateMPEG() && edid.regid() == regid) ||
                (edid.isPrivateDVB() && edid.pds() == pds))
            {
                return index; // found
            }
        }
    }
    return _list.size(); // not found
}


//----------------------------------------------------------------------------
// Explore the descriptor and invoke a callback for each language.
// The callback shall return true to continue, false to stop.
//----------------------------------------------------------------------------

template <typename F>
void ts::DescriptorList::browseLanguages(const DuckContext& duck, size_t start_index, F callback) const
{
    // Standards of the context and the parent table. Used to interpret descriptors.
    // DVB is assumed if ATSC is not specified. ISDB reuses some DVB descriptors.
    const Standards standards = duck.standards() | tableStandards();
    const bool atsc = bool(standards & Standards::ATSC);
    const bool isdb = bool(standards & Standards::ISDB);
    const bool dvb = bool(standards & Standards::DVB) || !atsc;

    // Seach all known types of descriptors containing languages.
    bool more = true;
    for (size_t index = start_index; more && index < _list.size(); index++) {
        const DescriptorPtr& desc(_list[index]);
        assert(desc != nullptr);
        if (desc->isValid()) {

            const DID tag = desc->tag();
            const char* data = reinterpret_cast<const char*>(desc->payload());
            size_t size = desc->payloadSize();

            if (tag == DID_MPEG_LANGUAGE) {
                while (more && size >= 4) {
                    more = callback(index, data, 3);
                    data += 4; size -= 4;
                }
            }
            else if (dvb && tag == DID_DVB_COMPONENT && size >= 6) {
                more = callback(index, data + 3, 3);
            }
            else if (dvb && tag == DID_DVB_SUBTITLING) {
                while (more && size >= 8) {
                    more = callback(index, data, 3);
                    data += 8; size -= 8;
                }
            }
            else if (dvb && (tag == DID_DVB_TELETEXT || tag == DID_DVB_VBI_TELETEXT)) {
                while (more && size >= 5) {
                    more = callback(index, data, 3);
                    data += 5; size -= 5;
                }
            }
            else if (dvb && (tag == DID_DVB_MLINGUAL_COMPONENT || tag == DID_DVB_MLINGUAL_BOUQUET || tag == DID_DVB_MLINGUAL_NETWORK)) {
                if (tag == DID_DVB_MLINGUAL_COMPONENT && size > 0) {
                    // Skip leading component_tag in multilingual_component_descriptor.
                    data++; size--;
                }
                while (more && size >= 4) {
                    more = callback(index, data, 3);
                    const size_t len = std::min<size_t>(4 + uint8_t(data[3]), size);
                    data += len; size -= len;
                }
            }
            else if (dvb && tag == DID_DVB_MLINGUAL_SERVICE) {
                while (more && size >= 4) {
                    more = callback(index, data, 3);
                    size_t len = std::min<size_t>(4 + uint8_t(data[3]), size);
                    if (len < size) {
                        len = std::min<size_t>(len + 1 + uint8_t(data[len]), size);
                    }
                    data += len; size -= len;
                }
            }
            else if (dvb && tag == DID_DVB_SHORT_EVENT && size >= 3) {
                more = callback(index, data, 3);
            }
            else if (dvb && tag == DID_DVB_EXTENDED_EVENT && size >= 4) {
                more = callback(index, data + 1, 3);
            }
            else if (atsc && tag == DID_ATSC_CAPTION && size > 0) {
                data++; size--;
                while (more && size >= 6) {
                    more = callback(index, data, 3);
                    data += 6; size -= 6;
                }
            }
            else if (isdb && tag == DID_ISDB_AUDIO_COMP) {
                if (size >= 9) {
                    more = callback(index, data + 6, 3);
                }
                if (more && size >= 12 && (data[5] & 0x80) != 0) {
                    more = callback(index, data + 9, 3);
                }
            }
            else if (isdb && tag == DID_ISDB_DATA_CONTENT && size >= 4) {
                size_t len = std::min<size_t>(4 + uint8_t(data[3]), size);
                if (len < size) {
                    len = std::min<size_t>(len + 1 + uint8_t(data[len]), size);
                }
                if (len + 3 <= size) {
                    more = callback(index, data + len, 3);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Search a descriptor for the specified language.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::searchLanguage(const DuckContext& duck, const UString& language, size_t start_index) const
{
    size_t result = size(); // not found by default
    browseLanguages(duck, start_index, [&](size_t index, const char* addr, size_t size) {
        if (language.similar(addr, size)) {
            result = index;
            return false; // stop browsing languages
        }
        return true;
    });
    return result;
}


//----------------------------------------------------------------------------
// Get a list of all language codes from all descriptors.
//----------------------------------------------------------------------------

void ts::DescriptorList::getAllLanguages(const DuckContext& duck, UStringVector& languages, size_t max_count) const
{
    languages.clear();
    languages.reserve(_list.size());

    if (max_count > 0) {
        browseLanguages(duck, 0, [max_count, &languages](size_t index, const char* addr, size_t size) {
            languages.push_back(UString::FromUTF8(addr, size));
            return languages.size() < max_count;
        });
    }
}


//----------------------------------------------------------------------------
// Search any kind of subtitle descriptor.
//----------------------------------------------------------------------------

size_t ts::DescriptorList::searchSubtitle(const DuckContext& duck, const UString& language, size_t start_index) const
{
    // Standards of the context and the parent table.
    const Standards standards = duck.standards() | tableStandards();
    const bool dvb = bool(standards & Standards::DVB);

    // Value to return if not found
    size_t not_found = count();

    // Seach all known types of descriptors containing subtitles.
    for (size_t index = start_index; index < _list.size(); index++) {
        const DescriptorPtr& desc(_list[index]);
        assert(desc != nullptr);
        if (desc->isValid()) {

            const DID tag = desc->tag();
            const uint8_t* data = desc->payload();
            size_t size = desc->payloadSize();

            if (dvb && tag == DID_DVB_SUBTITLING) {
                // DVB Subtitling Descriptor, always contain subtitles
                if (language.empty()) {
                    return index;
                }
                else {
                    not_found = count() + 1;
                    while (size >= 8) {
                        if (language.similar(data, 3)) {
                            return index;
                        }
                        data += 8;
                        size -= 8;
                    }
                }
            }
            else if (dvb && tag == DID_DVB_TELETEXT) {
                // DVB Teletext Descriptor, may contain subtitles
                while (size >= 5) {
                    // Get teletext type:
                    //   0x02: Teletext subtitles
                    //   0x05: Teletext subtitles for hearing impaired
                    uint8_t tel_type = data[3] >> 3;
                    if (tel_type == 0x02 || tel_type == 0x05) {
                        // This is a teletext containing subtitles
                        if (language.empty()) {
                            return index;
                        }
                        else {
                            not_found = count() + 1;
                            if (language.similar(data, 3)) {
                                return index;
                            }
                        }
                    }
                    data += 5;
                    size -= 5;
                }
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
        DescriptorContext context(duck, *this, index);
        assert(_list[index] != nullptr);
        if (_list[index]->toXML(duck, parent, context, false) == nullptr) {
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
    EDID edid;

    // Analyze all children nodes. Most of them are descriptors.
    for (const xml::Element* node = parent == nullptr ? nullptr : parent->firstChildElement(); node != nullptr; node = node->nextSiblingElement()) {

        DescriptorPtr bin = std::make_shared<Descriptor>();

        if (node->name().isContainedSimilarIn(allowedOthers)) {
            // The tag is not a descriptor name, this is one of the allowed other node.
            others.push_back(node);
        }
        else if (node->name().similar(u"metadata")) {
            // Always ignore <metadata> nodes.
        }
        else if (!bin->fromXML(duck, edid, node, tableId())) {
            // Failed to analyze the node as a descriptor.
            parent->report().error(u"Illegal <%s> at line %d", node->name(), node->lineNumber());
            success = false;
        }
        else if (bin->isValid()) {
            // The XML tag is a valid descriptor name.
            if (duck.fixPDS()) {
                addPrivateIdentifier(edid);
            }
            add(bin);
        }
        else {
            // The XML name is correct but the XML structure failed to produce a valid descriptor.
            parent->report().error(u"Error in descriptor <%s> at line %d", node->name(), node->lineNumber());
            success = false;
        }
    }
    return success;
}
