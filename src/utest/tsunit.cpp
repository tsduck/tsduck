//----------------------------------------------------------------------------
//
// TSUnit - A simple C++ unitary test framework.
// Copyright (c) 2019-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsunit.h"
#include <cstring>
#include <cctype>
#include <codecvt>
#include <locale>

#if (defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)) && !defined(WINDOWS)
    #define WINDOWS 1
#endif

//---------------------------------------------------------------------------------
// Internal utility functions.
//---------------------------------------------------------------------------------

namespace {

    // The prefix string for error messages.
    const char errorPrefix[] = "*** ";

    // An hexadecimal character.
    char toHexa(uint8_t nibble)
    {
        nibble &= 0x0F;
        return nibble < 10 ? char('0' + nibble) : char('A' + nibble - 10);
    }

    // A lowercase version of a string.
    std::string lowerString(const std::string& name)
    {
        std::string res;
        res.reserve(name.size());
        for (auto c : name) {
            res.push_back(char(std::tolower(c)));
        }
        return res;
    }

    // Check if sub is at pos in str, not case-sensitive.
    bool matchSubstring(const std::string& sub, const std::string& str, size_t pos)
    {
        if (sub.empty() || str.empty() || pos + sub.size() > str.size()) {
            return false;
        }
        for (size_t i = 0; i < sub.size(); ++i) {
            if (std::tolower(sub[i]) != std::tolower(str[pos + i])) {
                return false;
            }
        }
        return true;
    }

    // A name without leading and trailing "test", case-insentive.
    std::string trimTest(const std::string& name)
    {
        const std::string test("test");
        std::string res(name);
        while (matchSubstring(test, res, 0)) {
            res.erase(0, test.size());
        }
        while (res.size() >= test.size() && matchSubstring(test, res, res.size() - test.size())) {
            res.erase(res.size() - test.size());
        }
        return res.empty() ? name : res;
    }
}

//---------------------------------------------------------------------------------
// Convert to string.
//---------------------------------------------------------------------------------

std::string tsunit::toString(const Bytes& value)
{
    std::string str;
    str.reserve(3 * value.size());
    for (uint8_t b : value) {
        if (!str.empty()) {
            str += ' ';
        }
        str += toHexa(b >> 4);
        str += toHexa(b & 0x0F);
    }
    return str;
}

//---------------------------------------------------------------------------------
// Explicitly convert UTF-16 to UTF-8
//---------------------------------------------------------------------------------

std::string tsunit::convertFromUTF16(const std::u16string& u16)
{
    #if defined(WINDOWS)
        // Workaround for Visual Studio bug.
        std::wstring wstr(u16.begin(), u16.end());
        #pragma warning(push)
        #pragma warning(disable:4996)
        return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.to_bytes(wstr);
        #pragma warning(pop)
    #else
        // Normal C++ implementation.
        #if defined(__llvm__) || defined(__clang__)
           #pragma clang diagnostic push
           #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        #elif defined(__GNUC__)
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        #endif
        return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(u16);
        #if defined(__llvm__) || defined(__clang__)
            #pragma clang diagnostic pop
        #elif defined(__GNUC__)
            #pragma GCC diagnostic pop
        #endif
    #endif
}

//---------------------------------------------------------------------------------
// Base class for all user tests.
//---------------------------------------------------------------------------------

tsunit::Test::~Test()
{
}

void tsunit::Test::beforeTestSuite()
{
}

void tsunit::Test::afterTestSuite()
{
}

void tsunit::Test::beforeTest()
{
}

void tsunit::Test::afterTest()
{
}

// The null device is initialized by Main when no debug is allowed.
std::ofstream tsunit::Test::_nulDevice;

// Stream where the unitary tests send debug messages.
std::ostream& tsunit::Test::debug()
{
    return _nulDevice.is_open() ? _nulDevice : std::cerr;
}

// This static method checks if debug mode is enabled.
bool tsunit::Test::debugMode()
{
    return !_nulDevice.is_open();
}

//---------------------------------------------------------------------------------
// Generic root class for named objects.
//---------------------------------------------------------------------------------

tsunit::Named::Named(const std::string& name) :
    _name(name)
{
}

tsunit::Named::~Named()
{
}

std::string tsunit::Named::getName() const
{
    return _name;
}

