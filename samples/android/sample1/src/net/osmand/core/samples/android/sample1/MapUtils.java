package net.osmand.core.samples.android.sample1;

public class MapUtils {

	public static float unifyRotationTo360(float rotate) {
		while (rotate < -180) {
			rotate += 360;
		}
		while (rotate > +180) {
			rotate -= 360;
		}
		return rotate;
	}

}
