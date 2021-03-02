//---------------------------------------------------------------------------
//
// TSDuck sample Java application using tsp memory plugins.
//
// This sample application uses the input and output memory plugins at the
// same time. Real applications may use one of them only.
//
//----------------------------------------------------------------------------

import java.util.Arrays;

import io.tsduck.AbstractPluginEventHandler;
import io.tsduck.AsyncReport;
import io.tsduck.PluginEventContext;
import io.tsduck.Report;
import io.tsduck.TS;
import io.tsduck.TSProcessor;

public class SampleMemoryPlugins {

    /**
     * An event handler for memory input plugin.
     * It is invoked by the "memory" input plugin each time TS packets are needed.
     *
     * For the purpose of demonstration, this class sends a predefined list of
     * transport stream packets one by one. A real application should get its
     * input packets from somewhere else and return them in larger chunks.
     */
    private static class InputHandler extends AbstractPluginEventHandler {

        // A predefined list of transport stream packets used as input.
        static private final byte[][] PACKETS = new byte[][] {
            {(byte)0x47, (byte)0x00, (byte)0x64, (byte)0x10, (byte)0x27, (byte)0xCD, (byte)0x88, (byte)0x8B, (byte)0x72, (byte)0x99, (byte)0xB9, (byte)0x7E, (byte)0x9F, (byte)0xC8, (byte)0x11, (byte)0xD8,
             (byte)0x34, (byte)0xEC, (byte)0xFE, (byte)0x57, (byte)0xFF, (byte)0xF9, (byte)0x80, (byte)0x2F, (byte)0xF5, (byte)0xCD, (byte)0x0B, (byte)0x05, (byte)0xD0, (byte)0x9F, (byte)0x21, (byte)0x23,
             (byte)0x60, (byte)0x47, (byte)0x28, (byte)0x80, (byte)0x4E, (byte)0xE3, (byte)0xAD, (byte)0x89, (byte)0xFF, (byte)0x92, (byte)0x85, (byte)0x93, (byte)0x5E, (byte)0x41, (byte)0xB8, (byte)0xCF,
             (byte)0xED, (byte)0xA8, (byte)0x41, (byte)0xEC, (byte)0x13, (byte)0x22, (byte)0x7B, (byte)0x5C, (byte)0x37, (byte)0xD2, (byte)0xDB, (byte)0x06, (byte)0xB1, (byte)0xB5, (byte)0xF5, (byte)0x62,
             (byte)0x9D, (byte)0x3C, (byte)0x48, (byte)0x99, (byte)0x5B, (byte)0xDD, (byte)0xF9, (byte)0x66, (byte)0x1A, (byte)0xD1, (byte)0xE9, (byte)0xE1, (byte)0x43, (byte)0x6F, (byte)0xDF, (byte)0x17,
             (byte)0x0E, (byte)0x85, (byte)0xD1, (byte)0x98, (byte)0x97, (byte)0x64, (byte)0xFB, (byte)0x18, (byte)0xF9, (byte)0x91, (byte)0xB6, (byte)0x7D, (byte)0x5A, (byte)0xBF, (byte)0x2D, (byte)0xAD,
             (byte)0xCD, (byte)0x28, (byte)0xC2, (byte)0xDE, (byte)0xB2, (byte)0x47, (byte)0x92, (byte)0x57, (byte)0xF5, (byte)0x5B, (byte)0x1C, (byte)0x71, (byte)0xC8, (byte)0xF0, (byte)0xC9, (byte)0x21,
             (byte)0x3E, (byte)0x0D, (byte)0xC3, (byte)0xF8, (byte)0x32, (byte)0x15, (byte)0xEF, (byte)0x55, (byte)0x81, (byte)0xF6, (byte)0x05, (byte)0xA8, (byte)0x0D, (byte)0x83, (byte)0xD2, (byte)0x8D,
             (byte)0x34, (byte)0x29, (byte)0x1C, (byte)0x2A, (byte)0x01, (byte)0x07, (byte)0xE4, (byte)0x50, (byte)0x48, (byte)0x55, (byte)0xD4, (byte)0xA8, (byte)0xDF, (byte)0x94, (byte)0x47, (byte)0x4C,
             (byte)0x2A, (byte)0x0A, (byte)0x84, (byte)0xB5, (byte)0x6F, (byte)0xD4, (byte)0xF3, (byte)0x87, (byte)0x8C, (byte)0xB1, (byte)0x2B, (byte)0x47, (byte)0x85, (byte)0x0A, (byte)0x2D, (byte)0x1D,
             (byte)0x9A, (byte)0x2B, (byte)0x18, (byte)0x45, (byte)0x99, (byte)0x01, (byte)0xC1, (byte)0x93, (byte)0x79, (byte)0xFB, (byte)0x94, (byte)0xF3, (byte)0x52, (byte)0x24, (byte)0x1D, (byte)0xB4,
             (byte)0xD9, (byte)0x2F, (byte)0x2D, (byte)0x95, (byte)0xD6, (byte)0xFA, (byte)0xA8, (byte)0xA5, (byte)0x46, (byte)0x62, (byte)0x54, (byte)0x01},

            {(byte)0x47, (byte)0x00, (byte)0x64, (byte)0x10, (byte)0x9E, (byte)0x9E, (byte)0xD4, (byte)0x70, (byte)0x06, (byte)0xDB, (byte)0x11, (byte)0xD4, (byte)0x7A, (byte)0xA5, (byte)0x97, (byte)0x11,
             (byte)0x2E, (byte)0xBC, (byte)0x7D, (byte)0x62, (byte)0x7C, (byte)0xA6, (byte)0x90, (byte)0xB8, (byte)0x49, (byte)0x72, (byte)0x1A, (byte)0xBD, (byte)0xA0, (byte)0x04, (byte)0x0D, (byte)0x59,
             (byte)0x42, (byte)0xDD, (byte)0x91, (byte)0x29, (byte)0xA4, (byte)0xD4, (byte)0x21, (byte)0xD1, (byte)0x63, (byte)0xD8, (byte)0x36, (byte)0xC6, (byte)0xAE, (byte)0xD2, (byte)0x65, (byte)0xE0,
             (byte)0xA5, (byte)0x85, (byte)0xE9, (byte)0xF9, (byte)0xD9, (byte)0x7E, (byte)0x44, (byte)0xF1, (byte)0x7B, (byte)0xF2, (byte)0x49, (byte)0x19, (byte)0x22, (byte)0xEC, (byte)0x8D, (byte)0x9B,
             (byte)0xBC, (byte)0xAB, (byte)0xE3, (byte)0xC9, (byte)0x0A, (byte)0x98, (byte)0x3F, (byte)0xF3, (byte)0xAF, (byte)0x3B, (byte)0x1B, (byte)0xE9, (byte)0x52, (byte)0xCF, (byte)0x70, (byte)0x8A,
             (byte)0x2C, (byte)0x22, (byte)0xF9, (byte)0xF8, (byte)0x93, (byte)0x7B, (byte)0xAF, (byte)0xD8, (byte)0x99, (byte)0x97, (byte)0x4F, (byte)0x54, (byte)0x59, (byte)0x3F, (byte)0x69, (byte)0xC4,
             (byte)0xFB, (byte)0xD6, (byte)0x93, (byte)0xC8, (byte)0x00, (byte)0x80, (byte)0x08, (byte)0x8D, (byte)0x1B, (byte)0x97, (byte)0xB2, (byte)0xDD, (byte)0x9D, (byte)0x39, (byte)0x2B, (byte)0x55,
             (byte)0xA8, (byte)0xAA, (byte)0x2D, (byte)0x15, (byte)0xDB, (byte)0xF4, (byte)0x3E, (byte)0xC6, (byte)0x4C, (byte)0x38, (byte)0x44, (byte)0x3E, (byte)0x04, (byte)0x05, (byte)0x83, (byte)0xAA,
             (byte)0x52, (byte)0x8E, (byte)0x81, (byte)0xD6, (byte)0x69, (byte)0x20, (byte)0x33, (byte)0xAA, (byte)0xB6, (byte)0x5C, (byte)0xFF, (byte)0xB8, (byte)0xCB, (byte)0xE5, (byte)0xCC, (byte)0xAB,
             (byte)0xB8, (byte)0x73, (byte)0xE8, (byte)0xC3, (byte)0xDE, (byte)0x25, (byte)0x38, (byte)0xCA, (byte)0x8F, (byte)0x38, (byte)0xF4, (byte)0x33, (byte)0xEE, (byte)0xA4, (byte)0xA6, (byte)0x57,
             (byte)0x17, (byte)0x2F, (byte)0xC2, (byte)0xCB, (byte)0xE9, (byte)0xD7, (byte)0xCC, (byte)0x42, (byte)0xCA, (byte)0x27, (byte)0xFF, (byte)0xDA, (byte)0xF6, (byte)0x5D, (byte)0x53, (byte)0x86,
             (byte)0xBB, (byte)0x7E, (byte)0x0A, (byte)0x62, (byte)0xA9, (byte)0x83, (byte)0xD0, (byte)0x54, (byte)0x2D, (byte)0x50, (byte)0x1F, (byte)0xBC},

            {(byte)0x47, (byte)0x00, (byte)0x64, (byte)0x10, (byte)0x37, (byte)0x71, (byte)0x69, (byte)0xA4, (byte)0x09, (byte)0xF0, (byte)0xE4, (byte)0x7F, (byte)0xCC, (byte)0x4C, (byte)0xDA, (byte)0x9F,
             (byte)0x21, (byte)0x90, (byte)0x12, (byte)0x7A, (byte)0xEC, (byte)0x04, (byte)0x48, (byte)0xC8, (byte)0x6C, (byte)0x9D, (byte)0x0C, (byte)0xF2, (byte)0x3E, (byte)0x3E, (byte)0x5C, (byte)0xCA,
             (byte)0xF3, (byte)0x98, (byte)0xF3, (byte)0xCC, (byte)0x09, (byte)0x44, (byte)0x61, (byte)0x30, (byte)0x91, (byte)0x1B, (byte)0xAA, (byte)0x66, (byte)0x8E, (byte)0x8A, (byte)0x2D, (byte)0xDC,
             (byte)0x23, (byte)0x9B, (byte)0xF9, (byte)0xE0, (byte)0x8F, (byte)0x67, (byte)0x79, (byte)0xDD, (byte)0x7C, (byte)0x6C, (byte)0xF8, (byte)0x35, (byte)0x82, (byte)0x13, (byte)0x8B, (byte)0x23,
             (byte)0xFF, (byte)0x1C, (byte)0xE4, (byte)0x5B, (byte)0x53, (byte)0x17, (byte)0x95, (byte)0xD0, (byte)0x08, (byte)0x40, (byte)0xAE, (byte)0x6B, (byte)0xFF, (byte)0xF9, (byte)0x80, (byte)0x27,
             (byte)0x6F, (byte)0xE1, (byte)0xF2, (byte)0x82, (byte)0x85, (byte)0xB6, (byte)0xF5, (byte)0x00, (byte)0x5F, (byte)0x49, (byte)0xF4, (byte)0x67, (byte)0x45, (byte)0x84, (byte)0xE8, (byte)0x76,
             (byte)0xE8, (byte)0xB9, (byte)0xE0, (byte)0x06, (byte)0x12, (byte)0x1A, (byte)0x6A, (byte)0x5F, (byte)0x16, (byte)0x7D, (byte)0x59, (byte)0x5A, (byte)0x4D, (byte)0xA2, (byte)0x7F, (byte)0x4D,
             (byte)0x93, (byte)0x85, (byte)0x93, (byte)0xEA, (byte)0x79, (byte)0x2D, (byte)0x8C, (byte)0xAB, (byte)0xA1, (byte)0x6C, (byte)0xE4, (byte)0x04, (byte)0x80, (byte)0xB1, (byte)0x87, (byte)0x40,
             (byte)0xCB, (byte)0x07, (byte)0x93, (byte)0xED, (byte)0xAA, (byte)0x8C, (byte)0x82, (byte)0xDD, (byte)0xA6, (byte)0x4A, (byte)0xFB, (byte)0x1F, (byte)0x95, (byte)0x6A, (byte)0x2C, (byte)0xEF,
             (byte)0x61, (byte)0x09, (byte)0x73, (byte)0x07, (byte)0x9C, (byte)0xDA, (byte)0x03, (byte)0xE0, (byte)0x49, (byte)0x94, (byte)0x19, (byte)0x4C, (byte)0xE1, (byte)0xCF, (byte)0x99, (byte)0xE9,
             (byte)0x42, (byte)0xDE, (byte)0xCD, (byte)0x4F, (byte)0xEC, (byte)0x79, (byte)0xD8, (byte)0xF6, (byte)0xE2, (byte)0xE2, (byte)0xEB, (byte)0x2A, (byte)0x6C, (byte)0x66, (byte)0x2E, (byte)0xBE,
             (byte)0x44, (byte)0x5C, (byte)0xC7, (byte)0x97, (byte)0xE2, (byte)0x3A, (byte)0xC9, (byte)0xB5, (byte)0x52, (byte)0xA8, (byte)0xEF, (byte)0x3F}
        };