std::string tsunit::Named::getBaseName() const
{
    return trimTest(_name);
}

std::string tsunit::Named::getLowerBaseName() const
{
    return lowerString(trimTest(_name));
}

//---------------------------------------------------------------------------------
// TestSuite base class methods.
//---------------------------------------------------------------------------------

tsunit::TestSuite::TestSuite(const std::string& name, Test* test) :
    Named(name),
    _test(test)
{
}

tsunit::TestSuite::~TestSuite()
{
    // Deallocate all test cases.`
    for (const auto& it : _testmap) {
        if (it.second != nullptr) {
            delete it.second;
        }
    }
    _testmap.clear();

    // Deallocate the user test object.
    if (_test != nullptr) {
        delete _test;
        _test = nullptr;
    }
}

bool tsunit::TestSuite::runBeforeTestSuite()
{
    if (_test != nullptr) {
        try {
            _test->beforeTestSuite();
        }
        catch (const std::exception& e) {
            std::cout << std::endl
                      << errorPrefix << getName() << ", beforeTestSuite, " << e.what() << std::endl
                      << errorPrefix << "Test suite will NOT run" << std::endl;
            return false;
        }
        catch (...) {
            std::cout << std::endl
                      << errorPrefix << getName() << ", beforeTestSuite, unknown exception" << std::endl
                      << errorPrefix << "Test suite will NOT run" << std::endl;
            return false;
        }
    }
    return true;
}

bool tsunit::TestSuite::runAfterTestSuite()
{
    if (_test != nullptr) {
        try {
            _test->afterTestSuite();
        }
        catch (const std::exception& e) {
            std::cout << std::endl << errorPrefix << getName() << ", afterTestSuite, " << e.what() << std::endl;
            return false;
        }
        catch (...) {
            std::cout << std::endl << errorPrefix << getName() << ", afterTestSuite, unknown exception" << std::endl;
            return false;
        }
    }
    return true;
}

bool tsunit::TestSuite::runBeforeTest(const TestCase* test)
{
    if (_test != nullptr) {
        try {
            _test->beforeTest();
        }
        catch (const std::exception& e) {
            std::cout << std::endl
                      << errorPrefix << getName() << "::" << test->getName() << ", beforeTest, " << e.what() << std::endl
                      << errorPrefix << "Test will NOT run" << std::endl;
            return false;
        }
        catch (...) {
            std::cout << std::endl
                      << errorPrefix << getName() << "::" << test->getName() << ", beforeTest, unknown exception" << std::endl
                      << errorPrefix << "Test will NOT run" << std::endl;
            return false;
        }
    }
    return true;
}

bool tsunit::TestSuite::runTest(TestCase* test)
{
    if (test != nullptr) {
        try {
            test->run();
        }
        catch (const std::exception& e) {
            std::cout << std::endl << errorPrefix << getName() << "::" << test->getName() << ", " << e.what() << std::endl;
            return false;
        }
        catch (...) {
            std::cout << std::endl << errorPrefix << getName() << "::" << test->getName() << ", unknown exception" << std::endl;
            return false;
        }
    }
    return true;
}

bool tsunit::TestSuite::runAfterTest(const TestCase* test)
{
    if (_test != nullptr) {
        try {
            _test->afterTest();
        }
        catch (const std::exception& e) {
            std::cout << std::endl << errorPrefix << getName() << "::" << test->getName() << ", afterTest, " << e.what() << std::endl;
            return false;
        }
        catch (...) {
            std::cout << std::endl << errorPrefix << getName() << "::" << test->getName() << ", afterTest, unknown exception" << std::endl;
            return false;
        }
    }
    return true;
}

void tsunit::TestSuite::getAllTestNames(std::list<std::string>& names) const
{
    names.clear();
    for (const auto& it : _testmap) {
        if (it.second != nullptr) {
            names.push_back(it.second->getName());
        }
    }
}

void tsunit::TestSuite::addTestCase(TestCase* test)
{
    // Add or replace a test case.
    if (test != nullptr) {
        TestCase*& t(_testmap[test->getLowerBaseName()]);
        if (t != nullptr) {
            delete t;
        }
        t = test;
    }
}

