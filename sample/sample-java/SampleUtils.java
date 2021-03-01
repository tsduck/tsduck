//---------------------------------------------------------------------------
//
// TSDuck sample Java application: Common utilities.
//
//----------------------------------------------------------------------------

import java.io.BufferedReader;
import java.io.InputStreamReader;

public class SampleUtils {

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
     * Execute a command and print its output.
     * @param cmd The shell command line to execute.
     */
    public static void execute(String cmd) {
        try {
            Process process = Runtime.getRuntime().exec(cmd);
            BufferedReader input = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line;
            while ((line = input.readLine()) != null) {
                System.out.println(line);
            }
            process.waitFor();
        }
        catch (Exception e) {
            System.out.println(e.toString());
            e.printStackTrace();
        }
    }
}
