//---------------------------------------------------------------------------
//
// TSDuck sample Java application : analyze a transport stream.
// A TSProcessor is used, using plugin "analyze" and JSON output.
// The JSON output is intercepted and parsed in a Java class.
// When found, the list of services in the transport stream is displayed.
// Any other information on the TS can be similarly extracted and used.
//
// Warning: need org.json third-party package.
// Source: https://github.com/stleary/JSON-java
// Windows: execute get-org-json.ps1
// Unix (Linux, macOS): execute get-org-json.sh
//
//----------------------------------------------------------------------------

import org.json.JSONArray;
import org.json.JSONObject;

import io.tsduck.AbstractAsyncReport;
import io.tsduck.Report;
import io.tsduck.TSProcessor;

public class SampleAnalyzeTS {

    /**
     * This string is a user-defined marker to locate the JSON line in the log.
     * It can be anything that is sufficiently weird to be unique in the logs.
     */
    private static final String JSON_MARKER = "[_TS_JSON_]";

    /**
     * This method processes the parsed JSON data from the TS analysis.
     * Here, we just display the list of services.
     * @param jsonText TS analysis in a JSON string.
     */
    private static void jsonHandler(String jsonText) {
        final JSONObject root = new JSONObject(jsonText);
        final JSONArray services = root.getJSONArray("services");
        for (int i = 0; i < services.length(); i++) {
            JSONObject srv = (JSONObject) services.get(i);
            System.out.printf("Name: \"%s\", provider: \"%s\", bitrate: %d b/s\n", srv.getString("name"), srv.getString("provider"), srv.getInt("bitrate"));
        }
    }

    /**
     * A pure Java class which handles TSDuck log messages.
     */
    private static class Logger extends AbstractAsyncReport {

        /**
         * Constructor.
         * @param severity Initial severity.
         */
        public Logger() {
            super(Report.Info, false, 512);
        }

        /**
         * This method is invoked each time a message is logged by TSDuck.
         * @param severity Severity of the message.
         * @param message Message line.
         */
        @Override
        public void logMessageHandler(int severity, String message) {
            // Filter lines containing the JSON marker.
            final int pos = message.lastIndexOf(JSON_MARKER);
            if (pos >= 0) {
                // Pass the JSON text to the JSON processing routine.
                jsonHandler(message.substring(pos + JSON_MARKER.length()));
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

        // Transport stream file to analyze: command line argument, defaults to file.ts.
        final String fileName = args.length >= 1 ? args[0] : "file.ts";

        // Create a user-defined report to log multi-threaded messages.
        Logger rep = new Logger();

        // Create a TS processor using the report.
        TSProcessor tsp = new TSProcessor(rep);

        // Set the plugin chain.
        tsp.input = new String[] {"file", fileName};
        tsp.plugins = new String[][] {{"analyze", "--json-line=" + JSON_MARKER}};
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
