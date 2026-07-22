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
#include "tsBinaryFile.h"
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
// Unitary tests
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
