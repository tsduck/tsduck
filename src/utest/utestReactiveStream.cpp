//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::ReactiveStream
//
//----------------------------------------------------------------------------

#include "tsReactiveStream.h"
#include "tsReactiveTextStream.h"
#include "tsBinaryFile.h"
#include "tsForkPipe.h"
#include "tsCerrReport.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ReactiveStreamTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(File);
    TSUNIT_DECLARE_TEST(Process);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    fs::path _temp_file_name {};
};

TSUNIT_REGISTER(ReactiveStreamTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ReactiveStreamTest::beforeTest()
{
    if (_temp_file_name.empty()) {
        _temp_file_name = ts::TempFile(u".tmp");
    }
    fs::remove(_temp_file_name, &ts::ErrCodeReport());
}

// Test suite cleanup method.
void ReactiveStreamTest::afterTest()
{
    fs::remove(_temp_file_name, &ts::ErrCodeReport());
}


//----------------------------------------------------------------------------
// Unitary test: write / read a file
//----------------------------------------------------------------------------

namespace {

    class WorkFile: public ts::ReactiveStreamHandlerInterface
    {
    private:
        ts::BinaryFile& _file;

        WorkFile() = delete;

    public:
        static constexpr size_t max_messages = 16;
        size_t written_messages = 0;
        size_t read_messages = 0;

        WorkFile(ts::BinaryFile& file) :
            _file(file)
        {

        }

        virtual void handleWriteStream(ts::ReactiveStream& stream, int error_code, const ts::ObjectPtr& user_data) override
        {
            tsunit::Test::debug() << "handle write: written messages: " << written_messages << ", error code: " << error_code << std::endl;
            TSUNIT_ASSERT(&stream.stream() == &_file);
            TSUNIT_ASSERT(ts::SysSuccess(error_code));
            TSUNIT_ASSERT(written_messages < max_messages);

            if (++written_messages == max_messages) {
                TSUNIT_ASSERT(_file.rewind());
                TSUNIT_ASSERT(stream.startReadStream(this));
            }
            else {
                TSUNIT_ASSERT(stream.startWriteStream(this, &written_messages, sizeof(written_messages)));
            }
        }

        virtual void handleReadStream(ts::ReactiveStream& stream, const ts::ByteBlock& data, ts::ReactiveInputControl& control, int error_code, const ts::ObjectPtr& user_data) override
        {
            tsunit::Test::debug() << "handle read: size: " << data.size() << ", read messages: " << read_messages << ", error code: " << error_code << std::endl;
            TSUNIT_ASSERT(&stream.stream() == &_file);

            if (error_code == ts::SYS_EOF) {
                stream.reactor().exitEventLoop();
                return;
            }
            TSUNIT_ASSERT(ts::SysSuccess(error_code));

            if (data.size() < sizeof(size_t)) {
                control.used_size = 0;
                control.min_next_size = sizeof(size_t);
            }
            else {
                const size_t* value = reinterpret_cast<const size_t*>(data.data());
                control.used_size = sizeof(size_t);
                control.min_next_size = sizeof(size_t);

                TSUNIT_EQUAL(read_messages, *value);
                read_messages++;
            }
        }
    };
}

TSUNIT_DEFINE_TEST(File)
{
    debug() << "Reactive stream: file name: " << _temp_file_name.string() << std::endl;
    ts::Reactor reactor(&CERR);
    ts::BinaryFile file(&reactor);
    ts::ReactiveStream rfile(reactor, file, file);
    WorkFile wfile(file);
    const size_t initial_message = 0;

    TSUNIT_ASSERT(!fs::exists(_temp_file_name));
    TSUNIT_ASSERT(file.open(_temp_file_name, ts::BinaryFile::READ | ts::BinaryFile::WRITE | ts::BinaryFile::KEEP));
    TSUNIT_ASSERT(fs::exists(_temp_file_name));

    TSUNIT_ASSERT(reactor.open());
    TSUNIT_ASSERT(rfile.startWriteStream(&wfile, &initial_message, sizeof(initial_message)));
    TSUNIT_ASSERT(reactor.processEventLoop());
    TSUNIT_ASSERT(reactor.close());
    TSUNIT_ASSERT(file.close());

    TSUNIT_EQUAL(WorkFile::max_messages, wfile.written_messages);
    TSUNIT_EQUAL(WorkFile::max_messages, wfile.read_messages);

    fs::remove(_temp_file_name, &ts::ErrCodeReport());
    TSUNIT_ASSERT(!fs::exists(_temp_file_name));
}


//----------------------------------------------------------------------------
// Unitary test: write / read a file
//----------------------------------------------------------------------------

namespace {

    class TestProcessStream: private ts::ReactiveTextStreamHandlerInterface, private ts::ReactorHandlerInterface
    {
    public:
        TestProcessStream() = delete;
        TestProcessStream(const fs::path& fname) : file_name(fname) {}
        void run();

    private:
        virtual void handleTextLine(ts::ReactiveTextStream& stream, const ts::UString& line, int error_code, const ts::ObjectPtr& user_data) override;
        virtual void handleProcessTermination(ts::Reactor& reac, ts::EventId id, int pid) override;

#if defined(TS_WINDOWS)
        const ts::UString shell_command {u"powershell"};
        const ts::UString ls_command {u"Get-ChildItem "};
        const ts::UString cat_command {u"Get-Content "};
        const ts::UString exit_command {u"exit"};
        const std::string eol {"\r\n"};
#else
        const ts::UString shell_command {u"sh"};
        const ts::UString ls_command {u"ls -l "};
        const ts::UString cat_command {u"cat "};
        const ts::UString exit_command {u"exit"};
        const std::string eol {"\n"};
#endif
        const ts::UStringVector file_content {
            u"<<== this is a recognizable content ==>>",
            u"[[[[---- second line ----]]]]"
        };

        std::ostream& debug {tsunit::Test::debug()};
        ts::Reactor reactor {&CERR};
        ts::ForkPipe process {&reactor};
        ts::ReactiveStream react_process {reactor, process, process};
        ts::ReactiveTextStream text_process {react_process, eol};

        fs::path file_name;
        ts::UString base_name {file_name.filename()};
        ts::SysProcessIdType process_pid = ts::SYS_PROCESS_ID_INVALID;
        ts::EventId process_evid {};

        enum Event {GET_LS, GET_LINE_0, GET_LINE_1, END};
        Event expected = END;
        size_t line_count = 0;
        bool got_eof = false;
        bool got_process = false;
    };

    void TestProcessStream::run()
    {
        debug << "TestProcessStream: file name: " << file_name.string() << std::endl;
        debug << "TestProcessStream: base name: " << base_name << std::endl;
        TSUNIT_ASSERT(ts::UString::Save(file_content, file_name));
        TSUNIT_ASSERT(fs::exists(file_name));

        TSUNIT_ASSERT(reactor.open());

        TSUNIT_ASSERT(process.open(shell_command, ts::ForkPipe::ASYNCHRONOUS, 0, ts::ForkPipe::STDOUT_PIPE, ts::ForkPipe::STDIN_PIPE));
        TSUNIT_ASSERT(process.isOpen());
        TSUNIT_ASSERT(!process.isBroken());
        TSUNIT_ASSERT(!process.isSynchronous());

        process_pid = process.getProcessId();
        debug << "TestProcessStream: process pid: " << process_pid << std::endl;
        TSUNIT_ASSERT(process_pid != ts::SYS_PROCESS_ID_INVALID);

        process_evid = reactor.newProcessIdTermination(this, process_pid);
        TSUNIT_ASSERT(process_evid.isValid());

        TSUNIT_ASSERT(text_process.startReadText(this));
        TSUNIT_ASSERT(text_process.startWriteLine(nullptr, ls_command + file_name));
        expected = GET_LS;

        TSUNIT_ASSERT(reactor.processEventLoop());

        TSUNIT_ASSERT(process.close());
        TSUNIT_ASSERT(reactor.close());

        debug << "TestProcessStream: received " << line_count << " lines" << std::endl;
        TSUNIT_ASSERT(line_count > 0);

        fs::remove(file_name, &ts::ErrCodeReport());
        TSUNIT_ASSERT(!fs::exists(file_name));
    }

    void TestProcessStream::handleTextLine(ts::ReactiveTextStream& stream, const ts::UString& line, int error_code, const ts::ObjectPtr& user_data)
    {
        debug << "TestProcessStream: got line \"" << line.toTrimmed(false) << "\", " << line.size() << " chars, error code: " << error_code << std::endl;
        TSUNIT_ASSERT(&stream == &text_process);
        TSUNIT_ASSERT(!got_eof);

        if (error_code == ts::SYS_EOF) {
            TSUNIT_EQUAL(END, expected);
            got_eof = true;
            if (got_eof && got_process) {
                stream.stream().reactor().exitEventLoop();
            }
        }
        else {
            TSUNIT_ASSERT(ts::SysSuccess(error_code));
            line_count++;

            if (expected == GET_LS && line.contains(base_name) && !line.contains(ls_command)) {
                // Filter file base name but exclude ls command (powershell echoes its commands).
                debug << "TestProcessStream: got ls output, sending cat command" << std::endl;
                TSUNIT_ASSERT(text_process.startWriteLine(nullptr, cat_command + file_name));
                expected = GET_LINE_0;
            }
            else if (expected == GET_LINE_0 && line.contains(file_content[0])) {
                debug << "TestProcessStream: got cat output line 0" << std::endl;
                expected = GET_LINE_1;
            }
            else if (expected == GET_LINE_1 && line.contains(file_content[1])) {
                debug << "TestProcessStream: got cat output line 2, sending exit command" << std::endl;
                TSUNIT_ASSERT(text_process.startWriteLine(nullptr, exit_command));
                expected = END;
            }
        }
    }

    void TestProcessStream::handleProcessTermination(ts::Reactor& reac, ts::EventId id, int pid)
    {
        debug << "TestProcessStream: process terminated, pid: " << pid << std::endl;
        TSUNIT_ASSERT(&reac == &reactor);
        TSUNIT_ASSERT(id == process_evid);
        TSUNIT_EQUAL(process_pid, pid);

        got_process = true;
        if (got_eof && got_process) {
            reactor.exitEventLoop();
        }
    }
}

TSUNIT_DEFINE_TEST(Process)
{
    TestProcessStream test(_temp_file_name);
    test.run();
}
