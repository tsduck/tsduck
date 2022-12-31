//----------------------------------------------------------------------------
//
// TSUnit - A simple C++ unitary test framework.
// Copyright (c) 2019-2023, Thierry Lelegard
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
//! @file
//! TSUnit interface (a simple C++ unitary test framework).
//!
//! TSUnit is compatible with a subset of CppUnit. TSUnit was originally
//! developed to implement the unitary tests for TSDuck. Initially, TSDuck
//! used CppUnit but there were issues on availability of binary libraries
//! for CppUnit on various platforms, as well as compatibility issues.
//!
//----------------------------------------------------------------------------

#pragma once

#if defined(_MSC_VER)
// How to pass the system header files with /Wall
#pragma warning(disable:4100)  // unreferenced formal parameter
#pragma warning(disable:4251)  // 'classname' : class 'std::vector<_Ty>' needs to have dll-interface to be used by clients of class 'classname'
#pragma warning(disable:4275)  // non dll-interface class 'std::_Container_base_aux' used as base for dll-interface class 'std::_Container_base_aux_alloc_real<_Alloc>'
#pragma warning(disable:4355)  // 'this' : used in base member initializer list
#pragma warning(disable:4365)  // conversion from 'type1' to 'type2', signed/unsigned mismatch
#pragma warning(disable:4371)  // layout of class may have changed from a previous version of the compiler due to better packing of member 'xxxx'
#pragma warning(disable:4514)  // unreferenced inline function has been removed
#pragma warning(disable:4571)  // catch (...) semantics changed since Visual C++ 7.1; structured exceptions(SEH) are no longer caught
#pragma warning(disable:4619)  // disablement of warning that doesn't exist
#pragma warning(disable:4625)  // copy constructor was implicitly defined as deleted
#pragma warning(disable:4626)  // assignment operator was implicitly defined as deleted
#pragma warning(disable:4710)  // 'xxx' : function not inlined
#pragma warning(disable:4711)  // function 'xxx' selected for automatic inline expansion
#pragma warning(disable:4738)  // storing 32-bit float result in memory, possible loss of performance
#pragma warning(disable:4774)  // format string expected in argument N is not a string literal
#pragma warning(disable:4820)  // 'n' bytes padding added after data member 'nnnnn'
#pragma warning(disable:5026)  // move constructor was implicitly defined as deleted
#pragma warning(disable:5027)  // move assignment operator was implicitly defined as deleted
#pragma warning(disable:5031)  // bug in winioctl.h : #pragma warning(pop) : likely mismatch, popping warning state pushed in different file
#pragma warning(disable:5032)  // bug in winioctl.h : detected #pragma warning(push) with no corresponding #pragma warning(pop)
#pragma warning(disable:5039)  // pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
#pragma warning(disable:5045)  // Compiler will insert Spectre mitigation for memory load if / Qspectre switch specified
#pragma warning(disable:5262)  // implicit fall - through occurs here; are you missing a break statement ? Use [[fallthrough]] when a break statement is intentionally omitted between cases
#endif

#if defined(__llvm__)
#pragma clang diagnostic ignored "-Wc++98-compat"           // Need C++11, don't care about C++98
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"  // Idem
#pragma clang diagnostic ignored "-Wglobal-constructors"    // We use many global objects for test registration
#pragma clang diagnostic ignored "-Wexit-time-destructors"  // Idem
#pragma clang diagnostic ignored "-Wpadded"                 // Allow padding between class fields
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command" // Should work but fails with clang 10.0.0 on Linux
#endif

#include <limits>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <exception>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <atomic>

