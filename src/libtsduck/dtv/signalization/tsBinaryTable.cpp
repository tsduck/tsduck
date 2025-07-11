//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBinaryTable.h"
#include "tsAbstractTable.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsSection.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Move constructor.
//----------------------------------------------------------------------------

ts::BinaryTable::BinaryTable(BinaryTable&& other) noexcept :
    _is_valid(other._is_valid),
    _tid(other._tid),
    _tid_ext(other._tid_ext),
    _version(other._version),
    _source_pid(other._source_pid),
    _missing_count(other._missing_count),
    _sections(std::move(other._sections))
{
    other._missing_count = 0; // section array was moved
}


//----------------------------------------------------------------------------
// Copy constructor. The sections are either shared between the
// two tables or duplicated.
//----------------------------------------------------------------------------

ts::BinaryTable::BinaryTable(const BinaryTable& other, ShareMode mode) :
    _is_valid(other._is_valid),
    _tid(other._tid),
    _tid_ext(other._tid_ext),
    _version(other._version),
    _source_pid(other._source_pid),
    _missing_count(other._missing_count),
    _sections()
{
    switch (mode) {
        case ShareMode::SHARE: {
            // Copy the pointers, share the pointed sections
            _sections = other._sections;
            break;
        }
        case ShareMode::COPY: {
            _sections.resize(other._sections.size());
            for (size_t i = 0; i < _sections.size(); ++i) {
                if (other._sections[i] == nullptr) {
                    _sections[i].reset();
                }
                else {
                    _sections[i] = std::make_shared<Section>(*other._sections[i], ShareMode::COPY);
                }
            }
            break;
        }
        default: {
            // should not get there
            assert(false);
        }
    }
}


//----------------------------------------------------------------------------
// Constructor from an array of sections.
//----------------------------------------------------------------------------

ts::BinaryTable::BinaryTable(const SectionPtrVector& sections, bool replace, bool grow)
{
    if (!addSections(sections, replace, grow)) {
        clear();
    }
}


//----------------------------------------------------------------------------
// Assignment. The sections are referenced, and thus shared
// between the two table objects.
//----------------------------------------------------------------------------

