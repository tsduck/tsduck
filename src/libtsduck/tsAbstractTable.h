//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!
//!  @file
//!  Abstract base class for MPEG PSI/SI tables
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBinaryTable.h"
#include "tsTablesPtr.h"
#include "tsTablesDisplay.h"
#include "tsXML.h"

namespace ts {
    //!
    //! Abstract base class for MPEG PSI/SI tables.
    //!
    class TSDUCKDLL AbstractTable
    {
    public:
        //!
        //! Check if the table is valid.
        //! @return True if the table is valid.
        //!
        bool isValid() const {return _is_valid;}

        //!
        //! Invalidate the table.
        //! This object must be rebuilt.
        //!
        void invalidate() {_is_valid = false;}

        //!
        //! Get the table_id.
        //! @return The table_id.
        //!
        TID tableId() const {return _table_id;}

        //!
        //! Get the XMl node name representing this table.
        //! @return The XML node name.
        //!
        std::string xmlName() const {return _xml_name == 0 ? std::string() : std::string(_xml_name);}

        //!
        //! This abstract method serializes a table.
        //! @param [out] bin A binary table object.
        //! Its content is replaced with a binary representation of this object.
        //!
        virtual void serialize(BinaryTable& bin) const = 0;

        //!
        //! This abstract method deserializes a binary table.
        //! In case of success, this object is replaced with the interpreted content of @a bin.
        //! In case of error, this object is invalidated.
        //! @param [in] bin A binary table to interpret according to the table subclass.
        //!
        virtual void deserialize(const BinaryTable& bin) = 0;

        //!
        //! This abstract method converts the table to XML.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @return The new XML element.
        //!
        virtual XML::Element* toXML(XML& xml, XML::Element* parent) const = 0;

        //!
        //! This abstract converts an XML structure to a table.
        //! In case of success, this object is replaced with the interpreted content of the XML structure.
        //! In case of error, this object is invalidated.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] element XML element to convert.
        //!
        virtual void fromXML(XML& xml, const XML::Element* element) = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractTable () {}

    protected:
        //!
        //! The table id can be modified by subclasses only.
        //!
        TID _table_id;

        //!
        //! XML table name.
        //!
        const char* const _xml_name;

        //!
        //! It is the responsibility of the subclasses to set the valid flag
        //!
        bool _is_valid;

        //!
        //! Protected constructor for subclasses.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //!
        AbstractTable(TID tid, const char* xml_name) :
            _table_id(tid),
            _xml_name(xml_name),
            _is_valid(false)
        {
        }

    private:
        // Unreachable constructors and operators.
        AbstractTable() = delete;
        AbstractTable(const AbstractTable&) = delete;
        AbstractTable& operator=(const AbstractTable&) = delete;
    };
}