tsunit::TestCase* tsunit::TestSuite::getTestCase(const std::string& name) const
{
    const auto it = _testmap.find(lowerString(trimTest(name)));
    return it == _testmap.end() ? nullptr : it->second;
}

//---------------------------------------------------------------------------------
// A singleton class containing all tests.
//---------------------------------------------------------------------------------

tsunit::TestRepository* tsunit::TestRepository::_instance = nullptr;

tsunit::TestRepository* tsunit::TestRepository::instance()
{
    // Non thread-safe, pointless in the context of the unitary tests.
    if (_instance == nullptr) {
        _instance = new TestRepository();
        std::atexit(TestRepository::cleanupInstance);
    }
    return _instance;
}

void tsunit::TestRepository::cleanupInstance()
{
    if (_instance != nullptr) {
        delete _instance;
        _instance = nullptr;
    }
}

tsunit::TestRepository::~TestRepository()
{
    // Deallocate all test suites.
    for (const auto& it : _testsuites) {
        if (it.second != nullptr) {
            delete it.second;
        }
    }
    _testsuites.clear();
    _testindex.clear();
}

// Get all test suite names.
void tsunit::TestRepository::getAllTestSuiteNames(std::list<std::string>& names) const
{
    names.clear();
    for (const auto& it : _testsuites) {
        if (it.second != nullptr) {
            names.push_back(it.second->getName());
        }
    }
}

// Get one test suite.
tsunit::TestSuite* tsunit::TestRepository::getTestSuite(const std::string& name) const
{
    const auto it = _testsuites.find(lowerString(trimTest(name)));
    return it == _testsuites.end() ? nullptr : it->second;
}

// An inner class with constructors which register test suites.
tsunit::TestRepository::Register::Register(std::type_index index, const std::string& name, Test* test)
{
    TestRepository* repo = instance();
    TestSuite* suite = new TestSuite(name, test);
    TestSuite*& t(repo->_testsuites[suite->getLowerBaseName()]);
    if (t != nullptr) {
        delete t; // delete previous test suite with that name
    }
    t = suite;
    repo->_testindex[index] = suite;
}

tsunit::TestRepository::Register::Register(std::type_index index, TestCase* test)
{
    TestRepository* repo = instance();
    auto it = repo->_testindex.find(index);
    if (test != nullptr && it != repo->_testindex.end() && it->second != nullptr) {
        it->second->addTestCase(test);
    }
}

//----------------------------------------------------------------------------
// A class running test suites and test cases.
//----------------------------------------------------------------------------

std::string tsunit::TestRunner::_currentTestName;

// Run a test, a test suite or all test suites. Return true when all tests passed.
bool tsunit::TestRunner::run(TestSuite* suite, TestCase* test, bool prepost)
{
    bool ok = true;
    if (suite == nullptr) {
        // Run all test suites.
        Test::debug() << "====== Running all test suites" << std::endl;
        std::list<std::string> names;
        TestRepository* repo = TestRepository::instance();
        repo->getAllTestSuiteNames(names);
        for (const auto& name : names) {
            suite = repo->getTestSuite(name);
            if (suite != nullptr) {
                ok = run(suite) && ok;
            }
        }
    }
    else {
        // Run one test suite.
        if (prepost) {
            ok = suite->runBeforeTestSuite();
        }
        if (ok) {
            if (test == nullptr) {
                // Run all tests in one test suite.
                Test::debug() << "==== Running test suite " << suite->getName() << std::endl;
                std::list<std::string> names;
                suite->getAllTestNames(names);
                for (const auto& it : names) {
                    test = suite->getTestCase(it);
                    if (test != nullptr) {
                        ok = run(suite, test, false) && ok;
                    }
                }
            }
            else {
                // Run one test
                _currentTestName = suite->getName() + "::" + test->getName();
                Test::debug() << "== Running test " << _currentTestName << std::endl;
                // Run pre-test
                ok = suite->runBeforeTest(test);
                // Run test if pre-test succeeded.
                if (ok) {
                    ok = suite->runTest(test);
                    // Run post-test even if test is not OK (must do cleanup if the test ran in any way).
                    ok = suite->runAfterTest(test) && ok;
                }
                _currentTestName.clear();
                // Count passed and failed tests.
                if (ok) {
                    ++_passedCount;
                }
                else {
                    ++_failedCount;
                }
            }
        }
        // Run post-test-suite even if tests are not OK (must do cleanup if the tests ran in any way).
        if (prepost) {
            ok = suite->runAfterTestSuite() && ok;
        }
    }
    return ok;
}

