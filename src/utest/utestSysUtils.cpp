//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  CppUnit test suite for tsSysUtils.h
//
//----------------------------------------------------------------------------

#include "tsSysUtils.h"
#include "tsRegistry.h"
#include "tsMonotonic.h"
#include "tsTime.h"
#include "tsUID.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SysUtilsTest: public CppUnit::TestFixture
{
public:
    SysUtilsTest();

    virtual void setUp() override;
    virtual void tearDown() override;

    void testCurrentProcessId();
    void testCurrentExecutableFile();
    void testSleep();
    void testEnvironment();
    void testRegistry();
    void testIgnoreBrokenPipes();
    void testErrorCode();
    void testUid();
    void testVernacularFilePath();
    void testFilePaths();
    void testTempFiles();
    void testFileSize();
    void testFileTime();
    void testDirectory();
    void testWildcard();
    void testHomeDirectory();
    void testProcessMetrics();
    void testMemory();

    CPPUNIT_TEST_SUITE(SysUtilsTest);
    CPPUNIT_TEST(testCurrentProcessId);
    CPPUNIT_TEST(testCurrentExecutableFile);
    CPPUNIT_TEST(testSleep);
    CPPUNIT_TEST(testEnvironment);
    CPPUNIT_TEST(testRegistry);
    CPPUNIT_TEST(testIgnoreBrokenPipes);
    CPPUNIT_TEST(testErrorCode);
    CPPUNIT_TEST(testUid);
    CPPUNIT_TEST(testVernacularFilePath);
    CPPUNIT_TEST(testFilePaths);
    CPPUNIT_TEST(testTempFiles);
    CPPUNIT_TEST(testFileSize);
    CPPUNIT_TEST(testFileTime);
    CPPUNIT_TEST(testDirectory);
    CPPUNIT_TEST(testWildcard);
    CPPUNIT_TEST(testHomeDirectory);
    CPPUNIT_TEST(testProcessMetrics);
    CPPUNIT_TEST(testMemory);
    CPPUNIT_TEST_SUITE_END();
private:
    ts::NanoSecond  _nsPrecision;
    ts::MilliSecond _msPrecision;
};

