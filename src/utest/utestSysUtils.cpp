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
//
//  TSUnit test suite for tsSysUtils.h and tsFileUtils.h
//
//----------------------------------------------------------------------------

#include "tsFileUtils.h"
#include "tsSysUtils.h"
#include "tsSysInfo.h"
#include "tsRegistry.h"
#include "tsMonotonic.h"
#include "tsTime.h"
#include "tsUID.h"
#include "tsunit.h"

#if defined(TS_WINDOWS)
#include "tsWinUtils.h"
#endif


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SysUtilsTest: public tsunit::Test
{
public:
    SysUtilsTest();

    virtual void beforeTest() override;
    virtual void afterTest() override;

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
    void testSearchWildcard();
    void testHomeDirectory();
    void testProcessMetrics();
    void testIsTerminal();
    void testSysInfo();
    void testSymLinks();
    void testCurrentWorkingDirectory();
    void testIsAbsoluteFilePath();
    void testAbsoluteFilePath();
    void testCleanupFilePath();
    void testRelativeFilePath();

    TSUNIT_TEST_BEGIN(SysUtilsTest);
    TSUNIT_TEST(testCurrentProcessId);
    TSUNIT_TEST(testCurrentExecutableFile);
    TSUNIT_TEST(testSleep);
    TSUNIT_TEST(testEnvironment);
    TSUNIT_TEST(testRegistry);
    TSUNIT_TEST(testIgnoreBrokenPipes);
    TSUNIT_TEST(testErrorCode);
    TSUNIT_TEST(testUid);
    TSUNIT_TEST(testVernacularFilePath);
    TSUNIT_TEST(testFilePaths);
    TSUNIT_TEST(testTempFiles);
    TSUNIT_TEST(testFileSize);
    TSUNIT_TEST(testFileTime);
    TSUNIT_TEST(testDirectory);
    TSUNIT_TEST(testWildcard);
    TSUNIT_TEST(testSearchWildcard);
    TSUNIT_TEST(testHomeDirectory);
    TSUNIT_TEST(testProcessMetrics);
    TSUNIT_TEST(testIsTerminal);
    TSUNIT_TEST(testSysInfo);
    TSUNIT_TEST(testSymLinks);
    TSUNIT_TEST(testCurrentWorkingDirectory);
    TSUNIT_TEST(testIsAbsoluteFilePath);
    TSUNIT_TEST(testAbsoluteFilePath);
    TSUNIT_TEST(testCleanupFilePath);
    TSUNIT_TEST(testRelativeFilePath);
    TSUNIT_TEST_END();

private:
    ts::NanoSecond  _nsPrecision;
    ts::MilliSecond _msPrecision;
};

TSUNIT_REGISTER(SysUtilsTest);


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
void SysUtilsTest::beforeTest()
{
    _nsPrecision = ts::Monotonic::SetPrecision(2 * ts::NanoSecPerMilliSec);
    _msPrecision = (_nsPrecision + ts::NanoSecPerMilliSec - 1) / ts::NanoSecPerMilliSec;

    // Request 2 milliseconds as system time precision.
    debug() << "SysUtilsTest: timer precision = " << ts::UString::Decimal(_nsPrecision) << " ns, "
            << ts::UString::Decimal(_msPrecision) << " ms" << std::endl;
}

// Test suite cleanup method.
void SysUtilsTest::afterTest()
{
}

