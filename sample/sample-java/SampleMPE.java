//---------------------------------------------------------------------------
//
// TSDuck sample Java application running a chain of plugins
// and handling plugin events in a Java class.
//
//----------------------------------------------------------------------------

import io.tsduck.AbstractPluginEventHandler;
import io.tsduck.AsyncReport;
import io.tsduck.PluginEventContext;
import io.tsduck.TSProcessor;

public class SampleMPE {

    /**
     *  A 32-bit plugin event code ("MPE1" ASCII)
     */
    private static final int EVENT_CODE = 0x4D504531;

    /**
     * A pure Java class which handles TSDuck plugin events.
     */
    private static class EventHandler extends AbstractPluginEventHandler {

        /**
         * This handler is invoked when a plugin signals an event for which this object is registered.
         * @param context An instance of PluginEventContext containing the details of the event.
         * @param data A byte array containing the data of the event.
         */
        @Override
        public boolean handlePluginEvent(PluginEventContext context, byte[] data) {
            System.out.printf("==== Event code: 0x%X from plugin #%d (%s), data size: %d bytes, at TS packet %d\n",
                              context.eventCode(), context.pluginIndex(), context.pluginName(), data.length, context.pluginPackets());
            System.out.printf("MPE datagram: %s\n", SampleUtils.bytesToHex(data));
            return true;
        }
    }

    /**
     * Main program.
     * @param args Command line arguments.
     */
    public static void main(String[] args) {

        // Create a report to log multi-threaded messages.
        AsyncReport rep = new AsyncReport();

        // Create a user-defined event handler to catch plugin events.
        EventHandler handler = new EventHandler();

        // Create a TS processor using the report.
        TSProcessor tsp = new TSProcessor(rep);

        // Register our event handler in the TS processor.
        tsp.registerEventHandler(handler, EVENT_CODE);

        // Set the plugin chain.
        tsp.input = new String[] {"http", "https://github.com/tsduck/tsduck-test/raw/master/input/test-016.ts"};
        tsp.plugins = new String[][] {{"mpe", "--pid", "2001", "--max-datagram", "2", "--event-code", String.valueOf(EVENT_CODE)}};
        tsp.output = new String[] {"drop"};

        // Run the TS processing and wait until completion.
        tsp.start();
        tsp.waitForTermination();
        tsp.delete();

        // Delete the event handler.
        handler.delete();

        // Terminate the asynchronous report.
        rep.terminate();
        rep.delete();
    }
}
