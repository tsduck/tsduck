//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for tsSysUtils.h and tsFileUtils.h
//
//  While system-specific classes move to C++11 and C++17 predefined classes,
//  we also adapt the tests and keep them for a while, to make sure that the
//  predefined classes are effective replacements.
//
//----------------------------------------------------------------------------

#include "tsSysUtils.h"
#include "tsFileUtils.h"
#include "tsEnvironment.h"
#include "tsSysInfo.h"
#include "tsErrCodeReport.h"
#include "tsRegistry.h"
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
    TSUNIT_DECLARE_TEST(StdFileSystem);
    TSUNIT_DECLARE_TEST(CurrentExecutableFile);
    TSUNIT_DECLARE_TEST(Environment);
    TSUNIT_DECLARE_TEST(Registry);
    TSUNIT_DECLARE_TEST(IgnoreBrokenPipes);
    TSUNIT_DECLARE_TEST(ErrorCode);
    TSUNIT_DECLARE_TEST(Uid);
    TSUNIT_DECLARE_TEST(VernacularFilePath);
    TSUNIT_DECLARE_TEST(FilePaths);
    TSUNIT_DECLARE_TEST(TempFiles);
    TSUNIT_DECLARE_TEST(FileTime);
    TSUNIT_DECLARE_TEST(Wildcard);
    TSUNIT_DECLARE_TEST(SearchWildcard);
    TSUNIT_DECLARE_TEST(HomeDirectory);
    TSUNIT_DECLARE_TEST(ProcessCpuTime);
    TSUNIT_DECLARE_TEST(ProcessVirtualSize);
    TSUNIT_DECLARE_TEST(IsTerminal);
    TSUNIT_DECLARE_TEST(SysInfo);
    TSUNIT_DECLARE_TEST(IsAbsoluteFilePath);
    TSUNIT_DECLARE_TEST(AbsoluteFilePath);
    TSUNIT_DECLARE_TEST(CleanupFilePath);
    TSUNIT_DECLARE_TEST(RelativeFilePath);

public:
    virtual void beforeTestSuite() override;
    virtual void afterTestSuite() override;

private:
    cn::milliseconds _precision {};
 };