        private Report _report = null;
        private int _nextPacket = 0;

        /**
         * Constructor.
         * @param report The report of the application
         */
        public InputHandler(Report report) {
            _report = report;
        }

        /**
         * This event handler is called each time the memory plugin needs input packets.
         * @param context An instance of PluginEventContext containing the details of the event.
         * @param data A byte array containing the data of the event.
         */
        @Override
        public boolean handlePluginEvent(PluginEventContext context, byte[] data) {
            if (_nextPacket < PACKETS.length && context.maxDataSize() >= TS.PKT_SIZE) {
                _report.info(String.format("returning input packet #%d", _nextPacket));
                context.setOutputData(PACKETS[_nextPacket++]);
            }
            else {
                _report.info("end of input");
            }
            return true;
        }
    }

    /**
     * An event handler for memory output plugin.
     * It is invoked by the "memory" output plugin each time TS packets are sent.
     */
    private static class OutputHandler extends AbstractPluginEventHandler {

        private Report _report = null;

        /**
         * Constructor.
         * @param report The report of the application
         */
        public OutputHandler(Report report) {
            _report = report;
        }

        /**
         * This event handler is called each time the memory plugin sends output packets.
         * @param context An instance of PluginEventContext containing the details of the event.
         * @param data A byte array containing the data of the event.
         */
        @Override
        public boolean handlePluginEvent(PluginEventContext context, byte[] data) {
            int packets_count = data.length / TS.PKT_SIZE;
            _report.info(String.format("received %d output packets", packets_count));
            for (int i = 0; i < packets_count; i++) {
                byte[] packet = Arrays.copyOfRange(data, i * TS.PKT_SIZE, (i + 1) * TS.PKT_SIZE);
                _report.info(String.format("packet #%d: %s", i, SampleUtils.bytesToHex(packet)));
            }
            return true;
        }
    }

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        // Create a thread-safe asynchronous report.
        AsyncReport report = new AsyncReport();

        // Create our event handlers for the memory plugins.
        InputHandler input = new InputHandler(report);
        OutputHandler output = new OutputHandler(report);

        // Create a transport stream processor and register our event handlers.
        TSProcessor tsp = new TSProcessor(report);
        tsp.registerInputEventHandler(input);
        tsp.registerOutputEventHandler(output);

        // Build tsp options.
        tsp.input = new String[] {"memory"};
        tsp.plugins = new String[][] {
            {"count"},
        };
        tsp.output = new String[] {"memory"};

        // Run the TS processing and wait until completion.
        tsp.start();
        tsp.waitForTermination();
        tsp.delete();

        // Delete the event handlers.
        input.delete();
        output.delete();

        // Terminate the asynchronous report.
        report.terminate();
        report.delete();
    }
}