CPPUNIT_TEST_SUITE_REGISTRATION(SysUtilsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
SysUtilsTest::SysUtilsTest() :
    _nsPrecision(0),
    _msPrecision(0)
{
}

// Test suite initialization method.
void SysUtilsTest::setUp()
{
    _nsPrecision = ts::Monotonic::SetPrecision(2 * ts::NanoSecPerMilliSec);
    _msPrecision = (_nsPrecision + ts::NanoSecPerMilliSec - 1) / ts::NanoSecPerMilliSec;

    // Request 2 milliseconds as system time precision.
    utest::Out() << "SysUtilsTest: timer precision = " << ts::UString::Decimal(_nsPrecision) << " ns, "
                 << ts::UString::Decimal(_msPrecision) << " ms" << std::endl;
}

// Test suite cleanup method.
void SysUtilsTest::tearDown()
{
}

// Vectors of strings
namespace {
    void Display(const ts::UString& title, const ts::UString& prefix, const ts::UStringVector& strings)
    {
        utest::Out() << "SysUtilsTest: " << title << std::endl;
        for (ts::UStringVector::const_iterator it = strings.begin(); it != strings.end(); ++it) {
            utest::Out() << "SysUtilsTest: " << prefix << "\"" << *it << "\"" << std::endl;
        }
    }
}

// Create a file with the specified size using standard C++ I/O.
// Return true on success, false on error.
namespace {
    bool _CreateFile(const ts::UString& name, size_t size)
    {
        ts::UString data(size, '-');
        std::ofstream file(name.toUTF8().c_str(), std::ios::binary);
        if (file) {
            file << data;
            file.close();
        }
        return !file.fail();
    }
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void SysUtilsTest::testCurrentProcessId()
{
    // Hard to make automated tests since we do not expect predictible values

    utest::Out() << "SysUtilsTest: sizeof(ts::ProcessId) = " << sizeof(ts::ProcessId) << std::endl
                 << "SysUtilsTest: ts::CurrentProcessId() = " << ts::CurrentProcessId() << std::endl;
}

void SysUtilsTest::testCurrentExecutableFile()
{
    // Hard to make automated tests since we do not expect a predictible executable name.

    ts::UString exe(ts::ExecutableFile());
    utest::Out() << "SysUtilsTest: ts::ExecutableFile() = \"" << exe << "\"" << std::endl;
    CPPUNIT_ASSERT(!exe.empty());
    CPPUNIT_ASSERT(ts::FileExists(exe));
}

void SysUtilsTest::testSleep()
{
    // The will slow down our test suites by 400 ms...

    const ts::Time before(ts::Time::CurrentUTC());
    ts::SleepThread(400);
    const ts::Time after(ts::Time::CurrentUTC());
    CPPUNIT_ASSERT(after >= before + 400 - _msPrecision);

    utest::Out() << "SysUtilsTest: ts::SleepThread(400), measured " << (after - before) << " ms" << std::endl;
}

void SysUtilsTest::testEnvironment()
{
    utest::Out() << "SysUtilsTest: EnvironmentExists(\"HOME\") = "
                 << ts::EnvironmentExists(u"HOME") << std::endl
                 << "SysUtilsTest: GetEnvironment(\"HOME\") = \""
                 << ts::GetEnvironment(u"HOME", u"(default)") << "\"" << std::endl
                 << "SysUtilsTest: EnvironmentExists(\"HOMEPATH\") = "
                 << ts::EnvironmentExists(u"HOMEPATH") << std::endl
                 << "SysUtilsTest: GetEnvironment(\"HOMEPATH\") = \""
                 << ts::GetEnvironment(u"HOMEPATH", u"(default)") << "\"" << std::endl;

    CPPUNIT_ASSERT(ts::SetEnvironment(u"UTEST_A", u"foo"));
    CPPUNIT_ASSERT(ts::EnvironmentExists(u"UTEST_A"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"foo", ts::GetEnvironment(u"UTEST_A"));
    CPPUNIT_ASSERT(ts::DeleteEnvironment(u"UTEST_A"));
    CPPUNIT_ASSERT(!ts::EnvironmentExists(u"UTEST_A"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", ts::GetEnvironment(u"UTEST_A"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"bar", ts::GetEnvironment(u"UTEST_A", u"bar"));

    // Very large value
    const ts::UString large(2000, 'x');
    ts::SetEnvironment(u"UTEST_A", large);
    CPPUNIT_ASSERT(ts::EnvironmentExists(u"UTEST_A"));
    CPPUNIT_ASSERT(ts::GetEnvironment(u"UTEST_A") == large);

    // Overwrite existing value
    ts::SetEnvironment(u"UTEST_A", u"azerty");
    CPPUNIT_ASSERT(ts::EnvironmentExists(u"UTEST_A"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"azerty", ts::GetEnvironment(u"UTEST_A"));

    // Analyze full environment
    ts::SetEnvironment(u"UTEST_A", u"123456789");
    ts::SetEnvironment(u"UTEST_B", u"abcdefghijklm");
    ts::SetEnvironment(u"UTEST_C", u"nopqrstuvwxyz");

    ts::Environment env;
    ts::GetEnvironment(env);

    for (ts::Environment::const_iterator it = env.begin(); it != env.end(); ++it) {
        utest::Out() << "SysUtilsTest: env: \"" << it->first << "\" = \"" << it->second << "\"" << std::endl;
    }

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"123456789",     env[u"UTEST_A"]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abcdefghijklm", env[u"UTEST_B"]);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"nopqrstuvwxyz", env[u"UTEST_C"]);

    // Search path
    ts::UStringVector ref;
    ref.push_back(u"azert/aze");
    ref.push_back(u"qsdsd f\\qdfqsd f");
    ref.push_back(u"fsdvsdf");
    ref.push_back(u"qs5veazr5--verv");

    ts::UString value(ref[0]);
    for (size_t i = 1; i < ref.size(); ++i) {
        value += ts::SearchPathSeparator + ref[i];
    }
    ts::SetEnvironment(u"UTEST_A", value);

    ts::UStringVector path;
    ts::GetEnvironmentPath(path, u"UTEST_A");
    CPPUNIT_ASSERT(path == ref);

    // Expand variables in a string
    CPPUNIT_ASSERT(ts::SetEnvironment(u"UTEST_A", u"123456789"));
    CPPUNIT_ASSERT(ts::SetEnvironment(u"UTEST_B", u"abcdefghijklm"));
    CPPUNIT_ASSERT(ts::SetEnvironment(u"UTEST_C", u"nopqrstuvwxyz"));
    ts::DeleteEnvironment(u"UTEST_D");

    utest::Out()
        << "SysUtilsTest: ExpandEnvironment(\"\\$UTEST_A\") = \""
        << ts::ExpandEnvironment(u"\\$UTEST_A") << "\"" << std::endl;

    CPPUNIT_ASSERT(ts::ExpandEnvironment(u"").empty());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc", ts::ExpandEnvironment(u"abc"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"123456789", ts::ExpandEnvironment(u"$UTEST_A"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"123456789", ts::ExpandEnvironment(u"${UTEST_A}"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"$UTEST_A", ts::ExpandEnvironment(u"\\$UTEST_A"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc123456789", ts::ExpandEnvironment(u"abc$UTEST_A"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc123456789abcdefghijklm123456789/qsd", ts::ExpandEnvironment(u"abc$UTEST_A$UTEST_B$UTEST_D$UTEST_A/qsd"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"abc123456789aabcdefghijklm123456789/qsd", ts::ExpandEnvironment(u"abc${UTEST_A}a$UTEST_B$UTEST_D$UTEST_A/qsd"));
}

void SysUtilsTest::testRegistry()
{
    utest::Out()
        << "SysUtilsTest: SystemEnvironmentKey = " << ts::Registry::SystemEnvironmentKey << std::endl
        << "SysUtilsTest: UserEnvironmentKey = " << ts::Registry::UserEnvironmentKey << std::endl;

#if defined(TS_WINDOWS)

    const ts::UString path(ts::Registry::GetValue(ts::Registry::SystemEnvironmentKey, u"Path"));
    utest::Out() << "SysUtilsTest: Path = " << path << std::endl;
    CPPUNIT_ASSERT(!path.empty());

    
    ts::Registry::Handle root;
    ts::UString subkey, endkey;
    CPPUNIT_ASSERT(ts::Registry::SplitKey(u"HKLM\\FOO\\BAR\\TOE", root, subkey));
    CPPUNIT_ASSERT(root == HKEY_LOCAL_MACHINE);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"FOO\\BAR\\TOE", subkey);

    CPPUNIT_ASSERT(ts::Registry::SplitKey(u"HKCU\\FOO1\\BAR1\\TOE1", root, subkey, endkey));
    CPPUNIT_ASSERT(root == HKEY_CURRENT_USER);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"FOO1\\BAR1", subkey);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"TOE1", endkey);

    CPPUNIT_ASSERT(!ts::Registry::SplitKey(u"HKFOO\\FOO1\\BAR1\\TOE1", root, subkey, endkey));

    const ts::UString key(ts::Registry::UserEnvironmentKey + u"\\UTEST_Z");

    CPPUNIT_ASSERT(ts::Registry::CreateKey(key, true));
    CPPUNIT_ASSERT(ts::Registry::SetValue(key, u"UTEST_X", u"VAL_X"));
    CPPUNIT_ASSERT(ts::Registry::SetValue(key, u"UTEST_Y", 47));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"VAL_X", ts::Registry::GetValue(key, u"UTEST_X"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"47", ts::Registry::GetValue(key, u"UTEST_Y"));
    CPPUNIT_ASSERT(ts::Registry::DeleteValue(key, u"UTEST_X"));
    CPPUNIT_ASSERT(ts::Registry::DeleteValue(key, u"UTEST_Y"));
    CPPUNIT_ASSERT(!ts::Registry::DeleteValue(key, u"UTEST_Y"));
    CPPUNIT_ASSERT(ts::Registry::DeleteKey(key));
    CPPUNIT_ASSERT(!ts::Registry::DeleteKey(key));

    CPPUNIT_ASSERT(ts::Registry::NotifySettingChange());
    CPPUNIT_ASSERT(ts::Registry::NotifyEnvironmentChange());

#else

    CPPUNIT_ASSERT(ts::Registry::GetValue(ts::Registry::SystemEnvironmentKey, u"Path").empty());
    CPPUNIT_ASSERT(!ts::Registry::SetValue(ts::Registry::UserEnvironmentKey, u"UTEST_X", u"VAL_X"));
    CPPUNIT_ASSERT(!ts::Registry::SetValue(ts::Registry::UserEnvironmentKey, u"UTEST_Y", 47));
    CPPUNIT_ASSERT(!ts::Registry::DeleteValue(ts::Registry::UserEnvironmentKey, u"UTEST_X"));
    CPPUNIT_ASSERT(!ts::Registry::CreateKey(ts::Registry::UserEnvironmentKey + u"\\UTEST_Z", true));
    CPPUNIT_ASSERT(!ts::Registry::DeleteKey(ts::Registry::UserEnvironmentKey + u"\\UTEST_Z"));
    CPPUNIT_ASSERT(!ts::Registry::NotifySettingChange());
    CPPUNIT_ASSERT(!ts::Registry::NotifyEnvironmentChange());

#endif
}

void SysUtilsTest::testIgnoreBrokenPipes()
{
    // Ignoring SIGPIPE may break up with some debuggers.
    // When running the unitary tests under a debugger, it may be useful
    // to define the environment variable NO_IGNORE_BROKEN_PIPES to
    // inhibit this test.

    if (ts::EnvironmentExists(u"NO_IGNORE_BROKEN_PIPES")) {
        utest::Out() << "SysUtilsTest: ignoring test case testIgnoreBrokenPipes" << std::endl;
    }
    else {

        ts::IgnorePipeSignal();

        // The previous line has effects on UNIX systems only.
        // Recreate a "broken pipe" situation on UNIX systems
        // and checks that we don't die.

#if defined(TS_UNIX)
        // Create a pipe
        int fd[2];
        CPPUNIT_ASSERT(::pipe(fd) == 0);
        // Close the reader end
        CPPUNIT_ASSERT(::close(fd[0]) == 0);
        // Write to pipe, assert error (but no process kill)
        char data[] = "azerty";
        const ::ssize_t ret = ::write(fd[1], data, sizeof(data));
        const int err = errno;
        CPPUNIT_ASSERT(ret == -1);
        CPPUNIT_ASSERT(err == EPIPE);
        // Close the writer end
        CPPUNIT_ASSERT(::close(fd[1]) == 0);
#endif
    }
}

void SysUtilsTest::testErrorCode()
{
    // Hard to make automated tests since we do not expect portable strings

    const ts::ErrorCode code =
#if defined(TS_WINDOWS)
        WAIT_TIMEOUT;
#elif defined(TS_UNIX)
        ETIMEDOUT;
#else
        0;
#endif

    const ts::UString codeMessage(ts::ErrorCodeMessage(code));
    const ts::UString successMessage(ts::ErrorCodeMessage(ts::SYS_SUCCESS));

    utest::Out()
        << "SysUtilsTest: sizeof(ts::ErrorCode) = " << sizeof(ts::ErrorCode) << std::endl
        << "SysUtilsTest: ts::SYS_SUCCESS = " << ts::SYS_SUCCESS << std::endl
        << "SysUtilsTest: SUCCESS message = \"" << successMessage << "\"" << std::endl
        << "SysUtilsTest: test code = " << code << std::endl
        << "SysUtilsTest: test code message = \"" << codeMessage << "\"" << std::endl;

    CPPUNIT_ASSERT(!codeMessage.empty());
    CPPUNIT_ASSERT(!successMessage.empty());
}

void SysUtilsTest::testUid()
{
    utest::Out() << "SysUtilsTest: newUid() = 0x" << ts::UString::Hexa(ts::UID::Instance()->newUID()) << std::endl;

    CPPUNIT_ASSERT(ts::UID::Instance()->newUID() != ts::UID::Instance()->newUID());
    CPPUNIT_ASSERT(ts::UID::Instance()->newUID() != ts::UID::Instance()->newUID());
    CPPUNIT_ASSERT(ts::UID::Instance()->newUID() != ts::UID::Instance()->newUID());
}

void SysUtilsTest::testVernacularFilePath()
{
#if defined(TS_WINDOWS)
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"C:\\alpha\\beta\\gamma", ts::VernacularFilePath(u"C:\\alpha/beta\\gamma"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"D:\\alpha\\beta\\gamma", ts::VernacularFilePath(u"/d/alpha/beta/gamma"));
#elif defined(TS_UNIX)
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"C:/alpha/beta/gamma", ts::VernacularFilePath(u"C:\\alpha/beta\\gamma"));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"/alpha-beta/gamma", ts::VernacularFilePath(u"/alpha-beta/gamma"));
#endif
}

void SysUtilsTest::testFilePaths()
{
    const ts::UString dir(ts::VernacularFilePath(u"/dir/for/this.test"));
    const ts::UString sep(1, ts::PathSeparator);
    const ts::UString dirSep(dir + ts::PathSeparator);

    CPPUNIT_ASSERT(ts::DirectoryName(dirSep + u"foo.bar") == dir);
    CPPUNIT_ASSERT(ts::DirectoryName(u"foo.bar") == u".");
    CPPUNIT_ASSERT(ts::DirectoryName(sep + u"foo.bar") == sep);

    CPPUNIT_ASSERT(ts::BaseName(dirSep + u"foo.bar") == u"foo.bar");
    CPPUNIT_ASSERT(ts::BaseName(dirSep) == u"");

    CPPUNIT_ASSERT(ts::PathSuffix(dirSep + u"foo.bar") == u".bar");
    CPPUNIT_ASSERT(ts::PathSuffix(dirSep + u"foo.") == u".");
    CPPUNIT_ASSERT(ts::PathSuffix(dirSep + u"foo") == u"");

    CPPUNIT_ASSERT(ts::AddPathSuffix(dirSep + u"foo", u".none") == dirSep + u"foo.none");
    CPPUNIT_ASSERT(ts::AddPathSuffix(dirSep + u"foo.", u".none") == dirSep + u"foo.");
    CPPUNIT_ASSERT(ts::AddPathSuffix(dirSep + u"foo.bar", u".none") == dirSep + u"foo.bar");

    CPPUNIT_ASSERT(ts::PathPrefix(dirSep + u"foo.bar") == dirSep + u"foo");
    CPPUNIT_ASSERT(ts::PathPrefix(dirSep + u"foo.") == dirSep + u"foo");
    CPPUNIT_ASSERT(ts::PathPrefix(dirSep + u"foo") == dirSep + u"foo");
}

void SysUtilsTest::testTempFiles()
{
    utest::Out() << "SysUtilsTest: TempDirectory() = \"" << ts::TempDirectory() << "\"" << std::endl;
    utest::Out() << "SysUtilsTest: TempFile() = \"" << ts::TempFile() << "\"" << std::endl;
    utest::Out() << "SysUtilsTest: TempFile(\".foo\") = \"" << ts::TempFile(u".foo") << "\"" << std::endl;

    // Check that the temporary directory exists
    CPPUNIT_ASSERT(ts::IsDirectory(ts::TempDirectory()));

    // Check that temporary files are in this directory
    const ts::UString tmpName(ts::TempFile());
    CPPUNIT_ASSERT(ts::DirectoryName(tmpName) == ts::TempDirectory());

    // Check that we are allowed to create temporary files.
    CPPUNIT_ASSERT(!ts::FileExists(tmpName));
    CPPUNIT_ASSERT(_CreateFile(tmpName, 0));
    CPPUNIT_ASSERT(ts::FileExists(tmpName));
    CPPUNIT_ASSERT(ts::GetFileSize(tmpName) == 0);
    CPPUNIT_ASSERT(ts::DeleteFile(tmpName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists(tmpName));
}

void SysUtilsTest::testFileSize()
{
    const ts::UString tmpName(ts::TempFile());
    CPPUNIT_ASSERT(!ts::FileExists(tmpName));

    // Create a file
    CPPUNIT_ASSERT(_CreateFile(tmpName, 1234));
    CPPUNIT_ASSERT(ts::FileExists(tmpName));
    CPPUNIT_ASSERT(ts::GetFileSize(tmpName) == 1234);

    CPPUNIT_ASSERT(ts::TruncateFile(tmpName, 567) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::GetFileSize(tmpName) == 567);

    const ts::UString tmpName2(ts::TempFile());
    CPPUNIT_ASSERT(!ts::FileExists(tmpName2));
    CPPUNIT_ASSERT(ts::RenameFile(tmpName, tmpName2) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::FileExists(tmpName2));
    CPPUNIT_ASSERT(!ts::FileExists(tmpName));
    CPPUNIT_ASSERT(ts::GetFileSize(tmpName2) == 567);

    CPPUNIT_ASSERT(ts::DeleteFile(tmpName2) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists(tmpName2));
}

void SysUtilsTest::testFileTime()
{
    const ts::UString tmpName(ts::TempFile());

    const ts::Time before(ts::Time::CurrentUTC());
    CPPUNIT_ASSERT(_CreateFile(tmpName, 0));
    const ts::Time after(ts::Time::CurrentUTC());

    // Some systems may not store the milliseconds in the file time.
    // So we use "before" without milliseconds.
    ts::Time::Fields beforeFields(before);
    beforeFields.millisecond = 0;
    const ts::Time beforeBase(beforeFields);

    CPPUNIT_ASSERT(ts::FileExists(tmpName));
    const ts::Time fileUtc(ts::GetFileModificationTimeUTC(tmpName));
    const ts::Time fileLocal(ts::GetFileModificationTimeLocal(tmpName));

    utest::Out()
        << "SysUtilsTest: file: " << tmpName << std::endl
        << "SysUtilsTest:      before:     " << before << std::endl
        << "SysUtilsTest:      before/ms:  " << beforeBase << std::endl
        << "SysUtilsTest:      file UTC:   " << fileUtc << std::endl
        << "SysUtilsTest:      after:      " << after << std::endl
        << "SysUtilsTest:      file local: " << fileLocal << std::endl;

    // Check that file modification occured between before and after.
    // Some systems may not store the milliseconds in the file time.
    // So we use before without milliseconds

    CPPUNIT_ASSERT(beforeBase <= fileUtc);
    CPPUNIT_ASSERT(fileUtc <= after);
    CPPUNIT_ASSERT(fileUtc.UTCToLocal() == fileLocal);
    CPPUNIT_ASSERT(fileLocal.localToUTC() == fileUtc);

    CPPUNIT_ASSERT(ts::DeleteFile(tmpName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists(tmpName));
}

void SysUtilsTest::testDirectory()
{
    const ts::UString dirName(ts::TempFile(u""));
    const ts::UString sep(1, ts::PathSeparator);
    const ts::UString fileName(sep + u"foo.bar");

    CPPUNIT_ASSERT(!ts::FileExists(dirName));
    CPPUNIT_ASSERT(ts::CreateDirectory(dirName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::FileExists(dirName));
    CPPUNIT_ASSERT(ts::IsDirectory(dirName));

    CPPUNIT_ASSERT(_CreateFile(dirName + fileName, 0));
    CPPUNIT_ASSERT(ts::FileExists(dirName + fileName));
    CPPUNIT_ASSERT(!ts::IsDirectory(dirName + fileName));

    const ts::UString dirName2(ts::TempFile(u""));
    CPPUNIT_ASSERT(!ts::FileExists(dirName2));
    CPPUNIT_ASSERT(ts::RenameFile(dirName, dirName2) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::FileExists(dirName2));
    CPPUNIT_ASSERT(ts::IsDirectory(dirName2));
    CPPUNIT_ASSERT(!ts::FileExists(dirName));
    CPPUNIT_ASSERT(!ts::IsDirectory(dirName));
    CPPUNIT_ASSERT(ts::FileExists(dirName2 + fileName));
    CPPUNIT_ASSERT(!ts::IsDirectory(dirName2 + fileName));

    CPPUNIT_ASSERT(ts::DeleteFile(dirName2 + fileName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists(dirName2 + fileName));
    CPPUNIT_ASSERT(ts::IsDirectory(dirName2));

    CPPUNIT_ASSERT(ts::DeleteFile(dirName2) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists(dirName2));
    CPPUNIT_ASSERT(!ts::IsDirectory(dirName2));
}

void SysUtilsTest::testWildcard()
{
    const ts::UString dirName(ts::TempFile(u""));
    const ts::UString filePrefix(dirName + ts::PathSeparator + u"foo.");
    const size_t count = 10;

    // Create temporary directory
    CPPUNIT_ASSERT(ts::CreateDirectory(dirName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::IsDirectory(dirName));

    // Create one file with unique pattern
    const ts::UString spuriousFileName(dirName + ts::PathSeparator + u"tagada");
    CPPUNIT_ASSERT(_CreateFile(spuriousFileName, 0));
    CPPUNIT_ASSERT(ts::FileExists(spuriousFileName));

    // Create many files
    ts::UStringVector fileNames;
    fileNames.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const ts::UString fileName(filePrefix + ts::UString::Format(u"%03d", {i}));
        CPPUNIT_ASSERT(_CreateFile(fileName, 0));
        CPPUNIT_ASSERT(ts::FileExists(fileName));
        fileNames.push_back(fileName);
    }
    Display(u"created files:", u"file: ", fileNames);

    // Get wildcard
    ts::UStringVector expanded;
    CPPUNIT_ASSERT(ts::ExpandWildcard(expanded, filePrefix + u"*"));
    std::sort(expanded.begin(), expanded.end());
    Display(u"expanded wildcard:", u"expanded: ", expanded);
    CPPUNIT_ASSERT(expanded == fileNames);

    // Final cleanup
    for (ts::UStringVector::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
        CPPUNIT_ASSERT(ts::DeleteFile(*it) == ts::SYS_SUCCESS);
        CPPUNIT_ASSERT(!ts::FileExists(*it));
    }
    CPPUNIT_ASSERT(ts::DeleteFile(spuriousFileName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists(spuriousFileName));
    CPPUNIT_ASSERT(ts::DeleteFile(dirName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists(dirName));
}

void SysUtilsTest::testHomeDirectory()
{
    const ts::UString dir(ts::UserHomeDirectory());
    utest::Out() << "SysUtilsTest: UserHomeDirectory() = \"" << dir << "\"" << std::endl;

    CPPUNIT_ASSERT(!dir.empty());
    CPPUNIT_ASSERT(ts::FileExists(dir));
    CPPUNIT_ASSERT(ts::IsDirectory(dir));
}

void SysUtilsTest::testProcessMetrics()
{
    ts::ProcessMetrics pm1;
    CPPUNIT_ASSERT(pm1.cpu_time == -1);
    CPPUNIT_ASSERT(pm1.vmem_size == 0);

    ts::GetProcessMetrics(pm1);
    utest::Out() << "ProcessMetricsTest: CPU time (1) = " << pm1.cpu_time << " ms" << std::endl
                 << "ProcessMetricsTest: virtual memory (1) = " << pm1.vmem_size << " bytes" << std::endl;

    CPPUNIT_ASSERT(pm1.cpu_time >= 0);
    CPPUNIT_ASSERT(pm1.vmem_size > 0);

    // Consume some milliseconds of CPU time
    uint64_t counter = 7;
    for (uint64_t i = 0; i < 10000000L; ++i) {
        counter = counter * counter;
    }

    ts::ProcessMetrics pm2;
    ts::GetProcessMetrics(pm2);
    utest::Out() << "ProcessMetricsTest: CPU time (2) = " << pm2.cpu_time << " ms" << std::endl
                 << "ProcessMetricsTest: virtual memory (2) = " << pm2.vmem_size << " bytes" << std::endl;

    CPPUNIT_ASSERT(pm2.cpu_time >= 0);
    CPPUNIT_ASSERT(pm2.cpu_time >= pm1.cpu_time);
    CPPUNIT_ASSERT(pm2.vmem_size > 0);
}

void SysUtilsTest::testMemory()
{
    // We can't predict the memory page size, except that it must be a multiple of 256.
    utest::Out() << "SysUtilsTest: MemoryPageSize() = " << ts::MemoryPageSize() << " bytes" << std::endl;
    CPPUNIT_ASSERT(ts::MemoryPageSize() > 0);
    CPPUNIT_ASSERT(ts::MemoryPageSize() % 256 == 0);
}