TSUNIT_REGISTER(SysUtilsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void SysUtilsTest::beforeTestSuite()
{
    _precision = cn::milliseconds(2);
    ts::SetTimersPrecision(_precision);
    debug() << "SysUtilsTest: timer precision = " << ts::UString::Chrono(_precision) << std::endl;
}

// Test suite cleanup method.
void SysUtilsTest::afterTestSuite()
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
    bool _CreateFile(const fs::path& name, size_t size)
    {
        ts::UString data(size, '-');
        std::ofstream file(name, std::ios::binary);
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

// Various tests on std::filesystem and std::filesystem::path.
// We trust the C++ runtime library, this is just a test to understand what it does.
TSUNIT_DEFINE_TEST(StdFileSystem)
{
    bool success = true;

    // Test directory creation.
    const fs::path tmpDirName1(ts::TempFile(u""));
    TSUNIT_ASSERT(!fs::exists(tmpDirName1));
    TSUNIT_ASSERT(fs::create_directories(tmpDirName1));
    TSUNIT_ASSERT(fs::exists(tmpDirName1));
    TSUNIT_ASSERT(fs::is_directory(tmpDirName1));

    // Test file in directory.
    fs::path tmpName1(tmpDirName1);
    tmpName1 /= u"foo.bar";
    TSUNIT_ASSERT(_CreateFile(tmpName1, 0));
    TSUNIT_ASSERT(fs::exists(tmpName1));
    TSUNIT_ASSERT(!fs::is_directory(tmpName1));

    // Test rename directory.
    const fs::path tmpDirName2(ts::TempFile(u""));
    TSUNIT_ASSERT(!fs::exists(tmpDirName2));
    fs::rename(tmpDirName1, tmpDirName2, &ts::ErrCodeReport(success, CERR));
    TSUNIT_ASSERT(success);
    TSUNIT_ASSERT(fs::exists(tmpDirName2));
    TSUNIT_ASSERT(fs::is_directory(tmpDirName2));
    TSUNIT_ASSERT(!fs::exists(tmpDirName1));
    TSUNIT_ASSERT(!fs::is_directory(tmpDirName1));
    tmpName1 = tmpDirName2;
    tmpName1 /= u"foo.bar";
    TSUNIT_ASSERT(fs::exists(tmpName1));
    TSUNIT_ASSERT(!fs::is_directory(tmpName1));

    // Test remove directory and its content.
    TSUNIT_ASSERT(fs::remove(tmpName1, &ts::ErrCodeReport(CERR, u"error deleting", tmpName1)));
    TSUNIT_ASSERT(!fs::exists(tmpName1));
    TSUNIT_ASSERT(fs::is_directory(tmpDirName2));
    TSUNIT_ASSERT(fs::remove(tmpDirName2, &ts::ErrCodeReport(CERR, u"error deleting", tmpDirName2)));
    TSUNIT_ASSERT(!fs::exists(tmpDirName2));
    TSUNIT_ASSERT(!fs::is_directory(tmpDirName2));

    // Test file size.
    const fs::path tmpName2(ts::TempFile());
    TSUNIT_ASSERT(!fs::exists(tmpName2));
    TSUNIT_ASSERT(_CreateFile(tmpName2, 1234));
    TSUNIT_ASSERT(fs::exists(tmpName2));
    TSUNIT_EQUAL(1234, fs::file_size(tmpName2, &ts::ErrCodeReport(CERR)));
    fs::resize_file(tmpName2, 567, &ts::ErrCodeReport(success, CERR));
    TSUNIT_ASSERT(success);
    TSUNIT_EQUAL(567, fs::file_size(tmpName2, &ts::ErrCodeReport(CERR)));

    // Test rename file.
    const fs::path tmpName3(ts::TempFile());
    TSUNIT_ASSERT(!fs::exists(tmpName3));
    fs::rename(tmpName2, tmpName3, &ts::ErrCodeReport(success, CERR));
    TSUNIT_ASSERT(success);
    TSUNIT_ASSERT(fs::exists(tmpName3));
    TSUNIT_ASSERT(!fs::exists(tmpName2));
    TSUNIT_EQUAL(567, fs::file_size(tmpName3, &ts::ErrCodeReport(CERR)));

    // Remove previous temporary file.
    TSUNIT_ASSERT(fs::remove(tmpName3, &ts::ErrCodeReport(CERR, u"error deleting", tmpName2)));
    TSUNIT_ASSERT(!fs::exists(tmpName3));
}

TSUNIT_DEFINE_TEST(CurrentExecutableFile)
{
    // Hard to make automated tests since we do not expect a predictible executable name.

    fs::path exe(ts::ExecutableFile());
    debug() << "SysUtilsTest: ts::ExecutableFile() = \"" << exe.string() << "\"" << std::endl;
    TSUNIT_ASSERT(!exe.empty());
    TSUNIT_ASSERT(fs::exists(exe));
}

TSUNIT_DEFINE_TEST(Environment)
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
        value += ts::SEARCH_PATH_SEPARATOR + ref[i];
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

    TSUNIT_EQUAL(u"a/${UTEST_A}/$UTEST_B/b", ts::ExpandEnvironment(u"a/${UTEST_A}/$UTEST_B/b", ts::ExpandOptions::NONE));
    TSUNIT_EQUAL(u"a/${UTEST_A}/abcdefghijklm/b", ts::ExpandEnvironment(u"a/${UTEST_A}/$UTEST_B/b", ts::ExpandOptions::DOLLAR));
    TSUNIT_EQUAL(u"a/123456789/$UTEST_B/b", ts::ExpandEnvironment(u"a/${UTEST_A}/$UTEST_B/b", ts::ExpandOptions::BRACES));
    TSUNIT_EQUAL(u"a/123456789/abcdefghijklm/b", ts::ExpandEnvironment(u"a/${UTEST_A}/$UTEST_B/b", ts::ExpandOptions::ALL));
}

TSUNIT_DEFINE_TEST(Registry)
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

TSUNIT_DEFINE_TEST(IgnoreBrokenPipes)
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

TSUNIT_DEFINE_TEST(ErrorCode)
{
    // Hard to make automated tests since we do not expect portable strings

    const int code =
#if defined(TS_WINDOWS)
        WAIT_TIMEOUT;
#elif defined(TS_UNIX)
        ETIMEDOUT;
#else
        0;
#endif

    const ts::UString codeMessage(ts::SysErrorCodeMessage(code));
    const ts::UString successMessage(ts::SysErrorCodeMessage(0));

    debug() << "SysUtilsTest: SUCCESS message = \"" << successMessage << "\"" << std::endl
            << "SysUtilsTest: test code = " << code << std::endl
            << "SysUtilsTest: test code message = \"" << codeMessage << "\"" << std::endl;

    TSUNIT_ASSERT(!codeMessage.empty());
    TSUNIT_ASSERT(!successMessage.empty());
}

TSUNIT_DEFINE_TEST(Uid)
{
    debug() << "SysUtilsTest: UID() = 0x" << ts::UString::Hexa(ts::UID()) << std::endl;
    debug() << "SysUtilsTest: UID() = 0x" << ts::UString::Hexa(ts::UID()) << std::endl;
    debug() << "SysUtilsTest: UID() = 0x" << ts::UString::Hexa(ts::UID()) << std::endl;

    TSUNIT_ASSERT(ts::UID() != ts::UID());
    TSUNIT_ASSERT(ts::UID() != ts::UID());
    TSUNIT_ASSERT(ts::UID() != ts::UID());
}

TSUNIT_DEFINE_TEST(VernacularFilePath)
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

TSUNIT_DEFINE_TEST(FilePaths)
{
    const ts::UString dir(ts::VernacularFilePath(u"/dir/for/this.test"));
    const ts::UString sep(1, fs::path::preferred_separator);
    const ts::UString dirSep(dir + fs::path::preferred_separator);

    TSUNIT_ASSERT(ts::DirectoryName(dirSep + u"foo.bar") == dir);
    TSUNIT_ASSERT(ts::DirectoryName(u"foo.bar") == u".");
    TSUNIT_ASSERT(ts::DirectoryName(sep + u"foo.bar") == sep);

    TSUNIT_ASSERT(ts::BaseName(dirSep + u"foo.bar") == u"foo.bar");
    TSUNIT_ASSERT(ts::BaseName(dirSep) == u"");
}

TSUNIT_DEFINE_TEST(TempFiles)
{
    debug() << "SysUtilsTest: TempDirectory() = \"" << fs::temp_directory_path() << "\"" << std::endl;
    debug() << "SysUtilsTest: TempFile() = \"" << ts::TempFile() << "\"" << std::endl;
    debug() << "SysUtilsTest: TempFile(\".foo\") = \"" << ts::TempFile(u".foo") << "\"" << std::endl;

    // Check that the temporary directory exists
    TSUNIT_ASSERT(fs::is_directory(fs::temp_directory_path()));

    // Check that temporary files are in this directory
    const fs::path tmpName(ts::TempFile());
    TSUNIT_EQUAL(fs::canonical(tmpName.parent_path()).string(), fs::canonical(fs::temp_directory_path()).string());

    // Check that we are allowed to create temporary files.
    TSUNIT_ASSERT(!fs::exists(tmpName));
    TSUNIT_ASSERT(_CreateFile(tmpName, 0));
    TSUNIT_ASSERT(fs::exists(tmpName));
    TSUNIT_EQUAL(0, fs::file_size(tmpName, &ts::ErrCodeReport(CERR)));
    TSUNIT_ASSERT(fs::remove(tmpName, &ts::ErrCodeReport(CERR, u"error deleting", tmpName)));
    TSUNIT_ASSERT(!fs::exists(tmpName));
}

TSUNIT_DEFINE_TEST(FileTime)
{
    const fs::path tmpName(ts::TempFile());

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
    const cn::milliseconds adjustment = cn::milliseconds(beforeFields.millisecond < 100 ? 1009 : 0);
    beforeFields.millisecond = 0;
    ts::Time beforeBase(beforeFields);
    beforeBase -= adjustment;

    TSUNIT_ASSERT(fs::exists(tmpName));
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

    TSUNIT_ASSERT(fs::remove(tmpName, &ts::ErrCodeReport(CERR, u"error deleting", tmpName)));
    TSUNIT_ASSERT(!fs::exists(tmpName));
}

TSUNIT_DEFINE_TEST(Wildcard)
{
    const ts::UString dir_name(ts::TempFile(u""));
    const ts::UString file_prefix(dir_name + fs::path::preferred_separator + u"foo.");
    static constexpr size_t count = 10;

    // Create temporary directory
    TSUNIT_ASSERT(fs::create_directory(dir_name));
    TSUNIT_ASSERT(fs::is_directory(dir_name));

    // Create one file with unique pattern
    const ts::UString spurious_file_name(dir_name + fs::path::preferred_separator + u"tagada");
    TSUNIT_ASSERT(_CreateFile(spurious_file_name, 0));
    TSUNIT_ASSERT(fs::exists(spurious_file_name));

    // Create many files
    ts::UStringVector file_names;
    file_names.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const ts::UString file_name(file_prefix + ts::UString::Format(u"%03d", i));
        TSUNIT_ASSERT(_CreateFile(file_name, 0));
        TSUNIT_ASSERT(fs::exists(file_name));
        file_names.push_back(file_name);
    }
    Display(u"created files:", u"file: ", file_names);

    // Get wildcard
    ts::UStringVector expanded;
    TSUNIT_ASSERT(ts::ExpandWildcard(expanded, file_prefix + u"*"));
    std::sort(expanded.begin(), expanded.end());
    Display(u"expanded wildcard:", u"expanded: ", expanded);
    TSUNIT_ASSERT(expanded == file_names);

#if defined(TS_WINDOWS)
    // On Windows, make sure it works with '/' instead of '\' (fs::path::preferred_separator)
    ts::UStringVector file_names_2;
    file_names_2.reserve(file_names.size());
    for (const auto& fn : file_names) {
        file_names_2.push_back(fn.toSubstituted(fs::path::preferred_separator, u'/'));
    }
    TSUNIT_ASSERT(ts::ExpandWildcard(expanded, file_prefix.toSubstituted(fs::path::preferred_separator, u'/') + u"*"));
    std::sort(expanded.begin(), expanded.end());
    Display(u"expanded wildcard 2:", u"expanded: ", expanded);
    TSUNIT_ASSERT(expanded == file_names_2);
#endif

    // Final cleanup
    for (const auto& file : file_names) {
        TSUNIT_ASSERT(fs::remove(file, &ts::ErrCodeReport(CERR, u"error deleting", file)));
        TSUNIT_ASSERT(!fs::exists(file));
    }
    TSUNIT_ASSERT(fs::remove(spurious_file_name, &ts::ErrCodeReport(CERR, u"error deleting", spurious_file_name)));
    TSUNIT_ASSERT(!fs::exists(spurious_file_name));
    TSUNIT_ASSERT(fs::remove(dir_name, &ts::ErrCodeReport(CERR, u"error deleting", dir_name)));
    TSUNIT_ASSERT(!fs::exists(dir_name));
}

