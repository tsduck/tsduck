//---------------------------------------------------------------------------
//
// TSDuck sample Java application : manipulate PSI/SI tables,
// convert between binary sections, XML and JSON tables.
//
// All command line arguments are interpreted as input file (.xml or .bin)
// and are loaded as initial content.
//
//----------------------------------------------------------------------------

import io.tsduck.AbstractSyncReport;
import io.tsduck.DuckContext;
import io.tsduck.Report;
import io.tsduck.SectionFile;

public class SampleTables {

    /**
     * A pure Java class which handles TSDuck log messages.
     */
    private static class Logger extends AbstractSyncReport {

        /**
         * Constructor.
         * @param severity Initial severity.
         */
        public Logger(int severity) {
            super(severity);
        }

        /**
         * This method is invoked each time a message is logged by TSDuck.
         * @param severity Severity of the message.
         * @param message Message line.
         */
        @Override
        public void logMessageHandler(int severity, String message) {
            System.out.printf("==> %s%s\n", Report.header(severity), message);
        }
    }

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        // Create a section file.
        Logger rep = new Logger(Report.Verbose);
        DuckContext duck = new DuckContext(rep);
        SectionFile file = new SectionFile(duck);

        // If command line arguments are provided, load the corresponding files.
        for (String name : args) {
            if (name.endsWith(".xml")) {
                rep.info("loading XML file " + name);
                file.loadXML(name);
            }
            else if (name.endsWith(".bin")) {
                rep.info("loading binary file " + name);
                file.loadBinary(name);
            }
            else {
                rep.error("unknown file type " + name + ", ignored");
            }
        }
        System.out.printf("After initial load: %d bytes, %d sections, %d tables\n", file.binarySize(), file.sectionsCount(), file.tablesCount());

        // Loading inline XML table.
        file.loadXML("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                     "<tsduck>\n" +
                     "  <PAT transport_stream_id=\"10\">\n" +
                     "    <service service_id=\"1\" program_map_PID=\"100\"/>\n" +
                     "    <service service_id=\"2\" program_map_PID=\"200\"/>\n" +
                     "  </PAT>\n" +
                     "</tsduck>");

        System.out.printf("After inline XML load: %d bytes, %d sections, %d tables\n", file.binarySize(), file.sectionsCount(), file.tablesCount());

        // Convert to XML and JSON.
        System.out.println("---- XML file content ----");
        System.out.println(file.toXML());
        System.out.println("---- JSON file content ----");
        System.out.println(file.toJSON());

        // Deallocate C++ resources (in reverse order from creation).
        file.delete();
        rep.delete();
        duck.delete();
    }
}
