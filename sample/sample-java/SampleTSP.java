//---------------------------------------------------------------------------
//
// TSDuck sample Java application running a chain of plugins.
//
//----------------------------------------------------------------------------

import io.tsduck.AsyncReport;
import io.tsduck.Info;
import io.tsduck.Report;
import io.tsduck.TSProcessor;

public class SampleTSP {

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        System.out.printf("TSDuck version: %s\n", Info.version());

        /*
         * Create an asynchronous report to log multi-threaded messages.
         * Initial level is verbose, using time-stamped messages.
         */
        AsyncReport rep = new AsyncReport(Report.Verbose, false, true, 512);

        /*
         * Create a TS processor using the report.
         */
        TSProcessor tsp = new TSProcessor(rep);

        /*
         * Set some global TS processing options.
         */
        tsp.addInputStuffingNull = 1;    // one null packet ...
        tsp.addInputStuffingInput = 10;  // ... every 10 input packets
        tsp.bitrate = 1000000;           // nominal bitrate is 1 Mb/s
        tsp.appName = "demo";            // informational only, for log messages

        /*
         * Set the plugin chain.
         */
        tsp.input = new String[] {"craft", "--count", "1000", "--pid", "100", "--payload-pattern", "0123"};
        tsp.plugins = new String[][] {
            {"until", "--packet", "100"},
            {"count"},
        };
        tsp.output = new String[] {"drop"};

        /*
         * Run the TS processing and wait until completion.
         */
        tsp.start();
        tsp.waitForTermination();
        tsp.delete();

        /*
         * Terminate the asynchronous report.
         */
        rep.terminate();
        rep.delete();
    }
}
