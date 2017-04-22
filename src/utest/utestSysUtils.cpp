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
//
//  CppUnit test suite for tsSysUtils.h
//
//----------------------------------------------------------------------------

#include "tsSysUtils.h"
#include "tsStringUtils.h"
#include "tsFormat.h"
#include "tsTime.h"
#include "tsUID.h"
#include "utestCppUnitTest.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SysUtilsTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testCurrentProcessId();
    void testCurrentExecutableFile();
    void testSleep();
    void testEnvironment();
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
};

CPPUNIT_TEST_SUITE_REGISTRATION(SysUtilsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void SysUtilsTest::setUp()
{
}

// Test suite cleanup method.
void SysUtilsTest::tearDown()
{
}

// Vectors of strings
namespace {
    void Display(const std::string& title, const std::string& prefix, const ts::StringVector& strings)
    {
        utest::Out() << "SysUtilsTest: " << title << std::endl;
        for (ts::StringVector::const_iterator it = strings.begin(); it != strings.end(); ++it) {
            utest::Out() << "SysUtilsTest: " << prefix << "\"" << *it << "\"" << std::endl;
        }
    }
}

// Create a file with the specified size using standard C++ I/O.
// Return true on success, false on error.
namespace {
    bool _CreateFile (const std::string& name, size_t size)
    {
        std::string data (size, '-');
        std::ofstream file (name.c_str(), std::ios::binary);
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

    utest::Out()
        << "SysUtilsTest: sizeof(ts::ProcessId) = " << sizeof(ts::ProcessId) << std::endl
        << "SysUtilsTest: ts::CurrentProcessId() = " << ts::CurrentProcessId() << std::endl;
}

void SysUtilsTest::testCurrentExecutableFile()
{
    // Hard to make automated tests since we do not expect a predictible executable name.

    std::string exe (ts::ExecutableFile());
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
    CPPUNIT_ASSERT(after >= before + 400);

    utest::Out() << "SysUtilsTest: ts::SleepThread(400), measured " << (after - before) << " ms" << std::endl;
}

void SysUtilsTest::testEnvironment()
{
    utest::Out()
        << "SysUtilsTest: EnvironmentExists (\"HOME\") = "
        << ts::EnvironmentExists("HOME") << std::endl
        << "SysUtilsTest: GetEnvironment(\"HOME\") = \""
        << ts::GetEnvironment("HOME", "(default)") << "\"" << std::endl
        << "SysUtilsTest: EnvironmentExists (\"HOMEPATH\") = "
        << ts::EnvironmentExists ("HOMEPATH") << std::endl
        << "SysUtilsTest: GetEnvironment(\"HOMEPATH\") = \""
        << ts::GetEnvironment("HOMEPATH", "(default)") << "\"" << std::endl;

    CPPUNIT_ASSERT(ts::SetEnvironment("UTEST_A", "foo"));
    CPPUNIT_ASSERT(ts::EnvironmentExists ("UTEST_A"));
    CPPUNIT_ASSERT(ts::GetEnvironment("UTEST_A") == "foo");
    CPPUNIT_ASSERT(ts::DeleteEnvironment("UTEST_A"));
    CPPUNIT_ASSERT(!ts::EnvironmentExists("UTEST_A"));
    CPPUNIT_ASSERT(ts::GetEnvironment("UTEST_A") == "");
    CPPUNIT_ASSERT(ts::GetEnvironment("UTEST_A", "bar") == "bar");

    // Very large value
    const std::string large (2000, 'x');
    ts::SetEnvironment("UTEST_A", large);
    CPPUNIT_ASSERT(ts::EnvironmentExists("UTEST_A"));
    CPPUNIT_ASSERT(ts::GetEnvironment("UTEST_A") == large);

    // Overwrite existing value
    ts::SetEnvironment("UTEST_A", "azerty");
    CPPUNIT_ASSERT(ts::EnvironmentExists ("UTEST_A"));
    CPPUNIT_ASSERT(ts::GetEnvironment("UTEST_A") == "azerty");

    // Analyze full environment
    ts::SetEnvironment("UTEST_A", "123456789");
    ts::SetEnvironment("UTEST_B", "abcdefghijklm");
    ts::SetEnvironment("UTEST_C", "nopqrstuvwxyz");

    ts::Environment env;
    ts::GetEnvironment (env);

    for (ts::Environment::const_iterator it = env.begin(); it != env.end(); ++it) {
        utest::Out() << "SysUtilsTest: env: \"" << it->first << "\" = \"" << it->second << "\"" << std::endl;
    }

    CPPUNIT_ASSERT(env["UTEST_A"] == "123456789");
    CPPUNIT_ASSERT(env["UTEST_B"] == "abcdefghijklm");
    CPPUNIT_ASSERT(env["UTEST_C"] == "nopqrstuvwxyz");

    // Search path
    ts::StringVector ref;
    ref.push_back ("azert/aze");
    ref.push_back ("qsdsd f\\qdfqsd f");
    ref.push_back ("fsdvsdf");
    ref.push_back ("qs5veazr5--verv");

    std::string value (ref[0]);
    for (size_t i = 1; i < ref.size(); ++i) {
        value += ts::SearchPathSeparator + ref[i];
    }
    ts::SetEnvironment("UTEST_A", value);

    ts::StringVector path;
    ts::GetEnvironmentPath (path, "UTEST_A");
    CPPUNIT_ASSERT(path == ref);

    // Expand variables in a string
    CPPUNIT_ASSERT(ts::SetEnvironment("UTEST_A", "123456789"));
    CPPUNIT_ASSERT(ts::SetEnvironment("UTEST_B", "abcdefghijklm"));
    CPPUNIT_ASSERT(ts::SetEnvironment("UTEST_C", "nopqrstuvwxyz"));
    ts::DeleteEnvironment("UTEST_D");

    utest::Out()
        << "SysUtilsTest: ExpandEnvironment(\"\\$UTEST_A\") = \""
        << ts::ExpandEnvironment("\\$UTEST_A") << "\"" << std::endl;

    CPPUNIT_ASSERT(ts::ExpandEnvironment("") == "");
    CPPUNIT_ASSERT(ts::ExpandEnvironment("abc") == "abc");
    CPPUNIT_ASSERT(ts::ExpandEnvironment("$UTEST_A") == "123456789");
    CPPUNIT_ASSERT(ts::ExpandEnvironment("${UTEST_A}") == "123456789");
    CPPUNIT_ASSERT(ts::ExpandEnvironment("\\$UTEST_A") == "$UTEST_A");
    CPPUNIT_ASSERT(ts::ExpandEnvironment("abc$UTEST_A") == "abc123456789");
    CPPUNIT_ASSERT(ts::ExpandEnvironment("abc$UTEST_A$UTEST_B$UTEST_D$UTEST_A/qsd") == "abc123456789abcdefghijklm123456789/qsd");
    CPPUNIT_ASSERT(ts::ExpandEnvironment("abc${UTEST_A}a$UTEST_B$UTEST_D$UTEST_A/qsd") == "abc123456789aabcdefghijklm123456789/qsd");
}

void SysUtilsTest::testIgnoreBrokenPipes()
{
    // Ignoring SIGPIPE may break up with some debuggers.
    // When running the unitary tests under a debugger, it may be useful
    // to define the environment variable NO_IGNORE_BROKEN_PIPES to
    // inhibit this test.

    if (ts::EnvironmentExists("NO_IGNORE_BROKEN_PIPES")) {
        utest::Out() << "SysUtilsTest: ignoring test case testIgnoreBrokenPipes" << std::endl;
    }
    else {

        ts::IgnorePipeSignal();

        // The previous line has effects on UNIX systems only.
        // Recreate a "broken pipe" situation on UNIX systems
        // and checks that we don't die.

#if defined(__unix)
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
#if defined(__windows)
        WAIT_TIMEOUT;
#elif defined(__unix)
        ETIMEDOUT;
#else
        0;
#endif

    const std::string codeMessage(ts::ErrorCodeMessage(code));
    const std::string successMessage(ts::ErrorCodeMessage(ts::SYS_SUCCESS));

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
    utest::Out() << "SysUtilsTest: newUid() = 0x" << ts::Format("%016" FMT_INT64 "X", ts::UID::Instance()->newUID()) << std::endl;
    
    CPPUNIT_ASSERT(ts::UID::Instance()->newUID() != ts::UID::Instance()->newUID());
    CPPUNIT_ASSERT(ts::UID::Instance()->newUID() != ts::UID::Instance()->newUID());
    CPPUNIT_ASSERT(ts::UID::Instance()->newUID() != ts::UID::Instance()->newUID());
}

void SysUtilsTest::testVernacularFilePath()
{
#if defined(__windows)
    CPPUNIT_ASSERT(ts::VernacularFilePath ("C:\\alpha/beta\\gamma") == "C:\\alpha\\beta\\gamma");
    CPPUNIT_ASSERT(ts::VernacularFilePath ("/d/alpha/beta/gamma") == "D:\\alpha\\beta\\gamma");
#elif defined(__unix)
    CPPUNIT_ASSERT(ts::VernacularFilePath ("C:\\alpha/beta\\gamma") == "C:/alpha/beta/gamma");
    CPPUNIT_ASSERT(ts::VernacularFilePath ("/alpha-beta/gamma") == "/alpha-beta/gamma");
#endif    
}

void SysUtilsTest::testFilePaths()
{
    const std::string dir(ts::VernacularFilePath("/dir/for/this.test"));
    const std::string sep(1, ts::PathSeparator);
    const std::string dirSep(dir + ts::PathSeparator);

    CPPUNIT_ASSERT(ts::DirectoryName(dirSep + "foo.bar") == dir);
    CPPUNIT_ASSERT(ts::DirectoryName("foo.bar") == ".");
    CPPUNIT_ASSERT(ts::DirectoryName(sep + "foo.bar") == sep);

    CPPUNIT_ASSERT(ts::BaseName(dirSep + "foo.bar") == "foo.bar");
    CPPUNIT_ASSERT(ts::BaseName(dirSep) == "");

    CPPUNIT_ASSERT(ts::PathSuffix(dirSep + "foo.bar") == ".bar");
    CPPUNIT_ASSERT(ts::PathSuffix(dirSep + "foo.") == ".");
    CPPUNIT_ASSERT(ts::PathSuffix(dirSep + "foo") == "");

    CPPUNIT_ASSERT(ts::AddPathSuffix(dirSep + "foo", ".none") == dirSep + "foo.none");
    CPPUNIT_ASSERT(ts::AddPathSuffix(dirSep + "foo.", ".none") == dirSep + "foo.");
    CPPUNIT_ASSERT(ts::AddPathSuffix(dirSep + "foo.bar", ".none") == dirSep + "foo.bar");

    CPPUNIT_ASSERT(ts::PathPrefix(dirSep + "foo.bar") == dirSep + "foo");
    CPPUNIT_ASSERT(ts::PathPrefix(dirSep + "foo.") == dirSep + "foo");
    CPPUNIT_ASSERT(ts::PathPrefix(dirSep + "foo") == dirSep + "foo");
}

void SysUtilsTest::testTempFiles()
{
    utest::Out() << "SysUtilsTest: TempDirectory() = \"" << ts::TempDirectory() << "\"" << std::endl;
    utest::Out() << "SysUtilsTest: TempFile() = \"" << ts::TempFile() << "\"" << std::endl;
    utest::Out() << "SysUtilsTest: TempFile(\".foo\") = \"" << ts::TempFile(".foo") << "\"" << std::endl;

    // Check that the temporary directory exists
    CPPUNIT_ASSERT(ts::IsDirectory (ts::TempDirectory()));

    // Check that temporary files are in this directory
    const std::string tmpName (ts::TempFile());
    CPPUNIT_ASSERT(ts::DirectoryName (tmpName) == ts::TempDirectory());

    // Check that we are allowed to create temporary files.
    CPPUNIT_ASSERT(!ts::FileExists (tmpName));
    CPPUNIT_ASSERT(_CreateFile (tmpName, 0));
    CPPUNIT_ASSERT(ts::FileExists (tmpName));
    CPPUNIT_ASSERT(ts::GetFileSize(tmpName) == 0);
    CPPUNIT_ASSERT(ts::DeleteFile (tmpName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists (tmpName));
}

void SysUtilsTest::testFileSize()
{
    const std::string tmpName (ts::TempFile());
    CPPUNIT_ASSERT(!ts::FileExists (tmpName));

    // Create a file
    CPPUNIT_ASSERT(_CreateFile (tmpName, 1234));
    CPPUNIT_ASSERT(ts::FileExists (tmpName));
    CPPUNIT_ASSERT(ts::GetFileSize(tmpName) == 1234);

    CPPUNIT_ASSERT(ts::TruncateFile (tmpName, 567) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::GetFileSize(tmpName) == 567);

    const std::string tmpName2 (ts::TempFile());
    CPPUNIT_ASSERT(!ts::FileExists (tmpName2));
    CPPUNIT_ASSERT(ts::RenameFile (tmpName, tmpName2) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::FileExists (tmpName2));
    CPPUNIT_ASSERT(!ts::FileExists (tmpName));
    CPPUNIT_ASSERT(ts::GetFileSize(tmpName2) == 567);

    CPPUNIT_ASSERT(ts::DeleteFile (tmpName2) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists (tmpName2));
}

void SysUtilsTest::testFileTime()
{
    const std::string tmpName (ts::TempFile());

    const ts::Time before (ts::Time::CurrentUTC());
    CPPUNIT_ASSERT(_CreateFile (tmpName, 0));
    const ts::Time after (ts::Time::CurrentUTC());

    // Some systems may not store the milliseconds in the file time.
    // So we use "before" without milliseconds.
    ts::Time::Fields beforeFields (before);
    beforeFields.millisecond = 0;
    const ts::Time beforeBase (beforeFields);

    CPPUNIT_ASSERT(ts::FileExists (tmpName));
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

    CPPUNIT_ASSERT(ts::DeleteFile (tmpName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists (tmpName));
}

void SysUtilsTest::testDirectory()
{
    const std::string dirName(ts::TempFile(""));
    const std::string sep(1, ts::PathSeparator);
    const std::string fileName(sep + "foo.bar");

    CPPUNIT_ASSERT(!ts::FileExists (dirName));
    CPPUNIT_ASSERT(ts::CreateDirectory (dirName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::FileExists (dirName));
    CPPUNIT_ASSERT(ts::IsDirectory (dirName));

    CPPUNIT_ASSERT(_CreateFile (dirName + fileName, 0));
    CPPUNIT_ASSERT(ts::FileExists (dirName + fileName));
    CPPUNIT_ASSERT(!ts::IsDirectory (dirName + fileName));

    const std::string dirName2 (ts::TempFile(""));
    CPPUNIT_ASSERT(!ts::FileExists (dirName2));
    CPPUNIT_ASSERT(ts::RenameFile (dirName, dirName2) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::FileExists (dirName2));
    CPPUNIT_ASSERT(ts::IsDirectory (dirName2));
    CPPUNIT_ASSERT(!ts::FileExists (dirName));
    CPPUNIT_ASSERT(!ts::IsDirectory (dirName));
    CPPUNIT_ASSERT(ts::FileExists (dirName2 + fileName));
    CPPUNIT_ASSERT(!ts::IsDirectory (dirName2 + fileName));

    CPPUNIT_ASSERT(ts::DeleteFile (dirName2 + fileName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists (dirName2 + fileName));
    CPPUNIT_ASSERT(ts::IsDirectory (dirName2));

    CPPUNIT_ASSERT(ts::DeleteFile (dirName2) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(!ts::FileExists (dirName2));
    CPPUNIT_ASSERT(!ts::IsDirectory (dirName2));
}

void SysUtilsTest::testWildcard()
{
    const std::string dirName (ts::TempFile(""));
    const std::string filePrefix (dirName + ts::PathSeparator + "foo.");
    const size_t count = 10;

    // Create temporary directory
    CPPUNIT_ASSERT(ts::CreateDirectory (dirName) == ts::SYS_SUCCESS);
    CPPUNIT_ASSERT(ts::IsDirectory (dirName));

    // Create one file with unique pattern
    const std::string spuriousFileName (dirName + ts::PathSeparator + "tagada");
    CPPUNIT_ASSERT(_CreateFile (spuriousFileName, 0));
    CPPUNIT_ASSERT(ts::FileExists (spuriousFileName));

    // Create many files
    ts::StringVector fileNames;
    fileNames.reserve (count);
    for (size_t i = 0; i < count; ++i) {
        const std::string fileName (filePrefix + ts::Format ("%03" FMT_SIZE_T "d", i));
        CPPUNIT_ASSERT(_CreateFile (fileName, 0));
        CPPUNIT_ASSERT(ts::FileExists (fileName));
        fileNames.push_back (fileName);
    }
    Display ("created files:", "file: ", fileNames);

    // Get wildcard
    ts::StringVector expanded;
    CPPUNIT_ASSERT(ts::ExpandWildcard(expanded, filePrefix + "*"));
    std::sort(expanded.begin(), expanded.end());
    Display("expanded wildcard:", "expanded: ", expanded);
    CPPUNIT_ASSERT(expanded == fileNames);

    // Final cleanup
    for (ts::StringVector::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
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
    const std::string dir(ts::UserHomeDirectory());
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
