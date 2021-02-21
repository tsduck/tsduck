//---------------------------------------------------------------------------
//
// TSDuck sample Java application running a chain of plugins:
// Filter tables using --log-hexa-line to get binary tables in a Java class.
//
// See SampleFilterTablesEvent.java for an equivalent example using plugin
// events to get a binary content of the tables.
//
//----------------------------------------------------------------------------

import io.tsduck.AbstractAsyncReport;
import io.tsduck.Report;
import io.tsduck.TSProcessor;

public class SampleFilterTablesLog {

    /**
     * This string is a user-defined marker to locate the hexa line in the log.
     */
    private static final String LOG_PREFIX = "#TABLE#";

    /**
     * A pure Java class which handles TSDuck log messages.
     */
    private static class Logger extends AbstractAsyncReport {

        /**
         * Constructor.
         * @param severity Initial severity.
         */
        public Logger(int severity) {
            super(severity, false, 512);
        }

        /**
         * This method is invoked each time a message is logged by TSDuck.
         * @param severity Severity of the message.
         * @param message Message line.
         */
        @Override
        public void logMessageHandler(int severity, String message) {
            // Filter lines containing the marker.
            final int pos = message.lastIndexOf(LOG_PREFIX);
            if (pos >= 0) {
                // Got a hexadecimal representation of the table.
                String hexa = message.substring(pos + LOG_PREFIX.length());
                System.out.println("Table: " + hexa);
            }
            else {
                // This looks like a real log message.
                System.err.println(message);
            }
        }
    }

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        // Create a user-defined report to log multi-threaded messages.
        Logger rep = new Logger(Report.Info);

        // Create a TS processor using the report.
        TSProcessor tsp = new TSProcessor(rep);

        // Set the plugin chain.
        tsp.input = new String[] {"http", "https://github.com/tsduck/tsduck-test/raw/master/input/test-001.ts"};
        tsp.plugins = new String[][] {{"tables", "--pid", "0", "--log-hexa-line=" + LOG_PREFIX}};
        tsp.output = new String[] {"drop"};

        // Run the TS processing and wait until completion.
        tsp.start();
        tsp.waitForTermination();
        tsp.delete();

        // Terminate the asynchronous report.
        rep.terminate();
        rep.delete();
    }
}