//---------------------------------------------------------------------------------
// The exception which is thrown by assertion failures.
//---------------------------------------------------------------------------------

tsunit::Failure::Failure(const std::string& heading, const std::string& details, const char* sourcefile, int linenumber) :
    _message(heading),
    _sourcefile(sourcefile),
    _linenumber(linenumber)
{
    if (sourcefile != nullptr) {
        if (!_message.empty()) {
            _message += ", ";
        }
        // Locate base name of source file.
        size_t end = _sourcefile.size();
        while (end > 0 && _sourcefile[end-1] != '/' && _sourcefile[end-1] != '\\') {
            end--;
        }
        _sourcefile.erase(0, end);
        _message += _sourcefile;
        _message += ", line ";
        _message += toString(linenumber);
    }
    if (!details.empty()) {
        if (!_message.empty()) {
            _message += '\n';
        }
        _message += details;
    }
}

char const* tsunit::Failure::what() const noexcept
{
    return _message.data();
}

//---------------------------------------------------------------------------------
// Generation of assertion failures.
//---------------------------------------------------------------------------------

std::atomic_size_t tsunit::Assertions::_passedCount(0);
std::atomic_size_t tsunit::Assertions::_failedAssertionsCount(0);
std::atomic_size_t tsunit::Assertions::_failedAssumptionsCount(0);

void tsunit::Assertions::fail(const std::string& message, const char* sourcefile, int linenumber)
{
    ++_failedAssertionsCount;
    throw Failure("test failed", message, sourcefile, linenumber);
}

void tsunit::Assertions::condition(bool cond, const std::string& expression, const char* sourcefile, int linenumber)
{
    if (cond) {
        ++_passedCount;
    }
    else {
        ++_failedAssertionsCount;
        throw Failure("assertion failure", "condition: " + expression, sourcefile, linenumber);
    }
}

void tsunit::Assertions::assumption(bool cond, const std::string& expression, const char* sourcefile, int linenumber)
{
    if (cond) {
        ++_passedCount;
    }
    else {
        ++_failedAssumptionsCount;
        // Same message as an exeption, but do not throw it.
        Failure fail("weak assumption failure", "condition: " + expression, sourcefile, linenumber);
        std::cout << std::endl << errorPrefix << TestRunner::getCurrentTestName() << ", " << fail.what() << std::endl;
    }
}

void tsunit::Assertions::equal(const Bytes& expected, const Bytes& actual, const std::string& estring, const std::string& vstring, const char* sourcefile, int linenumber)
{
    if (expected == actual) {
        ++_passedCount;
    }
    else {
        ++_failedAssertionsCount;
        const std::string details1("expected: " + toString(expected) + " (\"" + estring + "\")");
        const std::string details2("actual:   " + toString(actual) + " (\"" + vstring + "\")");
        throw Failure("incorrect value", details1 + "\n" + details2, sourcefile, linenumber);
    }

}

//---------------------------------------------------------------------------------
// Main constructor from command line
//---------------------------------------------------------------------------------

tsunit::Main::Main(int argc, char* argv[]) :
    _argv0(argc > 0 ? argv[0] : "")
{
    bool ok = true;

    // Decode the command line.
    for (int arg = 1; ok && arg < argc; arg++) {
        const char* opt = argv[arg];
        if (std::strlen(opt) != 2 || opt[0] != '-') {
            ok = false;
        }
        else {
            switch (opt[1]) {
                case 'd':
                    _debug = true;
                    break;
                case 'l':
                    _listMode = true;
                    break;
                case 't':
                    if (++arg >= argc) {
                        ok = false;
                    }
                    else {
                        _testName = argv[arg];
                    }
                    break;
                default:
                    ok = false;
                    break;
            }
        }
    }

    // Error message if incorrect line
    if (!ok) {
        _exitStatus = EXIT_FAILURE;
        std::cerr << _argv0 << ": invalid command" << std::endl
                  << std::endl
                  << "Syntax: " << _argv0 << " [options]" << std::endl
                  << std::endl
                  << "Options:" << std::endl
                  << "  -d : Debug messages are output on standard error." << std::endl
                  << "  -l : List all tests but do not execute them." << std::endl
                  << "  -t name : Run only one test or test suite." << std::endl;
    }
}