ts::BinaryTable& ts::BinaryTable::operator=(const BinaryTable& other)
{
    if (&other != this) {
        _is_valid = other._is_valid;
        _tid = other._tid;
        _tid_ext = other._tid_ext;
        _version = other._version;
        _source_pid = other._source_pid;
        _missing_count = other._missing_count;
        _sections = other._sections;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Move assignment.
//----------------------------------------------------------------------------

ts::BinaryTable& ts::BinaryTable::operator=(BinaryTable&& other) noexcept
{
    if (&other != this) {
        _is_valid = other._is_valid;
        _tid = other._tid;
        _tid_ext = other._tid_ext;
        _version = other._version;
        _source_pid = other._source_pid;
        _missing_count = other._missing_count;
        _sections = std::move(other._sections);
        other._missing_count = 0; // section array was moved
    }
    return *this;
}


//----------------------------------------------------------------------------
// Duplication. Similar to assignment but the sections are duplicated.
//----------------------------------------------------------------------------

ts::BinaryTable& ts::BinaryTable::copy(const BinaryTable& table)
{
    _is_valid = table._is_valid;
    _tid = table._tid;
    _tid_ext = table._tid_ext;
    _version = table._version;
    _source_pid = table._source_pid;
    _missing_count = table._missing_count;
    _sections.resize(table._sections.size());
    for (size_t i = 0; i < _sections.size(); ++i) {
        if (table._sections[i] == nullptr) {
            _sections[i].reset();
        }
        else {
            _sections[i] = std::make_shared<Section>(*table._sections[i], ShareMode::COPY);
        }
    }
    return *this;
}


//----------------------------------------------------------------------------
// Comparison. Note: Invalid tables are never identical
//----------------------------------------------------------------------------

bool ts::BinaryTable::operator==(const BinaryTable& table) const
{
    bool equal =
        _is_valid &&
        table._is_valid &&
        _tid == table._tid &&
        _tid_ext == table._tid_ext &&
        _version == table._version &&
        _sections.size() == table._sections.size();

    for (size_t i = 0; equal && i < _sections.size(); ++i) {
        equal = _sections[i] != nullptr && table._sections[i] != nullptr && *_sections[i] == *table._sections[i];
    }

    return equal;
}


//----------------------------------------------------------------------------
// Get a pointer to a section.
//----------------------------------------------------------------------------

const ts::SectionPtr ts::BinaryTable::sectionAt(size_t index) const
{
    return index < _sections.size() ? _sections[index] : SectionPtr();
}


//----------------------------------------------------------------------------
// Implementation of AbstractDefinedByStandards.
//----------------------------------------------------------------------------

ts::Standards ts::BinaryTable::definingStandards(Standards current_standards) const
{
    // The defining standard is taken from table id.
    return PSIRepository::Instance().getTableStandards(tableId(), _source_pid, current_standards);
}


//----------------------------------------------------------------------------
// Modifiable properties.
//----------------------------------------------------------------------------

void ts::BinaryTable::setTableIdExtension(uint16_t tid_ext, bool recompute_crc)
{
    _tid_ext = tid_ext;
    for (const auto& it : _sections) {
        if (it != nullptr) {
            it->setTableIdExtension(tid_ext, recompute_crc);
        }
    }
}

void ts::BinaryTable::setVersion(uint8_t version, bool recompute_crc)
{
    _version = version;
    for (const auto& it : _sections) {
        if (it != nullptr) {
            it->setVersion(version, recompute_crc);
        }
    }
}

void ts::BinaryTable::setSourcePID(PID pid)
{
    _source_pid = pid;
    for (const auto& it : _sections) {
        if (it != nullptr) {
            it->setSourcePID(pid);
        }
    }
}


//----------------------------------------------------------------------------
// Index of first and last TS packet of the table in the demultiplexed stream.
//----------------------------------------------------------------------------

ts::PacketCounter ts::BinaryTable::firstTSPacketIndex() const
{
    bool found = false;
    PacketCounter first = std::numeric_limits<PacketCounter>::max();
    for (const auto& it : _sections) {
        if (it != nullptr) {
            found = true;
            first = std::min(first, it->firstTSPacketIndex());
        }
    }
    return found ? first : 0;
}

ts::PacketCounter ts::BinaryTable::lastTSPacketIndex() const
{
    PacketCounter last = 0;
    for (const auto& it : _sections) {
        if (it != nullptr) {
            last = std::max(last, it->lastTSPacketIndex());
        }
    }
    return last;
}


//----------------------------------------------------------------------------
// Clear the content of the table. The table must be rebuilt
// using calls to addSection.
//----------------------------------------------------------------------------

void ts::BinaryTable::clear()
{
    _is_valid = false;
    _tid = 0xFF;
    _tid_ext = 0;
    _version = 0;
    _source_pid = PID_NULL;
    _missing_count = 0;
    _sections.clear();
}


//----------------------------------------------------------------------------
// Return the total size in bytes of all sections in the table.
//----------------------------------------------------------------------------

size_t ts::BinaryTable::totalSize() const
{
    size_t size = 0;
    for (const auto& it : _sections) {
        if (it != nullptr && it->isValid()) {
            size += it->size();
        }
    }
    return size;
}


//----------------------------------------------------------------------------
// Minimum number of TS packets required to transport the table.
//----------------------------------------------------------------------------

ts::PacketCounter ts::BinaryTable::packetCount(bool pack) const
{
    return Section::PacketCount(_sections, pack);
}


//----------------------------------------------------------------------------
// Get/set the generic user-defined string as "attribute" of the object.
//----------------------------------------------------------------------------

void ts::BinaryTable::setAttribute(const UString& attr)
{
    // Set attribute on all sections.
    for (const auto& it : _sections) {
        if (it != nullptr) {
            it->setAttribute(attr);
        }
    }
}

ts::UString ts::BinaryTable::attribute() const
{
    // Return first non-empty section attribute.
    for (const auto& it : _sections) {
        if (it != nullptr && !it->attribute().empty()) {
            return it->attribute();
        }
    }
    return UString();
}


//----------------------------------------------------------------------------
// Add several sections to a table
//----------------------------------------------------------------------------

bool ts::BinaryTable::addSections(SectionPtrVector::const_iterator first, SectionPtrVector::const_iterator last, bool replace, bool grow)
{
    bool ok = true;
    for (auto it = first; it != last; ++it) {
        ok = addSection(*it, replace, grow) && ok;
    }
    return ok;
}


//----------------------------------------------------------------------------
// A table is built by adding sections using addSection.
// When all sections are present, the table becomes valid.
//----------------------------------------------------------------------------

bool ts::BinaryTable::addSection(const SectionPtr& sect, bool replace, bool grow)
{
    // Reject invalid sections

    if (sect == nullptr || !sect->isValid()) {
        return false;
    }

    // Check the compatibility of the section with the table

    const int index = sect->sectionNumber();

    if (_sections.size() == 0) {
        // This is the first section, set the various parameters
        _sections.resize(size_t(sect->lastSectionNumber()) + 1);
        assert(index < int(_sections.size()));
        _tid = sect->tableId();
        _tid_ext = sect->tableIdExtension();
        _version = sect->version();
        _source_pid = sect->sourcePID();
        _missing_count = int(_sections.size());
    }
    else if (sect->tableId() != _tid || sect->tableIdExtension() != _tid_ext || sect->version() != _version) {
        // Not the same table
        return false;
    }
    else if (!grow && (index >= int(_sections.size()) || size_t(sect->lastSectionNumber()) != _sections.size() - 1)) {
        // Incompatible number of sections
        return false;
    }
    else if (size_t(sect->lastSectionNumber()) != _sections.size() - 1) {
        // Incompatible number of sections but the table is allowed to grow
        if (size_t(sect->lastSectionNumber()) < _sections.size() - 1) {
            // The new section must be updated
            sect->setLastSectionNumber(uint8_t(int(_sections.size()) - 1));
        }
        else {
            // The table must be updated (more sections)
            _missing_count += int(sect->lastSectionNumber()) + 1 - int(_sections.size());
            _sections.resize(size_t(sect->lastSectionNumber()) + 1);
            assert(index < int(_sections.size()));
            // Modify all previously entered sections
            for (int si = 0; si < int(_sections.size()); ++si) {
                if (_sections[si] != nullptr) {
                    _sections[si]->setLastSectionNumber(sect->lastSectionNumber());
                }
            }
        }
    }

    // Now add the section

    if (_sections[index] == nullptr) {
        // The section was not present, add it
        _sections[index] = sect;
        _missing_count--;
    }
    else if (!replace) {
        // Section already present, don't replace
        return false;
    }
    else {
        // Section already present but replace
        _sections[index] = sect;
    }

    // The table becomes valid if there is no more missing section
    _is_valid = _missing_count == 0;
    assert(_missing_count >= 0);

    return true;
}


//----------------------------------------------------------------------------
// Pack all sections in a table, removing references to missing sections.
//----------------------------------------------------------------------------

bool ts::BinaryTable::packSections()
{
    // There is nothing to do if no section is missing.
    if (_missing_count > 0) {
        assert(!_is_valid);
        assert(!_sections.empty());

        // Next section number to copy.
        size_t next_section = 0;

        // Pack all section pointers.
        for (size_t n = 0; n < _sections.size(); ++n) {
            if (_sections[n] != nullptr) {
                if (next_section != n) {
                    _sections[next_section] = _sections[n];
                }
                ++next_section;
            }
        }

        // Resize to new number of sections.
        _sections.resize(next_section);
        _missing_count = 0;
        _is_valid = !_sections.empty();

        // Now patch section numbers.
        for (size_t n = 0; n < _sections.size(); ++n) {
            assert(_sections[n] != nullptr);
            assert(next_section > 0);
            _sections[n]->setSectionNumber(uint8_t(n), false);
            _sections[n]->setLastSectionNumber(uint8_t(next_section - 1), true);
        }
    }
    return _is_valid;
}


//----------------------------------------------------------------------------
// Check if this is a table with short or long sections.
//----------------------------------------------------------------------------

bool ts::BinaryTable::isShortSection() const
{
    return _sections.size() == 1 && _sections[0] != nullptr && _sections[0]->isShortSection();
}

bool ts::BinaryTable::isLongSection() const
{
    // Some sections may not be populated, so look them all.
    for (const auto& sec : _sections) {
        if (sec != nullptr) {
            return sec->isLongSection();
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// This method converts the table to XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::BinaryTable::toXML(DuckContext& duck, xml::Element* parent, const XMLOptions& opt) const
{
    // Filter invalid tables.
    if (!_is_valid || _sections.size() == 0 || _sections[0] == nullptr) {
        return nullptr;
    }

    // The XML node we will generate.
    xml::Element* node = nullptr;

    // Try to generate a specialized XML structure.
    if (!opt.forceGeneric) {
        // Do we know how to deserialize this table?
        PSIRepository::TableFactory fac = PSIRepository::Instance().getTable(_tid, SectionContext(_source_pid, duck.standards())).factory;
        if (fac != nullptr) {
            // We know how to deserialize this table.
            AbstractTablePtr tp = fac();
            if (tp != nullptr) {
                // Deserialize from binary to object.
                tp->deserialize(duck, *this);
                if (tp->isValid()) {
                    // Serialize from object to XML.
                    node = tp->toXML(duck, parent);
                }
            }
        }
    }

    // If we could not generate a typed node, generate a generic one.
    if (node == nullptr) {
        if (_sections[0]->isShortSection()) {
            // Create a short section node.
            node = parent->addElement(AbstractTable::XML_GENERIC_SHORT_TABLE);
            node->setIntAttribute(u"table_id", _tid, true);
            node->setBoolAttribute(u"private", _sections[0]->isPrivateSection());
            node->addHexaText(_sections[0]->payload(), _sections[0]->payloadSize());
        }
        else {
            // Create a table with long sections.
            node = parent->addElement(AbstractTable::XML_GENERIC_LONG_TABLE);
            node->setIntAttribute(u"table_id", _tid, true);
            node->setIntAttribute(u"table_id_ext", _tid_ext, true);
            node->setIntAttribute(u"version", _version);
            node->setBoolAttribute(u"current", _sections[0]->isCurrent());
            node->setBoolAttribute(u"private", _sections[0]->isPrivateSection());

            // Add each section in binary format.
            for (const auto& sec : _sections) {
                if (sec != nullptr && sec->isValid()) {
                    node->addHexaTextChild(u"section", sec->payload(), sec->payloadSize());
                }
            }
        }
    }

    // Add optional metadata.
    if ((opt.setPID && _source_pid != PID_NULL) || opt.setLocalTime || opt.setPackets || opt.setSections) {
        // Add <metadata> element as first child of the table.
        // This element is not part of the table but describes how the table was collected.
        xml::Element* meta = AbstractTable::GetOrCreateMetadata(node);
        if (opt.setPID && _source_pid != PID_NULL) {
            meta->setIntAttribute(u"PID", _source_pid);
        }
        if (opt.setLocalTime) {
            meta->setDateTimeAttribute(u"time", Time::CurrentLocalTime());
        }
        if (opt.setPackets) {
            meta->setIntAttribute(u"first_ts_packet", firstTSPacketIndex());
            meta->setIntAttribute(u"last_ts_packet", lastTSPacketIndex());
        }
        if (opt.setSections) {
            for (const auto& sec : _sections) {
                if (sec != nullptr && sec->isValid()) {
                    meta->addHexaTextChild(u"section", sec->content(), sec->size());
                }
            }
        }
    }

    return node;
}




//----------------------------------------------------------------------------
// This method converts an XML node as a binary table.
//----------------------------------------------------------------------------

bool ts::BinaryTable::fromXML(DuckContext& duck, const xml::Element* node)
{
    // Filter invalid parameters.
    clear();
    if (node == nullptr) {
        // Not a valid XML name (not even an XML element).
        return false;
    }

    // Get the table factory for that kind of XML tag.
    const PSIRepository::TableFactory fac = PSIRepository::Instance().getTable(node->name()).factory;
    if (fac != nullptr) {
        // Create a table instance of the right type.
        AbstractTablePtr table = fac();
        if (table != nullptr) {
            table->fromXML(duck, node);
        }
        if (table != nullptr && table->isValid()) {
            // Accumulate standards in the context.
            duck.addStandards(table->definingStandards(duck.standards()));
            // Serialize the table.
            table->serialize(duck, *this);
            if (!isValid()) {
                // Serialization failed.
                node->report().error(u"<%s>, line %d, is correct but the binary serialization failed", node->name(), node->lineNumber());
                return false;
            }
        }
        // The XML element name was valid.
        return true;
    }

    // There are two possible forms of generic tables.
    if (node->name().similar(AbstractTable::XML_GENERIC_SHORT_TABLE)) {
        TID tid = 0xFF;
        bool priv = true;
        ByteBlock payload;
        if (node->getIntAttribute<TID>(tid, u"table_id", true, 0xFF, 0x00, 0xFF) &&
            node->getBoolAttribute(priv, u"private", false, true) &&
            node->getHexaText(payload, 0, MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE))
        {
            addNewSection(tid, priv, payload.data(), payload.size());
        }
        // The XML element name was valid.
        return true;
    }

    if (node->name().similar(AbstractTable::XML_GENERIC_LONG_TABLE)) {
        TID tid = 0xFF;
        uint16_t tidExt = 0xFFFF;
        uint8_t version = 0;
        bool priv = true;
        bool current = true;
        xml::ElementVector sectionNodes;
        if (node->getIntAttribute<TID>(tid, u"table_id", true, 0xFF, 0x00, 0xFF) &&
            node->getIntAttribute<uint16_t>(tidExt, u"table_id_ext", false, 0xFFFF, 0x0000, 0xFFFF) &&
            node->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
            node->getBoolAttribute(current, u"current", false, true) &&
            node->getBoolAttribute(priv, u"private", false, true) &&
            node->getChildren(sectionNodes, u"section", 1, 256))
        {
            for (size_t index = 0; index < sectionNodes.size(); ++index) {
                assert(sectionNodes[index] != nullptr);
                ByteBlock payload;
                if (sectionNodes[index]->getHexaText(payload, 0, MAX_PSI_LONG_SECTION_PAYLOAD_SIZE)) {
                    addNewSection(tid, priv, tidExt, version, current, uint8_t(index), uint8_t(index), payload.data(), payload.size());
                }
                else {
                    // Invalid <section> content.
                    clear();
                    break;
                }
            }
        }
        // The XML element name was valid.
        return true;
    }

    // At this point, the table is invalid.
    node->report().error(u"<%s>, line %d, is not a valid table", node->name(), node->lineNumber());
    return false;
}
