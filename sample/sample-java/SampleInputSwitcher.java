//---------------------------------------------------------------------------
//
// TSDuck sample Java application running an input switcher.
//
//----------------------------------------------------------------------------

import io.tsduck.AsyncReport;
import io.tsduck.InputSwitcher;
import io.tsduck.Report;

public class SampleInputSwitcher {

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        // Create an Input Switcher with an asynchronous logger.
        AsyncReport rep = new AsyncReport(Report.Verbose, false, false, 512);
        InputSwitcher tsswitch = new InputSwitcher(rep);

        tsswitch.firstInput = 2;     // Let's start with input #2.
        tsswitch.cycleCount = 3;     // Number of input cycles to execute (0 = infinite).
        tsswitch.appName = "demo";   // Informational only, for log messages

        // Set the plugins.
        tsswitch.inputs = new String[][] {
            {"craft", "--count", "2", "--pid", "100"},
            {"null", "3"},
            {"http", "https://tsduck.io/streams/test-patterns/test-2packets-02-03.ts"},
        };
        tsswitch.output = new String[] {"file", "sample-input-switcher-out.ts"};

        // Demonstrate switching event notification.
        tsswitch.eventCommand = "echo \"=== EVENT\"";
        tsswitch.eventUDPAddress = "localhost";
        tsswitch.eventUDPPort = 4444;

        // Run the input switcher session and wait until completion.
        tsswitch.start();
        tsswitch.waitForTermination();
        tsswitch.delete();

        // Terminate the asynchronous report.
        rep.terminate();
        rep.delete();

        // Checking the result:
        // $ tsdump sample-input-switcher-out.ts | grep PID:
        //   PID: 2 (0x0002), header size: 4, sync: 0x47       <-- Cycle #0, starting at plugin #2
        //   PID: 3 (0x0003), header size: 4, sync: 0x47
        //   PID: 100 (0x0064), header size: 4, sync: 0x47     <-- Cycle #1, plugin #0
        //   PID: 100 (0x0064), header size: 4, sync: 0x47
        //   PID: 8191 (0x1FFF), header size: 4, sync: 0x47    <-- Cycle #1, plugin #1
        //   PID: 8191 (0x1FFF), header size: 4, sync: 0x47
        //   PID: 8191 (0x1FFF), header size: 4, sync: 0x47
        //   PID: 2 (0x0002), header size: 4, sync: 0x47       <-- Cycle #1, plugin #2
        //   PID: 3 (0x0003), header size: 4, sync: 0x47
        //   PID: 100 (0x0064), header size: 4, sync: 0x47     <-- Cycle #2, plugin #0
        //   PID: 100 (0x0064), header size: 4, sync: 0x47
        //   PID: 8191 (0x1FFF), header size: 4, sync: 0x47    <-- Cycle #2, plugin #1
        //   PID: 8191 (0x1FFF), header size: 4, sync: 0x47
        //   PID: 8191 (0x1FFF), header size: 4, sync: 0x47
        //   PID: 2 (0x0002), header size: 4, sync: 0x47       <-- Cycle #2, plugin #2
        //   PID: 3 (0x0003), header size: 4, sync: 0x47
    }
}
