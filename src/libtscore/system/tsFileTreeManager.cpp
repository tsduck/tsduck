//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFileTreeManager.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::FileTreeManager::FileTreeManager(Report& report) :
    _report(report)
{
}


//----------------------------------------------------------------------------
// Force a cleanup of all files to delete.
//----------------------------------------------------------------------------

bool ts::FileTreeManager::cleanupOldFiles(const Time& current)
{
    bool success = true;

    // Process and dequeue from deletion queue, up to current time.
    // If there is no "delete after" condition, the queue is empty from the beginning.
    // Always remove the processed entry from the queue. If the file was modified and not deleted, either its
    // deletion was enqueued a second time when modified, or its modification should not be deleted.
    for (auto it = _deletion_queue.begin();
         it != _deletion_queue.end() && it->first <= current;
         it = _deletion_queue.erase(it))
    {
        // Delete the corresponding file, if still existent and not modified in the meantime.
        const fs::path& filename(it->second);
        if (fs::exists(filename)) {
            const Time mod(GetFileModificationTimeUTC(filename));
            if (mod == Time::Epoch) {
                _report.error(u"error getting date of %s", filename);
                success = false;
            }
            else if (mod + _delete_after > current) {
                _report.debug(u"file %s was modified, not deleted", filename);
            }
            else if (!fs::remove(filename, &ErrCodeReport(_report, u"error deleting", filename))) {
                // Failed to remove the file.
                success = false;
            }
            else {
                _report.debug(u"file %d deleted (modified: %s, due: %s, current: %s)", filename, mod, it->first, current);
                // File was deleted. Also delete empty directories, up to root directory of this file tree.
                for (auto parent = filename.parent_path(); fs::is_empty(parent) && !fs::equivalent(parent, _root); parent = parent.parent_path()) {
                    if (!fs::remove(parent, &ErrCodeReport(_report, u"error deleting empty directory", parent))) {
                        success = false;
                        break;
                    }
                }
            }
        }
    }

    return success;
}


//----------------------------------------------------------------------------
// Save a file into the file tree.
//----------------------------------------------------------------------------

bool ts::FileTreeManager::saveFile(const ByteBlock& content, const UString& filename, UChar replacement, const Time& current)
{
    bool success = true;

    // We need a root directory.
    if (_root.empty()) {
        _report.error(u"no root directory specified, cannot save \"%s\"", filename);
        success = false;
    }
    else {
        // Build the output path. Remove URI scheme if present.
        UString path(filename);
        size_t sep = path.find(u"://");
        if (sep < path.length()) {
            path.erase(0, sep + 3);
        }

        // Replace forbidden characters with replacement characters.
#if defined(TS_WINDOWS)
        static const UString forbidden(u"()[]{}:");
#else
        static const UString forbidden(u"()[]{}");
#endif
        for (auto& c : path) {
            if (forbidden.contains(c)) {
                c = replacement;
            }
        }

        // Normalize directory separators.
        path.substitute('\\', '/');

        // Cleanup the file path to avoid directory traversal attack.
        UStringVector comp;
        path.split(comp, u'/', true, true);
        fs::path outpath(_root);
        fs::path basename;
        for (size_t i = 0; i < comp.size(); ++i) {
            if (comp[i] != u"." && comp[i] != u"..") {
                if (i + 1 < comp.size()) {
                    outpath /= comp[i];
                }
                else {
                    basename = comp[i];
                }
            }
        }

        // We need a base name for the file.
        if (basename.empty()) {
            _report.error(u"no base name specified in \"%s\"", filename);
            success = false;
        }
        else {
            // Create intermediate subdirectories if required.
            fs::create_directories(outpath, &ErrCodeReport(_report, u"error creating directory", outpath));

            // Save final file.
            outpath /= basename;
            _report.verbose(u"saving %s", outpath);
            success = content.saveToFile(outpath, &_report);

            // Adjust file modification date fo later deletion.
            if (success && _delete_after > cn::seconds::zero() && !SetFileModificationTimeUTC(outpath, current)) {
                success = false;
                _report.error(u"cannot update modification time of %s", outpath);
            }

            // Enqueue the file for deletion.
            if (success && _delete_after > cn::seconds::zero()) {
                _deletion_queue.insert(std::make_pair(current + _delete_after, outpath));
            }
        }
    }

    // Finally, cleanup old files.
    return cleanupOldFiles(current) && success;
}
