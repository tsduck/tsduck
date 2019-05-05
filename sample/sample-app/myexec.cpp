#include "tsduck.h"

class SimpleTSP : public ts::TSP
{
public:
    // Constructor, using a message log level.
    SimpleTSP(int level) : TSP(level) {}

    // Implementation of "joint termination", inherited from TSP.
    virtual void useJointTermination(bool on) override;
    virtual void jointTerminate() override;
    virtual bool useJointTermination() const override;
    virtual bool thisJointTerminated() const override;

protected:
    // Inherited from Report (via TSP)
    virtual void writeLog(int severity, const ts::UString& msg) override;
};

void SimpleTSP::useJointTermination(bool on)
{
}

void SimpleTSP::jointTerminate()
{
}

bool SimpleTSP::useJointTermination() const
{
    return false;
}

bool SimpleTSP::thisJointTerminated() const
{
    return false;
}

void SimpleTSP::writeLog(int severity, const ts::UString& msg)
{
    std::cout << msg << std::endl;
}

int main(int argc, char* argv[])
{
    // Use debug output if first argument is -d.
    SimpleTSP tsp(argc > 1 && ::strcmp(argv[1], "-d") == 0 ? ts::Severity::Debug : ts::Severity::Verbose);

    // Demonstrate formatting capabilities
    tsp.verbose(u"format, i=%d, s=%s", {12, "abc"});

    // Further logging
    tsp.verbose(u"TSDuck version is: %s", {ts::GetVersion()});
    tsp.verbose(u"TSDuck built on: %s", {ts::GetVersion(ts::VERSION_DATE)});

    // Load an input plugin, processor plugin, and an output plugin and use them in sequence.
    // For simplicity, run all plugins on a single thread, rather than the model used in the tsp executable,
    // which runs each plugin in its own thread.

    // Use hardcoded plugins and input values.
    // For the input plugin, use the file plugin with filename input test_input.ts.
    // For the processor plugin, use the svrename plugin and rename the service with service ID 1 to service ID 3.
    // For the output plugin, use the file plugin with file output test_output.ts.

    // Note:  for the plugin repository to be able to find the file plugin (usually with base file name
    // "tsplugin_file"), or any other plugin shared library, there are a few options:
    // a) Run the sample-app binary from the same directory in which the TSDuck binaries are found.
    // b) Use the directory in which the TSDuck binaries are found as the working directory.
    // c) Set the TSPLUGINS_PATH environment variable to contain a list of paths in which TSDuck plugins
    //    may be found.

    // Create instance of input plugin
    ts::NewInputProfile inputPluginAllocator = ts::PluginRepository::Instance()->getInput(u"file", tsp);
    if (inputPluginAllocator == nullptr) {
        return EXIT_FAILURE;
    }

    // Using SafePtr to allow plugin objects to self-destruct when they are no longer needed.
    ts::SafePtr<ts::InputPlugin> inputPlugin(inputPluginAllocator(&tsp));
    if (inputPlugin.isNull()) {
        return EXIT_FAILURE;
    }

    // Create instance of processor plugin
    ts::NewProcessorProfile processorPluginAllocator = ts::PluginRepository::Instance()->getProcessor(u"svrename", tsp);
    if (processorPluginAllocator == nullptr) {
        return EXIT_FAILURE;
    }

    ts::SafePtr<ts::ProcessorPlugin> processorPlugin(processorPluginAllocator(&tsp));
    if (processorPlugin.isNull()) {
        return EXIT_FAILURE;
    }

    // Create instance of output plugin
    ts::NewOutputProfile outputPluginAllocator = ts::PluginRepository::Instance()->getOutput(u"file", tsp);
    if (outputPluginAllocator == nullptr) {
        return EXIT_FAILURE;
    }

    ts::SafePtr<ts::OutputPlugin> outputPlugin(outputPluginAllocator(&tsp));
    if (outputPlugin.isNull()) {
        return EXIT_FAILURE;
    }

    // Setup and initiate input plugin
    bool success = inputPlugin->analyze(u"file", {u"test_input.ts"});
    if (!success) {
        return EXIT_FAILURE;
    }

    success = inputPlugin->getOptions() && inputPlugin->start();
    if (!success) {
        return EXIT_FAILURE;
    }

    // Setup and initiate processor plugin
    success = processorPlugin->analyze(u"svrename", {u"1", u"--id", u"3"});
    if (!success) {
        inputPlugin->stop();
        return EXIT_FAILURE;
    }

    success = processorPlugin->getOptions() && processorPlugin->start();
    if (!success) {
        inputPlugin->stop();
        return EXIT_FAILURE;
    }

    // Setup and initiate output plugin
    success = outputPlugin->analyze(u"file", {u"test_output.ts"});
    if (!success) {
        inputPlugin->stop();
        processorPlugin->stop();
        return EXIT_FAILURE;
    }

    success = outputPlugin->getOptions() && outputPlugin->start();
    if (!success) {
        inputPlugin->stop();
        processorPlugin->stop();
        return EXIT_FAILURE;
    }

    // Process arrays of TS packets through the plugin pipeline (input -> processor -> output).

    // Using a packet buffer that can hold 43 188 byte TS packets because this is the largest amount that will fit
    // into 8192 bytes (8K), and 8K is considered to be a reasonable buffer size when reading or writing to file.

    // Notes:  Fetching up to 43 packets at a time may not improve performance over processing individual packets one
    // at a time.  This is because modern operating systems tend to cache file data, and in the case when files are being
    // read sequentially, as is the case here with the input plugin, the file system will almost certainly prefetch
    // the file in chunks much larger than 188 bytes, and subsequent reads will actually be reading from memory, not disk.
    // On the output side, the file system will cache writes up to a certain point and then flush them to disk.  An
    // implementation that processes packets one at a time will also be simpler, particularly because it is easier to
    // deal with dropped packets.  Further, while the tsp executable gives each plugin its own thread and uses thread-safe
    // mechanisms to make packets available to the next plugin in the pipeline, depending on the overhead of the input
    // plugin, processor plugin(s), and the output plugin, there may be no benefit, from a performance standpoint, to doing
    // so, and the use of additional threads could even potentially have a slight performance impact.  If we are dealing with
    // very fast operations (as in how quickly InputPlugin::receive(), ProcessorPlugin::processPacket(), and
    // OutputPlugin::send() return), it may be perfectly reasonable to run the entire pipeline from a single thread.

#define BUFSIZE 43
    ts::TSPacket packetBuffer[BUFSIZE];

    while (1) {
        size_t packetsReceived = inputPlugin->receive(packetBuffer, BUFSIZE);
        if (!packetsReceived) {
            break;
        }

        bool aborted = false;
        size_t i;
        size_t startIndex = 0;
        for (i = 0; i < packetsReceived; i++) {
            ts::TSPacketMetadata metadata;
            ts::ProcessorPlugin::Status processorStatus = processorPlugin->processPacket(packetBuffer[i], metadata);

            switch (processorStatus) {
                case ts::ProcessorPlugin::Status::TSP_OK:
                    break;

                case ts::ProcessorPlugin::Status::TSP_NULL:
                    packetBuffer[i] = ts::NullPacket;
                    break;

                case ts::ProcessorPlugin::Status::TSP_END:
                    aborted = true;
                    break;

                case ts::ProcessorPlugin::Status::TSP_DROP: {
                    // this packet should be discarded
                    // send contiguous array of packets up till this point to output plugin
                    const size_t numberPackets = i - startIndex;
                    if (numberPackets) {
                        success = outputPlugin->send(&packetBuffer[startIndex], numberPackets);
                        if (!success) {
                            aborted = true;
                            break;
                        }
                    }

                    startIndex = i + 1;
                    break;
                }

                default:
                    // shouldn't occur
                    aborted = true;
                    break;
            }
        }

        if (aborted) {
            break;
        }

        const size_t numberPackets = packetsReceived - startIndex;
        if (numberPackets) {
            success = outputPlugin->send(&packetBuffer[startIndex], numberPackets);
            if (!success) {
                break;
            }
        }
    }

    // Stop all plugins
    inputPlugin->stop();
    processorPlugin->stop();
    outputPlugin->stop();

    return EXIT_SUCCESS;
}