// Vectors of strings
namespace {
    void Display(const ts::UString& title, const ts::UString& prefix, const ts::UStringVector& strings)
    {
        tsunit::Test::debug() << "SysUtilsTest: " << title << std::endl;
        for (const auto& str : strings) {
            tsunit::Test::debug() << "SysUtilsTest: " << prefix << "\"" << str << "\"" << std::endl;
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

    debug() << "SysUtilsTest: sizeof(ts::ProcessId) = " << sizeof(ts::ProcessId) << std::endl
            << "SysUtilsTest: ts::CurrentProcessId() = " << ts::CurrentProcessId() << std::endl
            << "SysUtilsTest: ts::IsPrivilegedUser() = " << ts::IsPrivilegedUser() << std::endl;
}

void SysUtilsTest::testCurrentExecutableFile()
{
    // Hard to make automated tests since we do not expect a predictible executable name.

    ts::UString exe(ts::ExecutableFile());
    debug() << "SysUtilsTest: ts::ExecutableFile() = \"" << exe << "\"" << std::endl;
    TSUNIT_ASSERT(!exe.empty());
    TSUNIT_ASSERT(ts::FileExists(exe));
}

void SysUtilsTest::testSleep()
{
    // The will slow down our test suites by 400 ms...

    const ts::Time before(ts::Time::CurrentUTC());
    ts::SleepThread(400);
    const ts::Time after(ts::Time::CurrentUTC());
    TSUNIT_ASSERT(after >= before + 400 - _msPrecision);

    debug() << "SysUtilsTest: ts::SleepThread(400), measured " << (after - before) << " ms" << std::endl;
}

void SysUtilsTest::testEnvironment()
{
    debug() << "SysUtilsTest: EnvironmentExists(\"HOME\") = "
            << ts::EnvironmentExists(u"HOME") << std::endl
            << "SysUtilsTest: GetEnvironment(\"HOME\") = \""
            << ts::GetEnvironment(u"HOME", u"(default)") << "\"" << std::endl
            << "SysUtilsTest: EnvironmentExists(\"HOMEPATH\") = "
            << ts::EnvironmentExists(u"HOMEPATH") << std::endl
            << "SysUtilsTest: GetEnvironment(\"HOMEPATH\") = \""
            << ts::GetEnvironment(u"HOMEPATH", u"(default)") << "\"" << std::endl;

    TSUNIT_ASSERT(ts::SetEnvironment(u"UTEST_A", u"foo"));
    TSUNIT_ASSERT(ts::EnvironmentExists(u"UTEST_A"));
    TSUNIT_EQUAL(u"foo", ts::GetEnvironment(u"UTEST_A"));
    TSUNIT_ASSERT(ts::DeleteEnvironment(u"UTEST_A"));
    TSUNIT_ASSERT(!ts::EnvironmentExists(u"UTEST_A"));
    TSUNIT_EQUAL(u"", ts::GetEnvironment(u"UTEST_A"));
    TSUNIT_EQUAL(u"bar", ts::GetEnvironment(u"UTEST_A", u"bar"));

    // Very large value
    const ts::UString large(2000, 'x');
    ts::SetEnvironment(u"UTEST_A", large);
    TSUNIT_ASSERT(ts::EnvironmentExists(u"UTEST_A"));
    TSUNIT_ASSERT(ts::GetEnvironment(u"UTEST_A") == large);

    // Overwrite existing value
    ts::SetEnvironment(u"UTEST_A", u"azerty");
    TSUNIT_ASSERT(ts::EnvironmentExists(u"UTEST_A"));
    TSUNIT_EQUAL(u"azerty", ts::GetEnvironment(u"UTEST_A"));

    // Analyze full environment
    ts::SetEnvironment(u"UTEST_A", u"123456789");
    ts::SetEnvironment(u"UTEST_B", u"abcdefghijklm");
    ts::SetEnvironment(u"UTEST_C", u"nopqrstuvwxyz");

    ts::Environment env;
    ts::GetEnvironment(env);

    for (const auto& it : env) {
        debug() << "SysUtilsTest: env: \"" << it.first << "\" = \"" << it.second << "\"" << std::endl;
    }

    TSUNIT_EQUAL(u"123456789",     env[u"UTEST_A"]);
    TSUNIT_EQUAL(u"abcdefghijklm", env[u"UTEST_B"]);
    TSUNIT_EQUAL(u"nopqrstuvwxyz", env[u"UTEST_C"]);

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
    TSUNIT_ASSERT(path == ref);

    // Expand variables in a string
    TSUNIT_ASSERT(ts::SetEnvironment(u"UTEST_A", u"123456789"));
    TSUNIT_ASSERT(ts::SetEnvironment(u"UTEST_B", u"abcdefghijklm"));
    TSUNIT_ASSERT(ts::SetEnvironment(u"UTEST_C", u"nopqrstuvwxyz"));
    ts::DeleteEnvironment(u"UTEST_D");

    debug() << "SysUtilsTest: ExpandEnvironment(\"\\$UTEST_A\") = \""
            << ts::ExpandEnvironment(u"\\$UTEST_A") << "\"" << std::endl;

    TSUNIT_ASSERT(ts::ExpandEnvironment(u"").empty());
    TSUNIT_EQUAL(u"abc", ts::ExpandEnvironment(u"abc"));
    TSUNIT_EQUAL(u"123456789", ts::ExpandEnvironment(u"$UTEST_A"));
    TSUNIT_EQUAL(u"123456789", ts::ExpandEnvironment(u"${UTEST_A}"));
    TSUNIT_EQUAL(u"$UTEST_A", ts::ExpandEnvironment(u"\\$UTEST_A"));
    TSUNIT_EQUAL(u"abc123456789", ts::ExpandEnvironment(u"abc$UTEST_A"));
    TSUNIT_EQUAL(u"abc123456789abcdefghijklm123456789/qsd", ts::ExpandEnvironment(u"abc$UTEST_A$UTEST_B$UTEST_D$UTEST_A/qsd"));
    TSUNIT_EQUAL(u"abc123456789aabcdefghijklm123456789/qsd", ts::ExpandEnvironment(u"abc${UTEST_A}a$UTEST_B$UTEST_D$UTEST_A/qsd"));
}

void SysUtilsTest::testRegistry()
{
    debug() << "SysUtilsTest: SystemEnvironmentKey = " << ts::Registry::SystemEnvironmentKey << std::endl
            << "SysUtilsTest: UserEnvironmentKey = " << ts::Registry::UserEnvironmentKey << std::endl;

#if defined(TS_WINDOWS)

    const ts::UString path(ts::Registry::GetValue(ts::Registry::SystemEnvironmentKey, u"Path"));
    debug() << "SysUtilsTest: Path = " << path << std::endl;
    TSUNIT_ASSERT(!path.empty());


    ts::Registry::Handle root;
    ts::UString subkey, endkey;
    TSUNIT_ASSERT(ts::Registry::SplitKey(u"HKLM\\FOO\\BAR\\TOE", root, subkey));
    TSUNIT_ASSERT(root == HKEY_LOCAL_MACHINE);
    TSUNIT_EQUAL(u"FOO\\BAR\\TOE", subkey);

    TSUNIT_ASSERT(ts::Registry::SplitKey(u"HKCU\\FOO1\\BAR1\\TOE1", root, subkey, endkey));
    TSUNIT_ASSERT(root == HKEY_CURRENT_USER);
    TSUNIT_EQUAL(u"FOO1\\BAR1", subkey);
    TSUNIT_EQUAL(u"TOE1", endkey);

    TSUNIT_ASSERT(!ts::Registry::SplitKey(u"HKFOO\\FOO1\\BAR1\\TOE1", root, subkey, endkey));

    const ts::UString key(ts::Registry::UserEnvironmentKey + u"\\UTEST_Z");

    TSUNIT_ASSERT(ts::Registry::CreateKey(key, true));
    TSUNIT_ASSERT(ts::Registry::SetValue(key, u"UTEST_X", u"VAL_X"));
    TSUNIT_ASSERT(ts::Registry::SetValue(key, u"UTEST_Y", 47));
    TSUNIT_EQUAL(u"VAL_X", ts::Registry::GetValue(key, u"UTEST_X"));
    TSUNIT_EQUAL(u"47", ts::Registry::GetValue(key, u"UTEST_Y"));
    TSUNIT_ASSERT(ts::Registry::DeleteValue(key, u"UTEST_X"));
    TSUNIT_ASSERT(ts::Registry::DeleteValue(key, u"UTEST_Y"));
    TSUNIT_ASSERT(!ts::Registry::DeleteValue(key, u"UTEST_Y"));
    TSUNIT_ASSERT(ts::Registry::DeleteKey(key));
    TSUNIT_ASSERT(!ts::Registry::DeleteKey(key));

    TSUNIT_ASSERT(ts::Registry::NotifySettingChange());
    TSUNIT_ASSERT(ts::Registry::NotifyEnvironmentChange());

#else

    TSUNIT_ASSERT(ts::Registry::GetValue(ts::Registry::SystemEnvironmentKey, u"Path").empty());
    TSUNIT_ASSERT(!ts::Registry::SetValue(ts::Registry::UserEnvironmentKey, u"UTEST_X", u"VAL_X"));
    TSUNIT_ASSERT(!ts::Registry::SetValue(ts::Registry::UserEnvironmentKey, u"UTEST_Y", 47));
    TSUNIT_ASSERT(!ts::Registry::DeleteValue(ts::Registry::UserEnvironmentKey, u"UTEST_X"));
    TSUNIT_ASSERT(!ts::Registry::CreateKey(ts::Registry::UserEnvironmentKey + u"\\UTEST_Z", true));
    TSUNIT_ASSERT(!ts::Registry::DeleteKey(ts::Registry::UserEnvironmentKey + u"\\UTEST_Z"));
    TSUNIT_ASSERT(!ts::Registry::NotifySettingChange());
    TSUNIT_ASSERT(!ts::Registry::NotifyEnvironmentChange());

#endif
}

void SysUtilsTest::testIgnoreBrokenPipes()
{
    // Ignoring SIGPIPE may break up with some debuggers.
    // When running the unitary tests under a debugger, it may be useful
    // to define the environment variable NO_IGNORE_BROKEN_PIPES to
    // inhibit this test.

    if (ts::EnvironmentExists(u"NO_IGNORE_BROKEN_PIPES")) {
        debug() << "SysUtilsTest: ignoring test case testIgnoreBrokenPipes" << std::endl;
    }
    else {

        ts::IgnorePipeSignal();

        // The previous line has effects on UNIX systems only.
        // Recreate a "broken pipe" situation on UNIX systems
        // and checks that we don't die.

#if defined(TS_UNIX)
        // Create a pipe
        int fd[2];
        TSUNIT_ASSERT(::pipe(fd) == 0);
        // Close the reader end
        TSUNIT_ASSERT(::close(fd[0]) == 0);
        // Write to pipe, assert error (but no process kill)
        char data[] = "azerty";
        const ::ssize_t ret = ::write(fd[1], data, sizeof(data));
        const int err = errno;
        TSUNIT_ASSERT(ret == -1);
        TSUNIT_ASSERT(err == EPIPE);
        // Close the writer end
        TSUNIT_ASSERT(::close(fd[1]) == 0);
#endif
    }
}

void SysUtilsTest::testErrorCode()
{
    // Hard to make automated tests since we do not expect portable strings

    const ts::SysErrorCode code =
#if defined(TS_WINDOWS)
        WAIT_TIMEOUT;
#elif defined(TS_UNIX)
        ETIMEDOUT;
#else
        0;
#endif

    const ts::UString codeMessage(ts::SysErrorCodeMessage(code));
    const ts::UString successMessage(ts::SysErrorCodeMessage(ts::SYS_SUCCESS));

    debug() << "SysUtilsTest: sizeof(ts::SysErrorCode) = " << sizeof(ts::SysErrorCode) << std::endl
            << "SysUtilsTest: ts::SYS_SUCCESS = " << ts::SYS_SUCCESS << std::endl
            << "SysUtilsTest: SUCCESS message = \"" << successMessage << "\"" << std::endl
            << "SysUtilsTest: test code = " << code << std::endl
            << "SysUtilsTest: test code message = \"" << codeMessage << "\"" << std::endl;

    TSUNIT_ASSERT(!codeMessage.empty());
    TSUNIT_ASSERT(!successMessage.empty());
}

void SysUtilsTest::testUid()
{
    debug() << "SysUtilsTest: newUid() = 0x" << ts::UString::Hexa(ts::UID::Instance()->newUID()) << std::endl;

    TSUNIT_ASSERT(ts::UID::Instance()->newUID() != ts::UID::Instance()->newUID());
    TSUNIT_ASSERT(ts::UID::Instance()->newUID() != ts::UID::Instance()->newUID());
    TSUNIT_ASSERT(ts::UID::Instance()->newUID() != ts::UID::Instance()->newUID());
}

void SysUtilsTest::testVernacularFilePath()
{
#if defined(TS_WINDOWS)
    TSUNIT_EQUAL(u"C:\\alpha\\beta\\gamma", ts::VernacularFilePath(u"C:\\alpha/beta\\gamma"));
    TSUNIT_EQUAL(u"D:\\alpha\\beta\\gamma", ts::VernacularFilePath(u"/d/alpha/beta/gamma"));
    TSUNIT_EQUAL(u"D:\\alpha", ts::VernacularFilePath(u"/mnt/d/alpha"));
    TSUNIT_EQUAL(u"D:\\", ts::VernacularFilePath(u"/mnt/d"));
    TSUNIT_EQUAL(u"D:\\alpha", ts::VernacularFilePath(u"/cygdrive/d/alpha"));
    TSUNIT_EQUAL(u"D:\\", ts::VernacularFilePath(u"/cygdrive/d"));
    TSUNIT_EQUAL(u"D:\\alpha", ts::VernacularFilePath(u"/d/alpha"));
    TSUNIT_EQUAL(u"D:\\", ts::VernacularFilePath(u"/d"));
#elif defined(TS_UNIX)
    TSUNIT_EQUAL(u"C:/alpha/beta/gamma", ts::VernacularFilePath(u"C:\\alpha/beta\\gamma"));
    TSUNIT_EQUAL(u"/alpha-beta/gamma", ts::VernacularFilePath(u"/alpha-beta/gamma"));
#endif
}

void SysUtilsTest::testFilePaths()
{
    const ts::UString dir(ts::VernacularFilePath(u"/dir/for/this.test"));
    const ts::UString sep(1, ts::PathSeparator);
    const ts::UString dirSep(dir + ts::PathSeparator);

    TSUNIT_ASSERT(ts::DirectoryName(dirSep + u"foo.bar") == dir);
    TSUNIT_ASSERT(ts::DirectoryName(u"foo.bar") == u".");
    TSUNIT_ASSERT(ts::DirectoryName(sep + u"foo.bar") == sep);

    TSUNIT_ASSERT(ts::BaseName(dirSep + u"foo.bar") == u"foo.bar");
    TSUNIT_ASSERT(ts::BaseName(dirSep) == u"");

    TSUNIT_ASSERT(ts::PathSuffix(dirSep + u"foo.bar") == u".bar");
    TSUNIT_ASSERT(ts::PathSuffix(dirSep + u"foo.") == u".");
    TSUNIT_ASSERT(ts::PathSuffix(dirSep + u"foo") == u"");

    TSUNIT_ASSERT(ts::AddPathSuffix(dirSep + u"foo", u".none") == dirSep + u"foo.none");
    TSUNIT_ASSERT(ts::AddPathSuffix(dirSep + u"foo.", u".none") == dirSep + u"foo.");
    TSUNIT_ASSERT(ts::AddPathSuffix(dirSep + u"foo.bar", u".none") == dirSep + u"foo.bar");

    TSUNIT_ASSERT(ts::PathPrefix(dirSep + u"foo.bar") == dirSep + u"foo");
    TSUNIT_ASSERT(ts::PathPrefix(dirSep + u"foo.") == dirSep + u"foo");
    TSUNIT_ASSERT(ts::PathPrefix(dirSep + u"foo") == dirSep + u"foo");
}

void SysUtilsTest::testTempFiles()
{
    debug() << "SysUtilsTest: TempDirectory() = \"" << ts::TempDirectory() << "\"" << std::endl;
    debug() << "SysUtilsTest: TempFile() = \"" << ts::TempFile() << "\"" << std::endl;
    debug() << "SysUtilsTest: TempFile(\".foo\") = \"" << ts::TempFile(u".foo") << "\"" << std::endl;

    // Check that the temporary directory exists
    TSUNIT_ASSERT(ts::IsDirectory(ts::TempDirectory()));

    // Check that temporary files are in this directory
    const ts::UString tmpName(ts::TempFile());
    TSUNIT_ASSERT(ts::DirectoryName(tmpName) == ts::TempDirectory());

    // Check that we are allowed to create temporary files.
    TSUNIT_ASSERT(!ts::FileExists(tmpName));
    TSUNIT_ASSERT(_CreateFile(tmpName, 0));
    TSUNIT_ASSERT(ts::FileExists(tmpName));
    TSUNIT_ASSERT(ts::GetFileSize(tmpName) == 0);
    TSUNIT_ASSERT(ts::DeleteFile(tmpName));
    TSUNIT_ASSERT(!ts::FileExists(tmpName));
}

void SysUtilsTest::testFileSize()
{
    const ts::UString tmpName(ts::TempFile());
    TSUNIT_ASSERT(!ts::FileExists(tmpName));

    // Create a file
    TSUNIT_ASSERT(_CreateFile(tmpName, 1234));
    TSUNIT_ASSERT(ts::FileExists(tmpName));
    TSUNIT_ASSERT(ts::GetFileSize(tmpName) == 1234);

    TSUNIT_ASSERT(ts::TruncateFile(tmpName, 567));
    TSUNIT_ASSERT(ts::GetFileSize(tmpName) == 567);

    const ts::UString tmpName2(ts::TempFile());
    TSUNIT_ASSERT(!ts::FileExists(tmpName2));
    TSUNIT_ASSERT(ts::RenameFile(tmpName, tmpName2));
    TSUNIT_ASSERT(ts::FileExists(tmpName2));
    TSUNIT_ASSERT(!ts::FileExists(tmpName));
    TSUNIT_ASSERT(ts::GetFileSize(tmpName2) == 567);

    TSUNIT_ASSERT(ts::DeleteFile(tmpName2));
    TSUNIT_ASSERT(!ts::FileExists(tmpName2));
}

void SysUtilsTest::testFileTime()
{
    const ts::UString tmpName(ts::TempFile());

    const ts::Time before(ts::Time::CurrentUTC());
    TSUNIT_ASSERT(_CreateFile(tmpName, 0));
    const ts::Time after(ts::Time::CurrentUTC());

    // Some systems (Linux) do not store the milliseconds in the file time.
    // So we use "before" without milliseconds.

    // Additionally, it has been noticed that on Linux virtual machines,
    // when the "before" time is exactly a second (ms = 0), then the
    // file time (no ms) is one second less than the "before time".
    // This is a system artefact, not a test failure. As a precaution,
    // if the ms part of "before" is less than 100 ms, we compare with
    // 1 second less. This can be seen using that command:
    //
    // $ while ! (utest -d -t SysUtilsTest::testFileTime 2>&1 | tee /dev/stderr | grep -q 'before:.*000$'); do true; done
    // ...
    // SysUtilsTest: file: /tmp/tstmp-0051DB3AA2B00000.tmp
    // SysUtilsTest:      before:      2020/08/27 09:23:26.000  <== Time::CurrentUTC() has 000 as millisecond
    // SysUtilsTest:      before base: 2020/08/27 09:23:25.000
    // SysUtilsTest:      file UTC:    2020/08/27 09:23:25.000  <== GetFileModificationTimeUTC() is one second less
    // SysUtilsTest:      after:       2020/08/27 09:23:26.000
    // SysUtilsTest:      file local:  2020/08/27 11:23:25.000

    ts::Time::Fields beforeFields(before);
    const ts::MilliSecond adjustment = beforeFields.millisecond < 100 ? ts::MilliSecPerSec : 0;
    beforeFields.millisecond = 0;
    ts::Time beforeBase(beforeFields);
    beforeBase -= adjustment;

    TSUNIT_ASSERT(ts::FileExists(tmpName));
    const ts::Time fileUtc(ts::GetFileModificationTimeUTC(tmpName));
    const ts::Time fileLocal(ts::GetFileModificationTimeLocal(tmpName));

    debug() << "SysUtilsTest: file: " << tmpName << std::endl
            << "SysUtilsTest:      before:      " << before << std::endl
            << "SysUtilsTest:      before base: " << beforeBase << std::endl
            << "SysUtilsTest:      file UTC:    " << fileUtc << std::endl
            << "SysUtilsTest:      after:       " << after << std::endl
            << "SysUtilsTest:      file local:  " << fileLocal << std::endl;

    // Check that file modification occured between before and after.
    // Some systems may not store the milliseconds in the file time.
    // So we use before without milliseconds

    TSUNIT_ASSERT(beforeBase <= fileUtc);
    TSUNIT_ASSERT(fileUtc <= after);
    TSUNIT_ASSERT(fileUtc.UTCToLocal() == fileLocal);
    TSUNIT_ASSERT(fileLocal.localToUTC() == fileUtc);

    TSUNIT_ASSERT(ts::DeleteFile(tmpName));
    TSUNIT_ASSERT(!ts::FileExists(tmpName));
}

void SysUtilsTest::testDirectory()
{
    const ts::UString dirName(ts::TempFile(u""));
    const ts::UString sep(1, ts::PathSeparator);
    const ts::UString fileName(sep + u"foo.bar");

    TSUNIT_ASSERT(!ts::FileExists(dirName));
    TSUNIT_ASSERT(ts::CreateDirectory(dirName));
    TSUNIT_ASSERT(ts::FileExists(dirName));
    TSUNIT_ASSERT(ts::IsDirectory(dirName));

    TSUNIT_ASSERT(_CreateFile(dirName + fileName, 0));
    TSUNIT_ASSERT(ts::FileExists(dirName + fileName));
    TSUNIT_ASSERT(!ts::IsDirectory(dirName + fileName));

    const ts::UString dirName2(ts::TempFile(u""));
    TSUNIT_ASSERT(!ts::FileExists(dirName2));
    TSUNIT_ASSERT(ts::RenameFile(dirName, dirName2));
    TSUNIT_ASSERT(ts::FileExists(dirName2));
    TSUNIT_ASSERT(ts::IsDirectory(dirName2));
    TSUNIT_ASSERT(!ts::FileExists(dirName));
    TSUNIT_ASSERT(!ts::IsDirectory(dirName));
    TSUNIT_ASSERT(ts::FileExists(dirName2 + fileName));
    TSUNIT_ASSERT(!ts::IsDirectory(dirName2 + fileName));

    TSUNIT_ASSERT(ts::DeleteFile(dirName2 + fileName));
    TSUNIT_ASSERT(!ts::FileExists(dirName2 + fileName));
    TSUNIT_ASSERT(ts::IsDirectory(dirName2));

    TSUNIT_ASSERT(ts::DeleteFile(dirName2));
    TSUNIT_ASSERT(!ts::FileExists(dirName2));
    TSUNIT_ASSERT(!ts::IsDirectory(dirName2));
}

void SysUtilsTest::testWildcard()
{
    const ts::UString dirName(ts::TempFile(u""));
    const ts::UString filePrefix(dirName + ts::PathSeparator + u"foo.");
    const size_t count = 10;

    // Create temporary directory
    TSUNIT_ASSERT(ts::CreateDirectory(dirName));
    TSUNIT_ASSERT(ts::IsDirectory(dirName));

    // Create one file with unique pattern
    const ts::UString spuriousFileName(dirName + ts::PathSeparator + u"tagada");
    TSUNIT_ASSERT(_CreateFile(spuriousFileName, 0));
    TSUNIT_ASSERT(ts::FileExists(spuriousFileName));

    // Create many files
    ts::UStringVector fileNames;
    fileNames.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const ts::UString fileName(filePrefix + ts::UString::Format(u"%03d", {i}));
        TSUNIT_ASSERT(_CreateFile(fileName, 0));
        TSUNIT_ASSERT(ts::FileExists(fileName));
        fileNames.push_back(fileName);
    }
    Display(u"created files:", u"file: ", fileNames);

    // Get wildcard
    ts::UStringVector expanded;
    TSUNIT_ASSERT(ts::ExpandWildcard(expanded, filePrefix + u"*"));
    std::sort(expanded.begin(), expanded.end());
    Display(u"expanded wildcard:", u"expanded: ", expanded);
    TSUNIT_ASSERT(expanded == fileNames);

    // Final cleanup
    for (const auto& file : fileNames) {
        TSUNIT_ASSERT(ts::DeleteFile(file));
        TSUNIT_ASSERT(!ts::FileExists(file));
    }
    TSUNIT_ASSERT(ts::DeleteFile(spuriousFileName));
    TSUNIT_ASSERT(!ts::FileExists(spuriousFileName));
    TSUNIT_ASSERT(ts::DeleteFile(dirName));
    TSUNIT_ASSERT(!ts::FileExists(dirName));
}

void SysUtilsTest::testSearchWildcard()
{
#if defined(TS_LINUX)
    ts::UStringList files;
    const bool ok = ts::SearchWildcard(files, u"/sys/devices", u"dvb*.frontend*");
    debug() << "SysUtilsTest::testSearchWildcard: searched dvb*.frontend* in /sys/devices, status = " << ts::UString::TrueFalse(ok) << std::endl;
    for (const auto& it : files) {
        debug() << "    \"" << it << "\"" << std::endl;
    }
#endif
}

void SysUtilsTest::testHomeDirectory()
{
    const ts::UString dir(ts::UserHomeDirectory());
    debug() << "SysUtilsTest: UserHomeDirectory() = \"" << dir << "\"" << std::endl;

    TSUNIT_ASSERT(!dir.empty());
    TSUNIT_ASSERT(ts::FileExists(dir));
    TSUNIT_ASSERT(ts::IsDirectory(dir));
}

void SysUtilsTest::testProcessMetrics()
{
    ts::ProcessMetrics pm1;
    TSUNIT_ASSERT(pm1.cpu_time == -1);
    TSUNIT_ASSERT(pm1.vmem_size == 0);

    ts::GetProcessMetrics(pm1);
    debug() << "ProcessMetricsTest: CPU time (1) = " << pm1.cpu_time << " ms" << std::endl
            << "ProcessMetricsTest: virtual memory (1) = " << pm1.vmem_size << " bytes" << std::endl;

    TSUNIT_ASSERT(pm1.cpu_time >= 0);
    TSUNIT_ASSERT(pm1.vmem_size > 0);

    // Consume some milliseconds of CPU time
    uint64_t counter = 7;
    for (uint64_t i = 0; i < 10000000L; ++i) {
        counter = counter * counter;
    }

    ts::ProcessMetrics pm2;
    ts::GetProcessMetrics(pm2);
    debug() << "ProcessMetricsTest: CPU time (2) = " << pm2.cpu_time << " ms" << std::endl
            << "ProcessMetricsTest: virtual memory (2) = " << pm2.vmem_size << " bytes" << std::endl;

    TSUNIT_ASSERT(pm2.cpu_time >= 0);
    TSUNIT_ASSERT(pm2.cpu_time >= pm1.cpu_time);
    TSUNIT_ASSERT(pm2.vmem_size > 0);
}

void SysUtilsTest::testIsTerminal()
{
#if defined(TS_WINDOWS)
    debug() << "SysUtilsTest::testIsTerminal: stdin  = \"" << ts::WinDeviceName(::GetStdHandle(STD_INPUT_HANDLE)) << "\"" << std::endl
            << "SysUtilsTest::testIsTerminal: stdout = \"" << ts::WinDeviceName(::GetStdHandle(STD_OUTPUT_HANDLE)) << "\"" << std::endl
            << "SysUtilsTest::testIsTerminal: stderr = \"" << ts::WinDeviceName(::GetStdHandle(STD_ERROR_HANDLE)) << "\"" << std::endl;
#endif
    debug() << "SysUtilsTest::testIsTerminal: StdInIsTerminal = " << ts::UString::TrueFalse(ts::StdInIsTerminal())
            << ", StdOutIsTerminal = " << ts::UString::TrueFalse(ts::StdOutIsTerminal())
            << ", StdErrIsTerminal = " << ts::UString::TrueFalse(ts::StdErrIsTerminal())
            << std::endl;
}

void SysUtilsTest::testSysInfo()
{
    debug() << "SysUtilsTest::testSysInfo: " << std::endl
            << "    isLinux = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isLinux()) << std::endl
            << "    isFedora = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isFedora()) << std::endl
            << "    isRedHat = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isRedHat()) << std::endl
            << "    isUbuntu = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isUbuntu()) << std::endl
            << "    isDebian = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isDebian()) << std::endl
            << "    isMacOS = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isMacOS()) << std::endl
            << "    isBSD = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isBSD()) << std::endl
            << "    isFreeBSD = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isFreeBSD()) << std::endl
            << "    isNetBSD = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isNetBSD()) << std::endl
            << "    isOpenBSD = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isOpenBSD()) << std::endl
            << "    isDragonFlyBSD = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isDragonFlyBSD()) << std::endl
            << "    isWindows = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isWindows()) << std::endl
            << "    isIntel32 = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isIntel32()) << std::endl
            << "    isIntel64 = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isIntel64()) << std::endl
            << "    isArm32 = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isArm32()) << std::endl
            << "    isArm64 = " << ts::UString::TrueFalse(ts::SysInfo::Instance()->isArm64()) << std::endl
            << "    systemVersion = \"" << ts::SysInfo::Instance()->systemVersion() << '"' << std::endl
            << "    systemName = \"" << ts::SysInfo::Instance()->systemName() << '"' << std::endl
            << "    hostName = \"" << ts::SysInfo::Instance()->hostName() << '"' << std::endl
            << "    memoryPageSize = " << ts::SysInfo::Instance()->memoryPageSize() << std::endl;

#if defined(TS_WINDOWS)
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isWindows());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isLinux());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isMacOS());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isNetBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isOpenBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isDragonFlyBSD());
#elif defined(TS_LINUX)
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isWindows());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isLinux());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isMacOS());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isFreeBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isNetBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isOpenBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isDragonFlyBSD());
#elif defined(TS_MAC)
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isWindows());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isLinux());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isMacOS());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isFreeBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isNetBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isOpenBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isDragonFlyBSD());
#elif defined(TS_FREEBSD)
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isWindows());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isLinux());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isMacOS());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isBSD());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isFreeBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isNetBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isOpenBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isDragonFlyBSD());
#elif defined(TS_NETBSD)
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isWindows());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isLinux());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isMacOS());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isFreeBSD());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isNetBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isOpenBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isDragonFlyBSD());
#elif defined(TS_OPENBSD)
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isWindows());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isLinux());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isMacOS());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isFreeBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isNetBSD());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isOpenBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isDragonFlyBSD());
#elif defined(TS_DRAGONFLYBSD)
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isWindows());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isLinux());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isMacOS());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isFreeBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isNetBSD());
    TSUNIT_ASSERT(!ts::SysInfo::Instance()->isOpenBSD());
    TSUNIT_ASSERT(ts::SysInfo::Instance()->isDragonFlyBSD());
