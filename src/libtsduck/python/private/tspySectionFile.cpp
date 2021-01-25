//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
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
//
//  TSDuck Python bindings: encapsulates SectionFile objects for Python.
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tsSectionFile.h"
#include "tsDuckContext.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Interface of native methods.
//----------------------------------------------------------------------------

//!
//! Create a new instance of ts::SectionFile.
//! @param [in] duck A previously allocated instance of DuckContext.
//! @return A new ts::SectionFile instance.
//!
TSDUCKPY void* tspyNewSectionFile(void* duck);

//!
//! Delete a previously allocated instance of ts::SectionFile.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//!
TSDUCKPY void tspyDeleteSectionFile(void* sf);

//!
//! Clear the content of the SectionFile, erase all sections.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//!
TSDUCKPY void tspySectionFileClear(void* sf);

//!
//! Get the size in bytes of all sections.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @return The size in bytes of all sections.
//!
TSDUCKPY size_t tspySectionFileBinarySize(void* sf);

//!
//! Get the total number of sections in the file.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @return The total number of sections in the file.
//!
TSDUCKPY size_t tspySectionFileSectionsCount(void* sf);

//!
//! Get the total number of full tables in the file.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @return The total number of full tables in the file.
//!
TSDUCKPY size_t tspySectionFileTablesCount(void* sf);

//!
//! Load a binary section file.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @param [in] name Address of a buffer containing a UTF-16 string of the file name.
//! @param [in] name_size Size in bytes of the @a name buffer.
//! @return True on success, false on error.
//!
TSDUCKPY bool tspySectionFileLoadBinary(void* sf, const uint8_t* name, size_t name_size);

//!
//! Save a binary section file.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @param [in] name Address of a buffer containing a UTF-16 string of the file name.
//! @param [in] name_size Size in bytes of the @a name buffer.
//! @return True on success, false on error.
//!
TSDUCKPY bool tspySectionFileSaveBinary(void* sf, const uint8_t* name, size_t name_size);

//!
//! Load an XML section file.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @param [in] name Address of a buffer containing a UTF-16 string of the file name.
//! @param [in] name_size Size in bytes of the @a name buffer.
//! @return True on success, false on error.
//!
TSDUCKPY bool tspySectionFileLoadXML(void* sf, const uint8_t* name, size_t name_size);

//!
//! Save an XML section file.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @param [in] name Address of a buffer containing a UTF-16 string of the file name.
//! @param [in] name_size Size in bytes of the @a name buffer.
//! @return True on success, false on error.
//!
TSDUCKPY bool tspySectionFileSaveXML(void* sf, const uint8_t* name, size_t name_size);

//!
//! Save a JSON section file.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @param [in] name Address of a buffer containing a UTF-16 string of the file name.
//! @param [in] name_size Size in bytes of the @a name buffer.
//! @return True on success, false on error.
//!
TSDUCKPY bool tspySectionFileSaveJSON(void* sf, const uint8_t* name, size_t name_size);

//!
//! Serialize as XML text.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @param [out] buffer Address of a buffer where the version string is returned in UTF-16 format.
//! @param [in,out] size Initial/maximum size in bytes of the buffer. Upon return, contains the written size in bytes.
//! @return The full size of the complete string, can be larger than buffer size.
//!
TSDUCKPY size_t tspySectionFileToXML(void* sf, uint8_t* buffer, size_t* size);

//!
//! Serialize as JSON text.
//! @param [in] sf A previously allocated instance of ts::SectionFile.
//! @param [out] buffer Address of a buffer where the version string is returned in UTF-16 format.
//! @param [in,out] size Initial/maximum size in bytes of the buffer. Upon return, contains the written size in bytes.
//! @return The full size of the complete string, can be larger than buffer size.
//!
TSDUCKPY size_t tspySectionFileToJSON(void* sf, uint8_t* buffer, size_t* size);

//-----------------------------------------------------------------------------
// Python interface to SectionFile.
//-----------------------------------------------------------------------------

void* tspyNewSectionFile(void* duck)
{
    ts::DuckContext* dk = reinterpret_cast<ts::DuckContext*>(duck);
    return dk == nullptr ? nullptr : new ts::SectionFile(*dk);
}

void tspyDeleteSectionFile(void* sf)
{
    delete reinterpret_cast<ts::SectionFile*>(sf);
}

void tspySectionFileClear(void* sf)
{
     ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
     if (file != nullptr) {
         file->clear();
     }
}

size_t tspySectionFileBinarySize(void* sf)
{
     ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
     return file == nullptr ? 0 : file->binarySize();
}

size_t tspySectionFileSectionsCount(void* sf)
{
     ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
     return file == nullptr ? 0 : file->sectionsCount();
}

size_t tspySectionFileTablesCount(void* sf)
{
     ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
     return file == nullptr ? 0 : file->tablesCount();
}

bool tspySectionFileLoadBinary(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->loadBinary(ts::py::ToString(name, name_size));
}

bool tspySectionFileSaveBinary(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->saveBinary(ts::py::ToString(name, name_size));
}

bool tspySectionFileLoadXML(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->loadXML(ts::py::ToString(name, name_size));
}

bool tspySectionFileSaveXML(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->saveXML(ts::py::ToString(name, name_size));
}

bool tspySectionFileSaveJSON(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->saveJSON(ts::py::ToString(name, name_size));
}

size_t tspySectionFileToXML(void* sf, uint8_t* buffer, size_t* size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    ts::UString text;
    if (file != nullptr) {
        text = file->toXML();
    }
    ts::py::FromString(text, buffer, size);
    return 2 * text.length();
}

size_t tspySectionFileToJSON(void* sf, uint8_t* buffer, size_t* size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    ts::UString text;
    if (file != nullptr) {
        text = file->toJSON();
    }
    ts::py::FromString(text, buffer, size);
    return 2 * text.length();
}
