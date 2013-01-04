package net.osmand.plus;

import net.osmand.plus.api.ExternalServiceAPI;
import net.osmand.plus.api.InternalOsmAndAPI;
import net.osmand.plus.api.InternalToDoAPI;
import net.osmand.plus.api.SettingsAPI;
import net.osmand.plus.render.RendererRegistry;


public interface ClientContext {
	
	public String getString(int resId, Object... args);
	
	public void showToastMessage(int msgId, Object... args);
	
	public void showToastMessage(String msg);
	
	public RendererRegistry getRendererRegistry();

	public OsmandSettings getSettings();
	
	public SettingsAPI getSettingsAPI();
	
	public ExternalServiceAPI getExternalServiceAPI();
	
	public InternalToDoAPI getTodoAPI();
	
	public InternalOsmAndAPI getInternalAPI();

}
