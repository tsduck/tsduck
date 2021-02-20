//---------------------------------------------------------------------------
//
// TSDuck sample Java application running a chain of plugins
// and handling the TSDuck log messages in a Java class.
//
//----------------------------------------------------------------------------

import io.tsduck.AbstractAsyncReport;
import io.tsduck.Report;
import io.tsduck.TSProcessor;

public class SampleMessageHandling {

    /**
     * A pure Java class which handles TSDuck log messages.
     */
    private static class Logger extends AbstractAsyncReport {

        /**
         * Constructor.
         * @param severity Initial severity.
         */
        public Logger() {
            super(Report.Verbose, false, 512);
        }

        /**
         * This method is invoked each time a message is logged by TSDuck.
         * @param severity Severity of the message.
         * @param message Message line.
         */
        @Override
        public void logMessageHandler(int severity, String message) {
            System.out.printf("Severity: %d, message: %s%s\n", severity, Report.header(severity), message);
        }
    }

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        /*
         * Create a report to log multi-threaded messages.
         * In this example, this is a user-defined Java class which collects messages.
         */
        Logger rep = new Logger();

        /*
         * Checking that it works as well with Java application messages.
         */
        rep.info("application message");

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