#endif

    // We can't predict the memory page size, except that it must be a multiple of 256.
    TSUNIT_ASSERT(ts::SysInfo::Instance()->memoryPageSize() > 0);
    TSUNIT_ASSERT(ts::SysInfo::Instance()->memoryPageSize() % 256 == 0);
}

void SysUtilsTest::testSymLinks()
{
    debug() << "SysUtilsTest::testSymLinks: " << std::endl
                 << "    /proc/self -> \"" << ts::ResolveSymbolicLinks(u"/proc/self") << '"' << std::endl;

    // Obviously non existent paths should translate to themselves.
    const ts::UString badName(u"khzkfjhzHJKHK35464.foo.BAD.NOT.THERE");
    TSUNIT_ASSERT(!ts::IsSymbolicLink(badName));
    TSUNIT_EQUAL(badName, ts::ResolveSymbolicLinks(badName));

#if defined(TS_LINUX)
    TSUNIT_ASSERT(ts::IsSymbolicLink(u"/proc/self"));
    TSUNIT_ASSERT(!ts::ResolveSymbolicLinks(u"/proc/self").empty());
    TSUNIT_ASSERT(ts::ResolveSymbolicLinks(u"/proc/self") != u"/proc/self");
#endif
}

void SysUtilsTest::testCurrentWorkingDirectory()
{
    debug() << "SysUtilsTest::testCurrentWorkingDirectory: " << ts::CurrentWorkingDirectory() << std::endl;

    TSUNIT_ASSERT(!ts::CurrentWorkingDirectory().empty());
    TSUNIT_ASSERT(ts::IsDirectory(ts::CurrentWorkingDirectory()));
}


