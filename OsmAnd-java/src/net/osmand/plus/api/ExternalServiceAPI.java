package net.osmand.plus.api;

public interface ExternalServiceAPI {
	
	public boolean isWifiConnected();
	
	public boolean isInternetConnected();
	

	public boolean isLightSensorEnabled();
	
	public String getExternalStorageDirectory();
}