//!
//! Unitary tests namespace.
//!
namespace tsunit {
    //!
    //! Base class for all user tests.
    //!
    class Test
    {
    public:
        //!
        //! Virtual destructor.
        //!
        virtual ~Test();
        //!
        //! Invoked once before each individual test case.
        //!
        virtual void beforeTest();
        //!
        //! Invoked once after each individual test case.
        //!
        virtual void afterTest();
        //!
        //! This static method returns a reference to an output stream
        //! which can be used by unitary tests to log debug messages.
        //!
        //! A unitary test typically does not display anything. It simply
        //! performs assertions. A complete set of unitary test suites
        //! reports successes or failures using TSUnit.
        //!
        //! However, there are cases where the unitary test may want to
        //! issue trace, log or debug messages. Such messages should be
        //! sent to this output stream.
        //!
        //! By default, these messages are discarded. However, when the
        //! option -d (debug) is specified on the command line of the
        //! unitary test driver, these messages are reported on the
        //! standard error stream.
        //!
        //! @return A reference to an output stream used to report debug messages.
        //!
        static std::ostream& debug();
        //!
        //! This static method checks if debug mode is enabled.
        //! @return True if debug mode is enabled.
        //!
        static bool debugMode();

    private:
        friend class Main;
        static std::ofstream _nulDevice;
    };

    //!
    //! This class drives all unitary tests in a project.
    //!
    //! There must be one instance in the main program of the unitary
    //! test driver of the project.
    //!
    //! The layout of the unitary test driver main program is as simple as:
    //! @code
    //! #include "tsunit.h"
    //! int main(int argc, char* argv[])
    //! {
    //!     tsunit::Main ctx(argc, argv);
    //!     return ctx.run();
    //! }
    //! @endcode
    //!
    //! The command line arguments @c argc and @c argv are analyzed to setup
    //! the unitary tests. The accepted command line arguments are:
    //!
    //! @li -d : Debug messages from the unitary tests are output on standard error.
    //! @li -l : List all tests but do not execute them.
    //! @li -t name : Run only one test or test suite (use -l for test list).
    //!
    class Main
    {
    public:
        //!
        //! Constructor from command line arguments.
        //! The command line arguments are analyzed and the object is setup accordingly.
        //! @param [in] argc Number of arguments from command line.
        //! @param [in] argv Arguments from command line.
        //!
        Main(int argc, char* argv[]);
        //!
        //! Run the unitary tests.
        //! @return EXIT_SUCCESS if all tests passed, EXIT_FAILURE otherwise.
        //! Thus, the result can be used as exit status in the unitary test driver.
        //!
        int run();

    private:
        std::string _argv0;       // program name
        std::string _testName;    // name of test to run
        bool        _listMode;    // list tests, do not execute
        bool        _debug;       // enable debug messages
        int         _exitStatus;  // EXIT_SUCCESS or EXIT_FAILURE

        // Inaccessible operations.
        Main() = delete;
        Main(Main&&) = delete;
        Main(const Main&) = delete;
        Main& operator=(Main&&) = delete;
        Main& operator=(const Main&) = delete;
    };
}

//! @cond nodoxygen
// A macro to generate a unique name from a prefix and the source line number of the macro call.
#define TSUNIT_NAME1_(prefix,num) prefix##num
#define TSUNIT_NAME2_(prefix,num) TSUNIT_NAME1_(prefix,num)
#define TSUNIT_NAME(prefix) TSUNIT_NAME2_(prefix,__LINE__)
//! @endcond

