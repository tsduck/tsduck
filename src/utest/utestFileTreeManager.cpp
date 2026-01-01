//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for FileTreeManager class.
//
//----------------------------------------------------------------------------

#include "tsFileTreeManager.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class FileTreeManagerTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(FileTree);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    fs::path _temp_dir_name {};
};

TSUNIT_REGISTER(FileTreeManagerTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test initialization method.
void FileTreeManagerTest::beforeTest()
{
    if (_temp_dir_name.empty()) {
        _temp_dir_name = ts::TempFile();
    }
    fs::remove_all(_temp_dir_name, &ts::ErrCodeReport());
}

// Test cleanup method.
void FileTreeManagerTest::afterTest()
{
    fs::remove_all(_temp_dir_name, &ts::ErrCodeReport());
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(FileTree)
{
    ts::FileTreeManager ftm(CERR);

    // Create temporary root directory.
    fs::create_directory(_temp_dir_name, &ts::ErrCodeReport(CERR, u"error creating directory %s", _temp_dir_name));
    debug() << "TestFileTreeManager::FileTree: root: " << _temp_dir_name << std::endl;
    TSUNIT_ASSERT(fs::is_directory(_temp_dir_name));

    // Delete all files after 2 hours.
    ftm.setRootDirectory(_temp_dir_name);
    ftm.setDeleteAfter(cn::hours(2));

    // Binary content of all created files.
    const ts::ByteBlock content{1, 2, 3};

    // Create a few files in the file tree.
    TSUNIT_ASSERT(ftm.saveFile(content, u"a/b/c.foo"));
    TSUNIT_ASSERT(fs::exists(_temp_dir_name + u"/a/b/c.foo"));
    TSUNIT_EQUAL(3, fs::file_size(_temp_dir_name + u"/a/b/c.foo"));
    ts::ByteBlock actual;
    TSUNIT_ASSERT(actual.loadFromFile(_temp_dir_name + u"/a/b/c.foo"));
    TSUNIT_EQUAL(3, actual.size());
    TSUNIT_ASSERT(actual == content);

    TSUNIT_ASSERT(ftm.saveFile(content, u"/a/x/b[].foo"));
    TSUNIT_ASSERT(fs::exists(_temp_dir_name + u"/a/x/b__.foo"));

    TSUNIT_ASSERT(ftm.saveFile(content, u"http://a/z/x.foo"));
    TSUNIT_ASSERT(fs::exists(_temp_dir_name + u"/a/z/x.foo"));

    // Nothing should be deleted immediately.
    TSUNIT_ASSERT(ftm.cleanupOldFiles());
    TSUNIT_ASSERT(fs::exists(_temp_dir_name + u"/a/b/c.foo"));
    TSUNIT_ASSERT(fs::exists(_temp_dir_name + u"/a/x/b__.foo"));
    TSUNIT_ASSERT(fs::exists(_temp_dir_name + u"/a/z/x.foo"));

    // Rewrite the first one, pretend to be one hour later.
    TSUNIT_ASSERT(ftm.saveFile(content, u"a/b/c.foo", '_', ts::Time::CurrentUTC() + cn::hours(1)));
    TSUNIT_ASSERT(fs::exists(_temp_dir_name + u"/a/b/c.foo"));

    // File cleanup, pretend to be 2h 10mn later.
    // The first (rewritten) file is only 1h 10mn old and should stay.
    // Others must have been deleted.
    TSUNIT_ASSERT(ftm.cleanupOldFiles(ts::Time::CurrentUTC() + cn::hours(2) + cn::minutes(10)));
    TSUNIT_ASSERT(fs::exists(_temp_dir_name + u"/a/b/c.foo"));
    TSUNIT_ASSERT(!fs::exists(_temp_dir_name + u"/a/x/b__.foo"));
    TSUNIT_ASSERT(!fs::exists(_temp_dir_name + u"/a/z/x.foo"));
    TSUNIT_ASSERT(!fs::exists(_temp_dir_name + u"/a/x"));
    TSUNIT_ASSERT(!fs::exists(_temp_dir_name + u"/a/z"));

    // File cleanup, pretend to be 3h 10mn later.
    // All file should be deleted.
    TSUNIT_ASSERT(ftm.cleanupOldFiles(ts::Time::CurrentUTC() + cn::hours(3) + cn::minutes(10)));
    TSUNIT_ASSERT(!fs::exists(_temp_dir_name + u"/a/b/c.foo"));
    TSUNIT_ASSERT(!fs::exists(_temp_dir_name + u"/a/b"));
    TSUNIT_ASSERT(!fs::exists(_temp_dir_name + u"/a"));
    TSUNIT_ASSERT(fs::exists(_temp_dir_name));
}