void SysUtilsTest::testIsAbsoluteFilePath()
{
#if defined(TS_WINDOWS)
    TSUNIT_ASSERT(ts::IsAbsoluteFilePath(u"C:\\foo\\bar"));
    TSUNIT_ASSERT(ts::IsAbsoluteFilePath(u"\\\\foo\\bar"));
    TSUNIT_ASSERT(!ts::IsAbsoluteFilePath(u"foo\\bar"));
    TSUNIT_ASSERT(!ts::IsAbsoluteFilePath(u"bar"));
#else
    TSUNIT_ASSERT(ts::IsAbsoluteFilePath(u"/foo/bar"));
    TSUNIT_ASSERT(ts::IsAbsoluteFilePath(u"/"));
    TSUNIT_ASSERT(!ts::IsAbsoluteFilePath(u"foo/bar"));
    TSUNIT_ASSERT(!ts::IsAbsoluteFilePath(u"bar"));
#endif
}


void SysUtilsTest::testAbsoluteFilePath()
{
#if defined(TS_WINDOWS)
    TSUNIT_EQUAL(u"C:\\foo\\bar\\ab\\cd", ts::AbsoluteFilePath(u"ab\\cd", u"C:\\foo\\bar"));
    TSUNIT_EQUAL(u"C:\\ab\\cd", ts::AbsoluteFilePath(u"C:\\ab\\cd", u"C:\\foo\\bar"));
    TSUNIT_EQUAL(u"C:\\foo\\ab\\cd", ts::AbsoluteFilePath(u"..\\ab\\cd", u"C:\\foo\\bar"));
#else
    TSUNIT_EQUAL(u"/foo/bar/ab/cd", ts::AbsoluteFilePath(u"ab/cd", u"/foo/bar"));
    TSUNIT_EQUAL(u"/ab/cd", ts::AbsoluteFilePath(u"/ab/cd", u"/foo/bar"));
    TSUNIT_EQUAL(u"/foo/ab/cd", ts::AbsoluteFilePath(u"../ab/cd", u"/foo/bar"));
#endif
}


