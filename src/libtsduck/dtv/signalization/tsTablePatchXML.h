//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Implementation of on-the-fly table patching using XML.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBinaryTable.h"
#include "tsUString.h"
#include "tsSafePtr.h"
#include "tsxmlPatchDocument.h"
#include "tsxmlTweaks.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Implementation of on-the-fly table patching using XML.
    //! This class is typically used to handle -\-patch-xml command line options.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TablePatchXML
    {
        TS_NOBUILD_NOCOPY(TablePatchXML);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //!
        TablePatchXML(DuckContext& duck);

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);

        //!
        //! Clear all previously loaded patch files, clear list of patch files.
        //!
        void clear();

        //!
        //! Check if there is some patch to apply.
        //! When false, no patch file was specified or loaded.
        //! @return True if there is some patch to apply.
        //!
        bool hasPatchFiles() const { return !_patches.empty(); }

        //!
        //! Add a file name in the list of patch files.
        //! The file is not yet loaded.
        //! @param [in] filename Name of an XML patch file.
        //!
        void addPatchFileName(const UString& filename);

        //!
        //! Add file names in the list of patch files.
        //! The files are not yet loaded.
        //! @param [in] filenames Names of XML patch files.
        //! If a file name starts with "<?xml", this is considered as "inline XML content".
        //!
        void addPatchFileNames(const UStringVector& filenames);

        //!
        //! Add file names in the list of patch files.
        //! The files are not yet loaded.
        //! @param [in] filenames Names of XML patch files.
        //! If a file name starts with "<?xml", this is considered as "inline XML content".
        //!
        void addPatchFileNames(const UStringList& filenames);

        //!
        //! Load (or reload) the XML patch files.
        //! @param [in] tweaks XML tweaks to load in the documents.
        //! @return True on success, false some error occurred in the input files.
        //!
        bool loadPatchFiles(const xml::Tweaks& tweaks = xml::Tweaks());

        //!
        //! Apply the XML patch files to an XML document.
        //! @param [in,out] doc The XML document to patch.
        //!
        void applyPatches(xml::Document& doc) const;

        //!
        //! Apply the XML patch files to a binary table.
        //! @param [in,out] table The binary table to patch.
        //! @return True on success, false if the binary table or the patched XML is invalid.
        //! If the patch file deletes the table, then the returned value if true and the
        //! @a table is marked as invalid.
        //!
        bool applyPatches(BinaryTable& table) const;

        //!
        //! Apply the XML patch files to a binary section.
        //! This is a special processing since XML files are supposed to represent complete tables.
        //! The section is considered as section 0/0 of a complete table. The patch is applied on
        //! that table and the first section of the patched table is returned. Specific parts of
        //! the section are preserved, such as section number or last section number.
        //! @param [in,out] section Safe pointer to the section to patch.
        //! The pointer is modified, pointing to a new section content.
        //! @return True on success, false if the binary section or the patched XML is invalid.
        //! If the patch file deletes the table, then the returned value if true and @a section is null.
        //!
        bool applyPatches(SectionPtr& section) const;

    private:
        typedef ts::SafePtr<ts::xml::PatchDocument> PatchDocumentPtr;
        typedef std::vector<PatchDocumentPtr> PatchDocumentVector;

        DuckContext&        _duck;        // TSDuck execution context.
        UStringVector       _patchFiles;  // XML patch file names.
        PatchDocumentVector _patches;     // XML patch files as loaded documents
    };
}
