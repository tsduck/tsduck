//---------------------------------------------------------------------------
//
// TSDuck sample Java application : reorganize EIT's according to DVB rules
//
//----------------------------------------------------------------------------

import io.tsduck.DuckContext;
import io.tsduck.ErrReport;
import io.tsduck.Report;
import io.tsduck.SectionFile;

public class SampleEIT {

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        // Create a section file.
        Report rep = new ErrReport();
        DuckContext duck = new DuckContext(rep);
        SectionFile file = new SectionFile(duck);

        // Loading inline XML EIT's. There is one EITp/f without following event and an EIT
        // Schedule with two events which should not be grouped and in the wrong order.
        file.loadXML("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                     "<tsduck>\n" +
                     "  <EIT type=\"pf\" service_id=\"1\" transport_stream_id=\"10\" original_network_id=\"20\">\n" +
                     "    <event event_id=\"1000\" start_time=\"2020-06-10 15:00:00\" duration=\"00:10:00\">\n" +
                     "      <short_event_descriptor language_code=\"eng\">\n" +
                     "        <event_name>Event 1000 name</event_name>\n" +
                     "        <text>Event 1000 text</text>\n" +
                     "      </short_event_descriptor>\n" +
                     "    </event>\n" +
                     "  </EIT>\n" +
                     "  <EIT type=\"0\" service_id=\"1\" transport_stream_id=\"10\" original_network_id=\"20\">\n" +
                     "    <event event_id=\"2001\" start_time=\"2020-06-13 11:30:00\" duration=\"00:10:00\">\n" +
                     "      <short_event_descriptor language_code=\"foo\">\n" +
                     "        <event_name>Event 2001 name</event_name>\n" +
                     "        <text>Event 2001 text</text>\n" +
                     "      </short_event_descriptor>\n" +
                     "    </event>\n" +
                     "    <event event_id=\"2000\" start_time=\"2020-06-10 18:30:00\" duration=\"00:10:00\">\n" +
                     "      <short_event_descriptor language_code=\"foo\">\n" +
                     "        <event_name>Event 2000 name</event_name>\n" +
                     "        <text>Event 2000 text</text>\n" +
                     "      </short_event_descriptor>\n" +
                     "    </event>\n" +
                     "  </EIT>\n" +
                     "</tsduck>");

        // Reorganize EIT sections according to ETSI TS 101 211 rules.
        file.reorganizeEITs();

        // Save binary section file.
        file.saveBinary("eit.bin");

        // Launch a "tstabdump" command to dump the EIT sections.
        SampleUtils.execute("tstabdump eit.bin");

        // Deallocate C++ resources (in reverse order from creation).
        file.delete();
        rep.delete();
        duck.delete();
    }
}