TSUNIT_DEFINE_TEST(SearchWildcard)
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

TSUNIT_DEFINE_TEST(HomeDirectory)
{
    const ts::UString dir(ts::UserHomeDirectory());
    debug() << "SysUtilsTest: UserHomeDirectory() = \"" << dir << "\"" << std::endl;

    TSUNIT_ASSERT(!dir.empty());
    TSUNIT_ASSERT(fs::exists(dir));
    TSUNIT_ASSERT(fs::is_directory(dir));
}

TSUNIT_DEFINE_TEST(ProcessCpuTime)
{
    const cn::milliseconds t1 = ts::GetProcessCpuTime();
    debug() << "SysUtilsTest: CPU time (1) = " << ts::UString::Chrono(t1) << std::endl;
    TSUNIT_ASSERT(t1.count() >= 0);

    // Consume some milliseconds of CPU time
    uint64_t counter = 7;
    for (uint64_t i = 0; i < 10000000L; ++i) {
        counter = counter * counter;
    }

    const cn::milliseconds t2 = ts::GetProcessCpuTime();
    debug() << "SysUtilsTest: CPU time (2) = " << ts::UString::Chrono(t2) << " ms" << std::endl;
    TSUNIT_ASSERT(t2.count() >= 0);
    TSUNIT_ASSERT(t2 >= t1);
}

