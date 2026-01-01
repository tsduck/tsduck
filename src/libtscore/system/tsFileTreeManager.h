//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Manages a tree of files, save and cleanup.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsTime.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Manages a tree of files, save and cleanup.
    //! @ingroup libtscore files
    //!
    class TSCOREDLL FileTreeManager
    {
        TS_NOBUILD_NOCOPY(FileTreeManager);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors and messages.
        //!
        FileTreeManager(Report& report);

        //!
        //! Set the root directory of files to save and cleanup.
        //! If no root directory is specified, no file can be saved.
        //! By default, there is no initial root directory.
        //! @param [in] root Root directory.
        //!
        void setRootDirectory(const fs::path& root) { _root = root; }

        //!
        //! Set the maximum retention time of saved files.
        //! @param [in] age The maximum duration of a file, after being saved.
        //! When zero (the initial default), files are never deleted.
        //!
        template <class Rep, class Period>
        void setDeleteAfter(cn::duration<Rep,Period> age) { _delete_after = cn::duration_cast<cn::seconds>(age); }

        //!
        //! Force a cleanup of all files to delete.
        //! @param [in] current Hypothetical current UTC time. By default, use the current UTC time.
        //! This parameter can be used to simulate a later time and force deletion of files.
        //! @return True on success, false on error.
        //!
        bool cleanupOldFiles(const Time& current = Time::CurrentUTC());

        //!
        //! Save a file into the file tree.
        //! @param [in] content Binary content to save.
        //! @param [in] filename A relative path of the file to save.
        //! The path is sanitized first:
        //! - All invalid characters (for the current filesystem) are replaced with @a replacement.
        //! - All attempts to move upward in the file system hierarchy ("..") are removed.
        //! - If @a filename starts with an URI scheme, it is removed.
        //! - If the file path is absolute, it is relocated under the root directory of the file tree.
        //! @param [in] replacement The replacement character for invalid characters.
        //! @param [in] current Hypothetical current UTC time. By default, use the current UTC time.
        //! This parameter can be used to simulate a later time and force deletion of files.
        //! @return True on success, false on error.
        //!
        bool saveFile(const ByteBlock& content, const UString& filename, UChar replacement = '_', const Time& current = Time::CurrentUTC());

    private:
        Report&     _report;
        fs::path    _root {};                              // Root directory, where to save files.
        cn::seconds _delete_after {};                      // Delete files older than
        std::multimap<Time, fs::path> _deletion_queue {};  // Files to delete, indexed by deletion time.
    };
}
