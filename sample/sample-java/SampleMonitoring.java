//---------------------------------------------------------------------------
//
// TSDuck sample Java application running a chain of plugins on the long
// run with CPU and memory monitoring.
//
//----------------------------------------------------------------------------

import java.io.File;

import io.tsduck.AsyncReport;
import io.tsduck.Info;
import io.tsduck.Report;
import io.tsduck.SystemMonitor;
import io.tsduck.TSProcessor;

public class SampleMonitoring {

    private static final String URL = "https://tsduck.io/streams/france-dttv/tnt-uhf30-546MHz-2019-01-22.ts";

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) throws Exception {

        System.out.printf("TSDuck version: %s\n", Info.version());

        // Create an asynchronous report to log multi-threaded messages.
        AsyncReport rep = new AsyncReport();
        rep.setMaxSeverity(Report.Verbose);

        // Create and start a background system monitor.
        SystemMonitor mon = new SystemMonitor(rep);
        mon.start();

        // Build a temporary file name to download a real TS file.
        File tsfile = File.createTempFile("temp", ".ts", null);
        tsfile.deleteOnExit();

        // First phase: Download the TS file:
        System.out.printf("Downloading %s to %s ...\n", URL, tsfile.getPath());

        TSProcessor tsp = new TSProcessor(rep);
        tsp.input = new String[] {"http", URL};
        tsp.output = new String[] {"file", tsfile.getPath()};
        tsp.start();
        tsp.waitForTermination();
        tsp.delete();

        // Second phase: Play the file at regulated speed several times.
        // Must use another instance of TSProcessor.
        System.out.printf("Playing %s ...\n", tsfile.getPath());

        tsp = new TSProcessor(rep);
        tsp.input = new String[] {"file", tsfile.getPath(), "--repeat", "2"};
        tsp.plugins = new String[][] {{"regulate"}};
        tsp.output = new String[] {"drop"};
        tsp.start();
        tsp.waitForTermination();
        tsp.delete();

        // Terminate the system monitor.
        mon.stop();
        mon.waitForTermination();
        mon.delete();

        // Terminate the asynchronous report.
        rep.terminate();
        rep.delete();
    }
}
