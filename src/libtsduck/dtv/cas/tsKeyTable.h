//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A basic and non-secure implementation of a symmetric key table.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsByteBlock.h"
#include "tsBlockCipher.h"
#include "tsReport.h"
#include "tsxmlDocument.h"

namespace ts {
    //!
    //! Definition of a basic and non-secure implementation of a symmetric key table.
    //! @ingroup crypto
    //!
    //! Each key is identified using a binary key id. The key value is a byte block.
    //! There is no constraint on the key id and value sizes, it depends on the application.
    //!
    //! A key table can be loaded from an XML file with the following structure as example:
    //! @code
    //! <?xml version="1.0" encoding="UTF-8"?>
    //! <tsduck>
    //!   <key id="9A46E5742F4F41059009F4855CBACAC6" value="E897935A77C0227F8136DA4125A4F7F3"/>
    //!   <key id="7DE569C08943571E4F926682CFED94AC" value="35FB9EE7B57AE8DEFB1A2CFA15A906D3"/>
    //! </tsduck>
    //! @endcode
    //!
    class TSDUCKDLL KeyTable
    {
    public:
        //!
        //! Constructor.
        //!
        KeyTable();

        //!
        //! Clear the content of the key table.
        //!
        void clear() { _keys.clear(); }

        //!
        //! Check if the key table is empty.
        //! @return True if the key table is empty.
        //!
        bool empty() const { return _keys.empty(); }

        //!
        //! Get the number of keys in the table.
        //! @return The number of keys in the table.
        //!
        size_t size() const { return _keys.size(); }

        //!
        //! Check the presence of a key in the table.
        //! @param [in] id The key id to search.
        //! @return True if the key is present, false otherwise.
        //!
        bool hasKey(const ByteBlock& id) const;

        //!
        //! Check the presence of a key in the table.
        //! @param [in] id The key id to search as a string of hexadecimal digits.
        //! @return True if the key is present, false otherwise.
        //!
        bool hasKey(const UString& id) const;

        //!
        //! Store a key in the table.
        //! @param [in] id The key id to load.
        //! @param [in] value The key value to load.
        //! @param [in] replace If true, allow the replacement of an existing key.
        //! @return True on success, false otherwise.
        //!
        bool storeKey(const ByteBlock& id, const ByteBlock& value, bool replace = true);

        //!
        //! Store a key in the table.
        //! @param [in] id The key id to load as a string of hexadecimal digits.
        //! @param [in] value The key value to load as a string of hexadecimal digits.
        //! @param [in] replace If true, allow the replacement of an existing key.
        //! @return True on success, false otherwise.
        //!
        bool storeKey(const UString& id, const UString& value, bool replace = true);

        //!
        //! Get the value of a key from the table.
        //! @param [in] id The key id to retrieve.
        //! @param [out] value The returned key value.
        //! @return True on success, false otherwise.
        //!
        bool getKey(const ByteBlock& id, ByteBlock& value) const;

        //!
        //! Get the value of a key from the table.
        //! @param [in] id The key id to retrieve as a string of hexadecimal digits.
        //! @param [out] value The returned key value.
        //! @return True on success, false otherwise.
        //!
        bool getKey(const UString& id, ByteBlock& value) const;

        //!
        //! Retrieve a key in the table and initialize a block cipher engine.
        //! @param [in,out] cipher The block cipher engine into which the key shall be loaded.
        //! @param [in] id The key id to retrieve.
        //! @param [in] rounds Requested number of key scheduling rounds. If zero, the default is used.
        //! @return True on success, false otherwise.
        //!
        bool setKey(BlockCipher& cipher, const ByteBlock& id, size_t rounds = 0) const;

        //!
        //! Retrieve a key in the table and initialize a block cipher engine.
        //! @param [in,out] cipher The block cipher engine into which the key shall be loaded.
        //! @param [in] id The key id to retrieve as a string of hexadecimal digits.
        //! @param [in] rounds Requested number of key scheduling rounds. If zero, the default is used.
        //! @return True on success, false otherwise.
        //!
        bool setKey(BlockCipher& cipher, const UString& id, size_t rounds = 0) const;

        //!
        //! Load all keys from an XML file and add them in the key table.
        //! @param [in,out] report Where to report errors.
        //! @param [in] filename The name of the file to load.
        //! @param [in] replace If true, allow the replacement of existing keys.
        //! @param [in] id_size If not zero, all key ids must have this size in bytes.
        //! @param [in] value_size If not zero, all key values must have this size in bytes.
        //! @return True on success, false otherwise.
        //!
        bool loadFile(Report& report, const UString& filename, bool replace, size_t id_size, size_t value_size);

        //!
        //! Load all keys from an XML string and add them in the key table.
        //! @param [in,out] report Where to report errors.
        //! @param [in] text The XML text to parse.
        //! @param [in] replace If true, allow the replacement of existing keys.
        //! @param [in] id_size If not zero, all key ids must have this size in bytes.
        //! @param [in] value_size If not zero, all key values must have this size in bytes.
        //! @return True on success, false otherwise.
        //!
        bool loadXML(Report& report, const UString& text, bool replace, size_t id_size, size_t value_size);

    private:
        std::map<ByteBlock,ByteBlock> _keys;  // key table, indexed by id

        // Common code for loadFile() and loadXML().
        bool parseXML(xml::Document& doc, bool replace, size_t id_size, size_t value_size);
    };
}