void SysUtilsTest::testCleanupFilePath()
{
#if defined(TS_WINDOWS)
    TSUNIT_EQUAL(u"ab\\cd", ts::CleanupFilePath(u"ab\\cd"));
    TSUNIT_EQUAL(u"ab\\cd", ts::CleanupFilePath(u"ab\\\\\\\\cd\\\\"));
    TSUNIT_EQUAL(u"ab\\cd", ts::CleanupFilePath(u"ab\\.\\cd\\."));
    TSUNIT_EQUAL(u"ab\\cd", ts::CleanupFilePath(u"ab\\zer\\..\\cd"));
    TSUNIT_EQUAL(u"cd\\ef", ts::CleanupFilePath(u"ab\\..\\cd\\ef"));
    TSUNIT_EQUAL(u"\\cd\\ef", ts::CleanupFilePath(u"\\..\\cd\\ef"));
#else
    TSUNIT_EQUAL(u"ab/cd", ts::CleanupFilePath(u"ab/cd"));
    TSUNIT_EQUAL(u"ab/cd", ts::CleanupFilePath(u"ab////cd//"));
    TSUNIT_EQUAL(u"ab/cd", ts::CleanupFilePath(u"ab/./cd/."));
    TSUNIT_EQUAL(u"ab/cd", ts::CleanupFilePath(u"ab/zer/../cd"));
    TSUNIT_EQUAL(u"cd/ef", ts::CleanupFilePath(u"ab/../cd/ef"));
    TSUNIT_EQUAL(u"/cd/ef", ts::CleanupFilePath(u"/../cd/ef"));
#endif
}