TSUNIT_DEFINE_TEST(ProcessVirtualSize)
{
    const size_t m1 = ts::GetProcessVirtualSize();
    debug() << "SysUtilsTest: virtual memory (1) = " << m1 << " bytes" << std::endl;
    TSUNIT_ASSERT(m1 > 0);

    // Consume (maybe) some new memory.
    void* mem = ::malloc(5000000);
    const size_t m2 = ts::GetProcessVirtualSize();
    ::free(mem);

    debug() << "SysUtilsTest: virtual memory (2) = " << m2 << " bytes" << std::endl;
    TSUNIT_ASSERT(m2 > 0);
}

TSUNIT_DEFINE_TEST(IsTerminal)
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

TSUNIT_DEFINE_TEST(SysInfo)
{
    debug() << "SysUtilsTest::testSysInfo: " << std::endl
            << "    arch() = " << int(ts::SysInfo::Instance().arch()) << std::endl
            << "    os() = " << int(ts::SysInfo::Instance().os()) << std::endl
            << "    osFlavor() = " << int(ts::SysInfo::Instance().osFlavor()) << std::endl
            << "    systemVersion = \"" << ts::SysInfo::Instance().systemVersion() << '"' << std::endl
            << "    systemMajorVersion = " << ts::SysInfo::Instance().systemMajorVersion() << std::endl
            << "    systemName = \"" << ts::SysInfo::Instance().systemName() << '"' << std::endl
            << "    hostName = \"" << ts::SysInfo::Instance().hostName() << '"' << std::endl
            << "    memoryPageSize = " << ts::SysInfo::Instance().memoryPageSize() << std::endl;

#if defined(TS_WINDOWS)
    TSUNIT_EQUAL(ts::SysInfo::WINDOWS, ts::SysInfo::Instance().os());
    TSUNIT_EQUAL(ts::SysInfo::NONE, ts::SysInfo::Instance().osFlavor());
#elif defined(TS_LINUX)
    TSUNIT_EQUAL(ts::SysInfo::LINUX, ts::SysInfo::Instance().os());
#elif defined(TS_MAC)
    TSUNIT_EQUAL(ts::SysInfo::MACOS, ts::SysInfo::Instance().os());
    TSUNIT_EQUAL(ts::SysInfo::NONE, ts::SysInfo::Instance().osFlavor());
#elif defined(TS_FREEBSD)
    TSUNIT_EQUAL(ts::SysInfo::BSD, ts::SysInfo::Instance().os());
    TSUNIT_EQUAL(ts::SysInfo::FREEBSD, ts::SysInfo::Instance().osFlavor());
#elif defined(TS_NETBSD)
    TSUNIT_EQUAL(ts::SysInfo::BSD, ts::SysInfo::Instance().os());
    TSUNIT_EQUAL(ts::SysInfo::NETBSD, ts::SysInfo::Instance().osFlavor());
#elif defined(TS_OPENBSD)
    TSUNIT_EQUAL(ts::SysInfo::BSD, ts::SysInfo::Instance().os());
    TSUNIT_EQUAL(ts::SysInfo::OPENBSD, ts::SysInfo::Instance().osFlavor());
#elif defined(TS_DRAGONFLYBSD)
    TSUNIT_EQUAL(ts::SysInfo::BSD, ts::SysInfo::Instance().os());
    TSUNIT_EQUAL(ts::SysInfo::DFLYBSD, ts::SysInfo::Instance().osFlavor());
#endif

#if defined(TS_I386)
    // TS_I386 means 32-bit compiled code.
    // Win32 code can run on Win32 or Win64 systems.
    TSUNIT_ASSERT(ts::SysInfo::Instance().arch() == ts::SysInfo::INTEL32 || ts::SysInfo::Instance().arch() == ts::SysInfo::INTEL64);
#elif defined(TS_X86_64)
    TSUNIT_EQUAL(ts::SysInfo::INTEL64, ts::SysInfo::Instance().arch());
#elif defined(TS_ARM32)
    TSUNIT_EQUAL(ts::SysInfo::ARM32, ts::SysInfo::Instance().arch());
#elif defined(TS_ARM64)
    TSUNIT_EQUAL(ts::SysInfo::ARM64, ts::SysInfo::Instance().arch());
#elif defined(TS_POWERPC)
    TSUNIT_EQUAL(ts::SysInfo::PPC32, ts::SysInfo::Instance().arch());
#elif defined(TS_POWERPC64)
    TSUNIT_EQUAL(ts::SysInfo::PPC64, ts::SysInfo::Instance().arch());
#elif defined(TS_MIPS)
    TSUNIT_EQUAL(ts::SysInfo::MIPS32, ts::SysInfo::Instance().arch());
#elif defined(TS_MIPS64)
    TSUNIT_EQUAL(ts::SysInfo::MIPS64, ts::SysInfo::Instance().arch());
#elif defined(TS_RISCV64)
    TSUNIT_EQUAL(ts::SysInfo::RISCV64, ts::SysInfo::Instance().arch());
#elif defined(TS_S390X)
    TSUNIT_EQUAL(ts::SysInfo::S390X, ts::SysInfo::Instance().arch());
#endif

    // We can't predict the memory page size, except that it must be a multiple of 256.
    TSUNIT_ASSERT(ts::SysInfo::Instance().memoryPageSize() > 0);
    TSUNIT_ASSERT(ts::SysInfo::Instance().memoryPageSize() % 256 == 0);
}

TSUNIT_DEFINE_TEST(IsAbsoluteFilePath)
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

TSUNIT_DEFINE_TEST(AbsoluteFilePath)
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

TSUNIT_DEFINE_TEST(CleanupFilePath)
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

TSUNIT_DEFINE_TEST(RelativeFilePath)
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