//----------------------------------------------------------------------------
// Run the tests
//----------------------------------------------------------------------------

int tsunit::Main::run()
{
    // Filter previous errors
    if (_exitStatus != EXIT_SUCCESS) {
        return _exitStatus;
    }

    // In list mode, only print the list of tests.
    if (_listMode) {
        // Get the list of all test suites.
        std::list<std::string> suiteNames;
        TestRepository* repo = TestRepository::instance();
        repo->getAllTestSuiteNames(suiteNames);
        // Loop on all test suites.
        for (const auto& sname : suiteNames) {
            TestSuite* suite = repo->getTestSuite(sname);
            if (suite != nullptr) {
                // Test suite name alone
                std::cout << suite->getName() << std::endl;
                // Then loop on all individual tests in this test suite.
                std::list<std::string> testNames;
                suite->getAllTestNames(testNames);
                for (const auto& tname : testNames) {
                    std::cout << "    " << suite->getName() << "::" << tname << std::endl;
                }
            }
        }
        return EXIT_SUCCESS;
    }

    // Get optional test suite and test. When nullptr, run them all.
    tsunit::TestSuite* testSuite = nullptr;
    tsunit::TestCase* testCase = nullptr;
    if (!_testName.empty()) {
        TestRepository* repo = TestRepository::instance();
        // First, try to interpret the name as a test suite.
        testSuite = repo->getTestSuite(_testName);
        if (testSuite == nullptr) {
            // Could not find a test suite, try to interpret the name as suite::test.
            const size_t sep = _testName.rfind("::");
            if (sep != std::string::npos) {
                // Test suite name found, try to get it.
                testSuite = repo->getTestSuite(_testName.substr(0, sep));
                if (testSuite != nullptr) {
                    // Test suite found, get the test name.
                    testCase = testSuite->getTestCase(_testName.substr(sep + 2));
                    if (testCase == nullptr) {
                        // Test not found, forget about test suite.
                        testSuite = nullptr;
                    }
                }
            }
        }
        if (testSuite == nullptr) {
            std::cerr << _argv0 << ": unknown test \"" << _testName << "\"" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // In non debug mode, redirect debug messages to nul device
    if (!_debug) {
        // A file name which discards all output
        #if defined(WINDOWS)
            const char nul[] = "NUL:";
        #else
            const char nul[] = "/dev/null";
        #endif
        if (Test::_nulDevice.is_open()) {
            Test::_nulDevice.close();
        }
        Test::_nulDevice.open(nul);
        if (!Test::_nulDevice) {
            std::cerr << _argv0 << ": error opening " << nul << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Run the tests
    bool success = true;
    TestRunner runner;
    success = runner.run(testSuite, testCase);

    // Cleanup resources
    if (Test::_nulDevice.is_open()) {
        Test::_nulDevice.close();
    }

    // Print report.
    if (success && runner.getFailedCount() == 0 && Assertions::getFailedAssertionsCount() == 0) {
        std::cout << std::endl << "OK ";
        if (Assertions::getFailedAssumptionsCount() > 0) {
            std::cout << "with weak failures ";
        }
        std::cout << "(" << runner.getPassedCount() << " tests, " << Assertions::getPassedCount() << " assertions";
        if (Assertions::getFailedAssumptionsCount() > 0) {
            std::cout << ", " << Assertions::getFailedAssumptionsCount() << " weak assumptions failed";
        }
        std::cout << ")" << std::endl << std::endl;
    }
    else {
        std::cout << std::endl
                  << errorPrefix << "FAILURES (" << runner.getFailedCount() << " tests FAILED, " << runner.getPassedCount() << " passed, "
                  << Assertions::getFailedAssertionsCount() << " assertions FAILED, " << Assertions::getPassedCount() << " passed";
        if (Assertions::getFailedAssumptionsCount() > 0) {
            std::cout << ", " << Assertions::getFailedAssumptionsCount() << " weak assumptions failed";
        }
        std::cout << ")" << std::endl << std::endl;
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