void SysUtilsTest::testRelativeFilePath()
{
#if defined(TS_WINDOWS)
    TSUNIT_EQUAL(u"ef", ts::RelativeFilePath(u"C:\\ab\\cd\\ef", u"C:\\ab\\cd\\"));
    TSUNIT_EQUAL(u"ef", ts::RelativeFilePath(u"C:\\ab\\cd\\ef", u"C:\\aB\\CD\\"));
    TSUNIT_EQUAL(u"C:\\ab\\cd\\ef", ts::RelativeFilePath(u"C:\\ab\\cd\\ef", u"D:\\ab\\cd\\"));
    TSUNIT_EQUAL(u"cd\\ef", ts::RelativeFilePath(u"C:\\ab\\cd\\ef", u"C:\\AB"));
    TSUNIT_EQUAL(u"..\\ab\\cd\\ef", ts::RelativeFilePath(u"C:\\ab\\cd\\ef", u"C:\\AB", ts::CASE_SENSITIVE));
    TSUNIT_EQUAL(u"../ab/cd/ef", ts::RelativeFilePath(u"C:\\ab\\cd\\ef", u"C:\\AB", ts::CASE_SENSITIVE, true));
#else
    TSUNIT_EQUAL(u"ef", ts::RelativeFilePath(u"/ab/cd/ef", u"/ab/cd/"));
    TSUNIT_EQUAL(u"cd/ef", ts::RelativeFilePath(u"/ab/cd/ef", u"/ab"));
    TSUNIT_EQUAL(u"../../cd/ef", ts::RelativeFilePath(u"/ab/cd/ef", u"/ab/xy/kl/"));
    TSUNIT_EQUAL(u"../ab/cd/ef", ts::RelativeFilePath(u"/ab/cd/ef", u"/xy"));
    TSUNIT_EQUAL(u"ab/cd/ef", ts::RelativeFilePath(u"/ab/cd/ef", u"/"));
#endif
}
