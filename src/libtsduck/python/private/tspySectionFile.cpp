//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSDuck Python bindings: encapsulates SectionFile objects for Python.
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tsSectionFile.h"
#include "tsDuckContext.h"

//-----------------------------------------------------------------------------
// Python interface to SectionFile.
//-----------------------------------------------------------------------------

TSDUCKPY void* tspyNewSectionFile(void* duck)
{
    ts::DuckContext* dk = reinterpret_cast<ts::DuckContext*>(duck);
    return dk == nullptr ? nullptr : new ts::SectionFile(*dk);
}

TSDUCKPY void tspyDeleteSectionFile(void* sf)
{
    delete reinterpret_cast<ts::SectionFile*>(sf);
}

TSDUCKPY void tspySectionFileClear(void* sf)
{
     ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
     if (file != nullptr) {
         file->clear();
     }
}

TSDUCKPY size_t tspySectionFileBinarySize(void* sf)
{
     ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
     return file == nullptr ? 0 : file->binarySize();
}

TSDUCKPY size_t tspySectionFileSectionsCount(void* sf)
{
     ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
     return file == nullptr ? 0 : file->sectionsCount();
}

TSDUCKPY size_t tspySectionFileTablesCount(void* sf)
{
     ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
     return file == nullptr ? 0 : file->tablesCount();
}

TSDUCKPY bool tspySectionFileLoadBinary(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->loadBinary(ts::py::ToString(name, name_size));
}

TSDUCKPY bool tspySectionFileSaveBinary(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->saveBinary(ts::py::ToString(name, name_size));
}

TSDUCKPY bool tspySectionFileLoadXML(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->loadXML(ts::py::ToString(name, name_size));
}

TSDUCKPY bool tspySectionFileSaveXML(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->saveXML(ts::py::ToString(name, name_size));
}

TSDUCKPY bool tspySectionFileSaveJSON(void* sf, const uint8_t* name, size_t name_size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->saveJSON(ts::py::ToString(name, name_size));
}

TSDUCKPY size_t tspySectionFileToXML(void* sf, uint8_t* buffer, size_t* size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    ts::UString text;
    if (file != nullptr) {
        text = file->toXML();
    }
    ts::py::FromString(text, buffer, size);
    return 2 * text.length();
}

TSDUCKPY size_t tspySectionFileToJSON(void* sf, uint8_t* buffer, size_t* size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    ts::UString text;
    if (file != nullptr) {
        text = file->toJSON();
    }
    ts::py::FromString(text, buffer, size);
    return 2 * text.length();
}

TSDUCKPY bool tspySectionLoadBuffer(void* sf, const uint8_t* buffer, size_t size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    return file != nullptr && file->loadBuffer(buffer, size);
}

TSDUCKPY void tspySectionSaveBuffer(void* sf, uint8_t* buffer, size_t* size)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    if (file != nullptr && size != nullptr) {
        *size = file->saveBuffer(buffer, *size);
    }
}

TSDUCKPY void tspySectionFileSetCRCValidation(void* sf, int mode)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    if (file != nullptr) {
        file->setCRCValidation(ts::CRC32::Validation(mode));
    }
}

TSDUCKPY void tspySectionFileReorganizeEITs(void* sf, int year, int month, int day)
{
    ts::SectionFile* file = reinterpret_cast<ts::SectionFile*>(sf);
    if (file != nullptr) {
        ts::Time reftime;
        if (year > 0 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
            reftime = ts::Time::Fields(int(year), int(month), int(day));
        }
        file->reorganizeEITs(reftime);
    }
}