//!
//! Start the description of a test suite inside a test class.
//! @hideinitializer
//! @param classname Simple class name. The macro shall be invoked inside
//! the class declaration. See the sample code below.
//!
//! @code
//! class ExcTest: public std::exception { ... };
//!
//! namespace foo
//! {
//!     class Bar: public tsunit::Test
//!     {
//!     public:
//!         void test1();
//!         void test2();
//!         void test3();
//!
//!         TSUNIT_TEST_BEGIN(Bar);
//!         TSUNIT_TEST(test1);
//!         TSUNIT_TEST(test2);
//!         TSUNIT_TEST_EXCEPTION(test3, ExcTest);
//!         TSUNIT_TEST_END();
//!     };
//! }
//!
//! TSUNIT_REGISTER(foo::Bar);
//! @endcode
//!
#define TSUNIT_TEST_BEGIN(classname)                                  \
    public:                                                           \
        static tsunit::TestSuite* testSuite()                         \
        {                                                             \
            typedef classname _TestClass;                             \
            _TestClass* instance = new _TestClass;                    \
            tsunit::TestSuite* suite = new tsunit::TestSuite(#classname, instance)

//!
//! Add a test method to the test suite inside a test class.
//! @hideinitializer
//! @param method Simple name of a test method. Must be a <code>void (*)()</code> method.
//! @see TSUNIT_TEST_BEGIN
//!
#define TSUNIT_TEST(method)  \
            suite->addTestCase(new tsunit::TestCaseWrapper<_TestClass>(#method, &_TestClass::method, instance))

//!
//! Add a test method which should raise an exception to the test suite inside a test class.
//! @hideinitializer
//! @param method Simple name of a test method. Must be a <code>void (*)()</code> method.
//! @param exceptclass Name
//! @see TSUNIT_TEST_BEGIN
//!
#define TSUNIT_TEST_EXCEPTION(method, exceptclass)  \
            suite->addTestCase(new tsunit::TestExceptionWrapper<_TestClass, exceptclass>(#method, #exceptclass, &_TestClass::method, instance, __FILE__, __LINE__))

//!
//! End of the test suite inside a test class.
//! @hideinitializer
//! @see TSUNIT_TEST_BEGIN
//!
#define TSUNIT_TEST_END()   \
            return suite;   \
        }                   \
    private:                \
        typedef int TSUNIT_NAME(unused)

//!
//! Register a test class as a test suite.
//! @hideinitializer
//! @param classname Fully qualified class name. The macro shall be invoked outside the class.
//! @see TSUNIT_TEST_BEGIN
//!
#define TSUNIT_REGISTER(classname) \
    static const tsunit::TestRepository::Register TSUNIT_NAME(_Registrar)(classname::testSuite())

//!
//! Report a test case as failed.
//! @hideinitializer
//! @param message A message to display to explain the failure.
//!
#define TSUNIT_FAIL(message) (tsunit::Assertions::fail((message), __FILE__, __LINE__))

//!
//! Assert a condition, mark the test as failed when false.
//! @hideinitializer
//! @param cond A condition to assert.
//!
#define TSUNIT_ASSERT(cond) (tsunit::Assertions::condition((cond), #cond, __FILE__, __LINE__))

//!
//! Assume a condition, report failure but do not abort the test and do not mark as failed when false.
//! This is a replacement for TSUNIT_ASSERT when the condition cannot always be enforced
//! but of timing issues for instances.
//! @hideinitializer
//! @param cond A condition to assert.
//!
#define TSUNIT_ASSUME(cond) (tsunit::Assertions::assumption((cond), #cond, __FILE__, __LINE__))

//!
//! Assert that an expression has some expected value, mark the test as failed when different.
//! @hideinitializer
//! @param expected The expected value.
//! @param actual The actual value.
//!
#define TSUNIT_EQUAL(expected,actual) (tsunit::Assertions::equal((expected), (actual), #expected, #actual, __FILE__, __LINE__))

//---------------------------------------------------------------------------------
// Implementation classes, only used by public macros, not documented.
//---------------------------------------------------------------------------------

//! @cond nodoxygen
namespace tsunit {

    // Generic vector of bytes.
    typedef std::vector<uint8_t> Bytes;

    // Generic root class for named objects.
    class Named
    {
    public:
        Named(const std::string& name = std::string());
        virtual ~Named();
        std::string getName() const;           // original name
        std::string getBaseName() const;       // without leading and trailing "test", case-insentive
        std::string getLowerBaseName() const;  // lowercase version of base name
    private:
        std::string _name;
    };

    // Definition of a test case (one method in a user test class).
    class TestCase : public Named
    {
    public:
        TestCase(const std::string& name);
        virtual ~TestCase() override;
        virtual void run() = 0;
    private:
        TestCase() = delete;
        TestCase(TestCase&&) = delete;
        TestCase(const TestCase&) = delete;
        TestCase& operator=(TestCase&&) = delete;
        TestCase& operator=(const TestCase&) = delete;
    };

    // A template subclass of TestCase for a specific user test class.
    template<class TEST, typename std::enable_if<std::is_base_of<Test, TEST>::value>::type* = nullptr>
    class TestCaseWrapper: public TestCase
    {
    public:
        typedef void (TEST::*TestMethod)();
        TestCaseWrapper(const std::string& testname, TestMethod method, TEST* test);
        virtual void run() override;
    private:
        TestMethod _method;
        TEST* _test;
        TestCaseWrapper() = delete;
        TestCaseWrapper(TestCaseWrapper&&) = delete;
        TestCaseWrapper(const TestCaseWrapper&) = delete;
        TestCaseWrapper& operator=(TestCaseWrapper&&) = delete;
        TestCaseWrapper& operator=(const TestCaseWrapper&) = delete;
    };

    // A template subclass of TestCase for a specific user test class which raise an exception.
    template<class TEST, class EXCEP,
             typename std::enable_if<std::is_base_of<Test, TEST>::value>::type* = nullptr,
             typename std::enable_if<std::is_base_of<std::exception, EXCEP>::value>::type* = nullptr>
    class TestExceptionWrapper: public TestCase
    {
    public:
        typedef void (TEST::*TestMethod)();
        TestExceptionWrapper(const std::string& testname, const std::string& excepname, TestMethod method, TEST* test, const char* sourcefile, int linenumber);
        virtual void run() override;
    private:
        TestMethod _method;
        std::string _excepname;
        std::string _sourcefile;
        int _linenumber;
        TEST* _test;
        TestExceptionWrapper() = delete;
        TestExceptionWrapper(TestExceptionWrapper&&) = delete;
        TestExceptionWrapper(const TestExceptionWrapper&) = delete;
        TestExceptionWrapper& operator=(TestExceptionWrapper&&) = delete;
        TestExceptionWrapper& operator=(const TestExceptionWrapper&) = delete;
    };

    // Definition of a test suite (all test methods in a user test class).
    // The Test object and all test cases are owned by the instance of TestSuite;
    // they are deallocated in the destructor.
    class TestSuite : public Named
    {
    public:
        TestSuite(const std::string& name, Test* test);
        virtual ~TestSuite() override;
        void addTestCase(TestCase* test);
        TestCase* getTestCase(const std::string& name) const;
        void getAllTestNames(std::list<std::string>&) const;
        void runBeforeTest();
        void runAfterTest();
    private:
        Test* _test;
        std::map<std::string, TestCase*> _testmap;
        TestSuite() = delete;
        TestSuite(TestSuite&&) = delete;
        TestSuite(const TestSuite&) = delete;
        TestSuite& operator=(TestSuite&&) = delete;
        TestSuite& operator=(const TestSuite&) = delete;
    };

    // A singleton class containing all tests.
    class TestRepository
    {
    public:
        ~TestRepository();
        void addTestSuite(TestSuite* test);
        TestSuite* getTestSuite(const std::string& name) const;
        void getAllTestSuiteNames(std::list<std::string>&) const;
        static TestRepository* instance(); // singleton instance.
        class Register // an inner class with constructors which register test suites.
        {
        public:
            Register(TestSuite* test);
        };
    private:
        std::map<std::string,TestSuite*> _testsuites;
        static TestRepository* _instance;
        static void cleanupInstance();
        TestRepository();
        TestRepository(TestRepository&&) = delete;
        TestRepository(const TestRepository&) = delete;
        TestRepository& operator=(TestRepository&&) = delete;
        TestRepository& operator=(const TestRepository&) = delete;
    };

    // A class running test suites and test cases.
    class TestRunner
    {
    public:
        TestRunner();
        bool run(TestSuite* suite = nullptr, TestCase* test = nullptr);
        size_t getPassedCount() const { return _passedCount; }
        size_t getFailedCount() const { return _failedCount; }
        static std::string getCurrentTestName() { return _currentTestName; }
    private:
        static std::string _currentTestName;
        size_t _passedCount;
        size_t _failedCount;
        TestRunner(TestRunner&&) = delete;
        TestRunner(const TestRunner&) = delete;
        TestRunner& operator=(TestRunner&&) = delete;
        TestRunner& operator=(const TestRunner&) = delete;
    };

    // The exception which is thrown by assertion failures.
    class Failure: public std::exception
    {
    public:
        Failure(const std::string& heading, const std::string& details, const char* sourcefile, int linenumber);
        std::string getSourceFile() const { return _sourcefile; }
        int getLineNumber() const { return _linenumber; }
        virtual char const* what() const noexcept override;
    private:
        std::string _message;
        std::string _sourcefile;
        int _linenumber;
        Failure() = delete;
    };

    // A type trait to determine if a type is either an integer or an enum.
    template<typename T>
    struct is_intenum {
        static constexpr bool value = std::is_integral<T>::value || std::is_enum<T>::value;
    };

    // A generalization of std::underlying_type which works on all types, not only enums.
    template<bool ISENUM, typename T>
    struct underlying_type_impl {
        typedef T type;
    };

    template<typename T>
    struct underlying_type_impl<true,T> {
        typedef typename std::underlying_type<T>::type type;
    };

    template<typename T>
    struct underlying_type {
        typedef typename underlying_type_impl<std::is_enum<T>::value, T>::type type;
    };

    // Converts an integer or enum into a string.
    template<typename T>
    std::string toStringImpl(T value, const char* format);

    template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    std::string toString(T value) { return toStringImpl(static_cast<double>(value), static_cast<double>(std::fabs(value)) > 0.00001 && static_cast<double>(std::fabs(value)) < 10000.0 ? "%lf" :  "%le"); }

    template<typename T, typename std::enable_if<!std::is_floating_point<T>::value && std::is_signed<typename underlying_type<T>::type>::value>::type* = nullptr>
    std::string toString(T value) { return toStringImpl(static_cast<long long>(value), "%lld"); }

    template<typename T, typename std::enable_if<std::is_unsigned<typename underlying_type<T>::type>::value>::type* = nullptr>
    std::string toString(T value) { return toStringImpl(static_cast<unsigned long long>(value), "%llu"); }

    template<typename T>
    std::string toString(const T* value) { return toStringImpl<size_t>(reinterpret_cast<size_t>(value), "0x%zX"); }

    std::string toString(const Bytes& value);

    // Explicitly convert UTF-16 to UTF-8
    std::string convertFromUTF16(const std::u16string& u16);

    // Convert any string type to UTF-8.
    // Only provide two specializations: for std::string and std::u16string.
    template<typename CHAR> std::string toUTF8(const std::basic_string<CHAR>& s);
    template<> inline std::string toUTF8<char>(const std::string& s) { return s; }
    template<> inline std::string toUTF8<char16_t>(const std::u16string& s) { return convertFromUTF16(s); }

    // A pseudo-class (only static methods and fields) to encapsulate all assertions.
    class Assertions
    {
    private:
        static std::atomic_size_t _passedCount;
        static std::atomic_size_t _failedAssertionsCount;
        static std::atomic_size_t _failedAssumptionsCount;
    public:
        // Assertion counts.
        static size_t getPassedCount() { return _passedCount; }
        static size_t getFailedAssertionsCount() { return _failedAssertionsCount; }
        static size_t getFailedAssumptionsCount() { return _failedAssumptionsCount; }

        // Assertion functions.
        [[noreturn]] static void fail(const std::string& message, const char* sourcefile, int linenumber);
        static void condition(bool cond, const std::string& expression, const char* sourcefile, int linenumber);
        static void assumption(bool cond, const std::string& expression, const char* sourcefile, int linenumber);

        template<typename CHAR>
        static void equalString(const std::basic_string<CHAR>& expected, const std::basic_string<CHAR>& actual, const char* sourcefile, int linenumber);

        // Assert equal for two integer or enum types. Always compare according to actual value.
        template<typename ETYPE,
                 typename ATYPE,
                 typename std::enable_if<is_intenum<ETYPE>::value>::type* = nullptr,
                 typename std::enable_if<is_intenum<ATYPE>::value>::type* = nullptr>
        static void equal(const ETYPE& expected, const ATYPE& actual, const std::string& estring, const std::string& vstring, const char* sourcefile, int linenumber);

        // Assert equal for two floating-point types. Always compare according to actual value.
        template<typename ETYPE,
                 typename ATYPE,
                 typename std::enable_if<std::is_floating_point<ETYPE>::value>::type* = nullptr,
                 typename std::enable_if<std::is_floating_point<ATYPE>::value>::type* = nullptr>
        static void equal(const ETYPE& expected, const ATYPE& actual, const std::string& estring, const std::string& vstring, const char* sourcefile, int linenumber);

        // Assert equal for string types.
        template<typename ETYPE,
                 typename ATYPE,
                 typename std::enable_if<std::is_convertible<ETYPE, std::string>::value>::type* = nullptr,
                 typename std::enable_if<std::is_convertible<ATYPE, std::string>::value>::type* = nullptr>
        static void equal(const ETYPE& expected, const ATYPE& actual, const std::string&, const std::string&, const char* sourcefile, int linenumber)
        {
            equalString(std::string(expected), std::string(actual), sourcefile, linenumber);
        }
        template<typename ETYPE,
                 typename ATYPE,
                 typename std::enable_if<std::is_convertible<ETYPE, std::u16string>::value>::type* = nullptr,
                 typename std::enable_if<std::is_convertible<ATYPE, std::u16string>::value>::type* = nullptr>
        static void equal(const ETYPE& expected, const ATYPE& actual, const std::string&, const std::string&, const char* sourcefile, int linenumber)
        {
            equalString(std::u16string(expected), std::u16string(actual), sourcefile, linenumber);
        }

        // Assert equal for pointer types.
        template<typename T>
        static void equal(const T* expected, const T* actual, const std::string& estring, const std::string& vstring, const char* sourcefile, int linenumber);

        // Assert equal for vectors of bytes.
        static void equal(const Bytes& expected, const Bytes& actual, const std::string& estring, const std::string& vstring, const char* sourcefile, int linenumber);
    };

    // Specialization for char C-strings.
    template<>
    inline void Assertions::equal<char>(const char* expected, const char* actual, const std::string&, const std::string&, const char* sourcefile, int linenumber)
    {
        equalString(std::string(expected), std::string(actual), sourcefile, linenumber);
    }

    // Specialization for char16_t C-strings.
    template<>
    inline void Assertions::equal<char16_t>(const char16_t* expected, const char16_t* actual, const std::string&, const std::string&, const char* sourcefile, int linenumber)
    {
        equalString(std::u16string(expected), std::u16string(actual), sourcefile, linenumber);
    }
}

// Out-of-line implementation of "large" templates.
template<class TEST, typename std::enable_if<std::is_base_of<tsunit::Test, TEST>::value>::type* T1>
tsunit::TestCaseWrapper<TEST,T1>::TestCaseWrapper(const std::string& testname, TestMethod method, TEST* test) :
    TestCase(testname),
    _method(method),
    _test(test)
{
}

template<class TEST, typename std::enable_if<std::is_base_of<tsunit::Test, TEST>::value>::type* T1>
void tsunit::TestCaseWrapper<TEST,T1>::run()
{
    (_test->*_method)();
}

template<class TEST,
         class EXCEP,
         typename std::enable_if<std::is_base_of<tsunit::Test, TEST>::value>::type* T1,
         typename std::enable_if<std::is_base_of<std::exception, EXCEP>::value>::type* T2>
tsunit::TestExceptionWrapper<TEST,EXCEP,T1,T2>::TestExceptionWrapper(const std::string& testname, const std::string& excepname,
                                                                     TestMethod method, TEST* test,
                                                                     const char* sourcefile, int linenumber) :
    TestCase(testname),
    _method(method),
    _excepname(excepname),
    _sourcefile(sourcefile),
    _linenumber(linenumber),
    _test(test)
{
}

template<class TEST,
         class EXCEP,
         typename std::enable_if<std::is_base_of<tsunit::Test, TEST>::value>::type* T1,
         typename std::enable_if<std::is_base_of<std::exception, EXCEP>::value>::type* T2>
void tsunit::TestExceptionWrapper<TEST,EXCEP,T1,T2>::run()
{
    try {
        (_test->*_method)();
        // Should not return, should have raised an exception.
        throw Failure("missing exception", "expected exception: " + _excepname + " (not thrown)", _sourcefile.data(), _linenumber);
    }
    catch (EXCEP&) {
        // Expected exception, exit normally.
    }
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

template<typename T>
std::string tsunit::toStringImpl(T value, const char* format)
{
    char buf[64];
    std::snprintf(buf, sizeof(buf), format, value);
    buf[sizeof(buf) - 1] = '\0';
    return std::string(buf);
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

template<typename ETYPE,
         typename ATYPE,
         typename std::enable_if<tsunit::is_intenum<ETYPE>::value>::type*,
         typename std::enable_if<tsunit::is_intenum<ATYPE>::value>::type*>
void tsunit::Assertions::equal(const ETYPE& expected, const ATYPE& actual, const std::string& estr, const std::string& astr, const char* file, int line)
{
    typedef typename underlying_type<ATYPE>::type valuetype;
    if (valuetype(expected) == valuetype(actual)) {
        ++_passedCount;
    }
    else {
        ++_failedAssertionsCount;
        const std::string details1("expected: " + toString(expected) + " (\"" + estr + "\")");
        const std::string details2("actual:   " + toString(actual) + " (\"" + astr + "\")");
        throw Failure("incorrect value", details1 + "\n" + details2, file, line);
    }
}

template<typename ETYPE,
         typename ATYPE,
         typename std::enable_if<std::is_floating_point<ETYPE>::value>::type*,
         typename std::enable_if<std::is_floating_point<ATYPE>::value>::type*>
void tsunit::Assertions::equal(const ETYPE& expected, const ATYPE& actual, const std::string& estr, const std::string& astr, const char* file, int line)
{
    constexpr double epsilon = 100.0 * static_cast<double>(std::numeric_limits<ATYPE>::epsilon());
    const double diff = std::fabs(static_cast<double>(expected) - static_cast<double>(actual));
    const double aexp = static_cast<double>(std::fabs(expected));
    const double aact = static_cast<double>(std::fabs(actual));
    if (diff <= (aexp < aact ? aact : aexp) * epsilon) {
        ++_passedCount;
    }
    else {
        ++_failedAssertionsCount;
        const std::string details1("expected: " + toString(expected) + " (\"" + estr + "\")");
        const std::string details2("actual:   " + toString(actual) + " (\"" + astr + "\")");
        throw Failure("incorrect value", details1 + "\n" + details2, file, line);
    }
}

template<typename T>
void tsunit::Assertions::equal(const T* expected, const T* actual, const std::string& estr, const std::string& astr, const char* file, int line)
{
    if (expected == actual) {
        ++_passedCount;
    }
    else {
        ++_failedAssertionsCount;
        const long long addrdiff = static_cast<long long>(reinterpret_cast<const char*>(actual) - reinterpret_cast<const char*>(expected));
        const long long typediff = static_cast<long long>(actual - expected);
        const std::string details1("expected: " + toString(expected) + " (\"" + estr + "\")");
        std::string details2("actual:   " + toString(actual) + toStringImpl(addrdiff, " (%+lld bytes"));
        if (addrdiff != typediff) {
            details2.append(toStringImpl(typediff, ", %+'lld elements"));
        }
        details2.append(", \"" + astr + "\")");
        throw Failure("incorrect value", details1 + "\n" + details2, file, line);
    }
}

template<typename CHAR>
void tsunit::Assertions::equalString(const std::basic_string<CHAR>& expected, const std::basic_string<CHAR>& actual, const char* sourcefile, int linenumber)
{
    if (expected == actual) {
        ++_passedCount;
    }
    else {
        ++_failedAssertionsCount;
        size_t diff = 0;
        while (diff < expected.size() && diff < actual.size() && expected[diff] == actual[diff]) {
            diff++;
        }
        const std::string details1("expected: \"" + toUTF8(expected) + "\"");
        const std::string details2("actual:   \"" + toUTF8(actual) + "\"");
        std::string details3;
        if (diff < expected.size() && diff < actual.size()) {
            details3 = "differ at index " + toString(diff) +
                ", expected '" + toUTF8(expected.substr(diff, 1)) +
                "', actual: '" + toUTF8(actual.substr(diff, 1)) + "'";
        }
        else {
            details3 = "lengths differ, expected: " + toString(expected.size()) + " chars, actual: " + toString(actual.size()) + " chars";
        }
        throw Failure("assertion failure, incorrect value", details1 + "\n" + details2 + "\n" + details3, sourcefile, linenumber);
    }
}

//! @endcond
