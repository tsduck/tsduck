//---------------------------------------------------------------------------
//
// TSDuck sample Java application : PSI/SI tables using Java byte arrays.
//
//----------------------------------------------------------------------------

import io.tsduck.DuckContext;
import io.tsduck.ErrReport;
import io.tsduck.Report;
import io.tsduck.SectionFile;

public class SampleBinaryTables {

    private static final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();

    /**
     * Convert a byte array as an hexadecimal string.
     * @param bytes The byte array to convert.
     * @return The hexadecimal string.
     */
    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        for (int j = 0; j < bytes.length; j++) {
            final int v = bytes[j] & 0xFF;
            hexChars[j * 2] = HEX_ARRAY[v >>> 4];
            hexChars[j * 2 + 1] = HEX_ARRAY[v & 0x0F];
        }
        return new String(hexChars);
    }

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        // Create a section file.
        Report rep = new ErrReport();
        DuckContext duck = new DuckContext(rep);
        SectionFile file1 = new SectionFile(duck);

        // Loading inline XML table.
        file1.loadXML("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
                      "<tsduck>\n" +
                      "  <PAT transport_stream_id=\"10\">\n" +
                      "    <service service_id=\"1\" program_map_PID=\"100\"/>\n" +
                      "    <service service_id=\"2\" program_map_PID=\"200\"/>\n" +
                      "  </PAT>\n" +
                      "</tsduck>");

        // Convert to binary.
        byte[] data = file1.toBinary();
        System.out.println("---- Binary content ----");
        System.out.println(bytesToHex(data));
        System.out.println();

        // Build another section file and load the binary data.
        SectionFile file2 = new SectionFile(duck);
        file2.fromBinary(data);

        // Convert the second section file to XML.
        System.out.println("---- XML content  ----");
        System.out.println(file2.toXML());

        // Deallocate C++ resources (in reverse order from creation).
        file2.delete();
        file1.delete();
        rep.delete();
        duck.delete();
    }
}
