//---------------------------------------------------------------------------
//
// TSDuck sample Java application : manipulate PSI/SI tables using various
// options (DTV standard, character sets, etc.)
//
// The input file japanese-tables.bin, located in the same directory as the
// sample source code, contains a TOT and an SDT from the Japanese DTTV.
// The standard is ISDB-T which reuses DVB tables such as TOT and SDT but
// uses different representations for character strings, time reference and
// define ISDB-specific descriptors. When interpreted with the DVB defaults,
// the strings, times and some descriptors are incorrect. The proper settings
// for Japan shall be set before deserializing the tables.
//
//----------------------------------------------------------------------------

import io.tsduck.DuckContext;
import io.tsduck.ErrReport;
import io.tsduck.Report;
import io.tsduck.SectionFile;

public class SampleJapaneseTables {

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        // Create a section file.
        Report rep = new ErrReport();
        DuckContext duck = new DuckContext(rep);
        SectionFile file = new SectionFile(duck);

        // Load a binary file containing tables which were capture on a Japanese TS.
        file.loadBinary("japanese-tables.bin");
        System.out.printf("Loaded %d bytes, %d sections, %d tables\n\n", file.binarySize(), file.sectionsCount(), file.tablesCount());

        // Convert to XML.
        System.out.println("---- XML file content with default DVB settings ----");
        System.out.println(file.toXML());

        // Use typical settings for Japan.
        duck.addStandards(DuckContext.ISDB | DuckContext.JAPAN);
        duck.setDefaultCharset("ARIB-STD-B24");
        duck.setTimeReference("JST");

        // Convert to XML again, see the difference.
        System.out.println("---- XML file content with Japanese settings ----");
        System.out.println(file.toXML());

        // Deallocate C++ resources (in reverse order from creation).
        file.delete();
        rep.delete();
        duck.delete();
    }
}
