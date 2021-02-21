//---------------------------------------------------------------------------
//
// TSDuck sample Java application : analyze the SDT in a transport stream.
// A TSProcessor is used, using plugin "tables" and XML output.
// The XML output is intercepted and parsed in a Java class.
// When found, various information are extracted from the XML structure.
//
//----------------------------------------------------------------------------

import java.io.ByteArrayInputStream;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import io.tsduck.AbstractAsyncReport;
import io.tsduck.Report;
import io.tsduck.TSProcessor;

public class SampleAnalyzeSDT {

    /**
     * This string is a user-defined marker to locate the XML line in the log.
     * It can be anything that is sufficiently weird to be unique in the logs.
     */
    private static final String XML_MARKER = "[_SDT_XML_]";

    /**
     * This method processes the XML data from the SDT.
     * Here, we just display the list of services.
     * @param xmlText SDT description in an XML string.
     */
    private static void xmlHandler(String xmlText) {
        try {
            // Create a DOM document from the XML text.
            final DocumentBuilder builder = DocumentBuilderFactory.newInstance().newDocumentBuilder();
            final Document doc = builder.parse(new ByteArrayInputStream(xmlText.getBytes("UTF-8")));
            final Element root = doc.getDocumentElement();

            // Loop on all SDT in XML document (there should be only one).
            final NodeList sdtList = root.getElementsByTagName("SDT");
            for (int sdtIndex = 0; sdtIndex < sdtList.getLength(); sdtIndex++) {
                Element sdt = (Element) sdtList.item(sdtIndex);

                // Display some SDT properties.
                int version = Integer.decode(sdt.getAttribute("version"));
                int tsId = Integer.decode(sdt.getAttribute("transport_stream_id"));
                int nwId = Integer.decode(sdt.getAttribute("original_network_id"));
                System.out.printf("SDT version: %d, TS id: %d, original network id: %d\n", version, tsId, nwId);

                // Loop on all services in the SDT.
                final NodeList srvList = sdt.getElementsByTagName("service");
                for (int srvIndex = 0; srvIndex < srvList.getLength(); srvIndex++) {
                    Element srv = (Element) srvList.item(srvIndex);
                    int srvId = Integer.decode(srv.getAttribute("service_id"));
                    String srvName = "(unknown)";
                    String srvProvider = "(unknown)";
                    // Find first service descriptor, if there is one.
                    final NodeList descList = srv.getElementsByTagName("service_descriptor");
                    if (descList.getLength() > 0) {
                        Element desc = (Element) descList.item(0);
                        srvName = desc.getAttribute("service_name");
                        srvProvider = desc.getAttribute("service_provider_name");
                    }
                    System.out.printf("Service id: %d, name: \"%s\", provider: \"%s\"\n", srvId, srvName, srvProvider);
                }
            }
        }
        catch (Exception e) {
            System.err.printf("Exception in XML handler: %s\n", e.getMessage());
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
            // Filter lines containing the XML marker.
            final int pos = message.lastIndexOf(XML_MARKER);
            if (pos >= 0) {
                // Pass the XML text to the XML processing routine.
                xmlHandler(message.substring(pos + XML_MARKER.length()));
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
        Logger rep = new Logger(Report.Info);

        // Create a TS processor using the report.
        TSProcessor tsp = new TSProcessor(rep);

        // Set the plugin chain.
        tsp.input = new String[] {"file", fileName};
        tsp.plugins = new String[][] {{"tables", "--pid", "0x11", "--tid", "0x42", "--max-tables", "1", "--log-xml-line=" + XML_MARKER}};
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
