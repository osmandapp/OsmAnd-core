package net.osmand.plus;

import net.osmand.Location;

public interface MapScreen {
	
	// map.getMapView().refreshMap();
	
	public void refreshMap();
	
	Location getLastKnownLocation();

}
