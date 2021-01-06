import io.tsduck.Info;

public class Test {
	public static void main(String[] args) {
		System.out.printf("TSDuck integer version: %d\n", Info.intVersion());
		System.out.printf("TSDuck version: \"%s\"\n", Info.version());
	}
}
