package net.osmand.plus;

import net.osmand.plus.api.ExternalServiceAPI;
import net.osmand.plus.api.InternalOsmAndAPI;
import net.osmand.plus.api.InternalToDoAPI;
import net.osmand.plus.api.SQLiteAPI;
import net.osmand.plus.api.SettingsAPI;
import net.osmand.plus.render.RendererRegistry;


/*
 * In Android version ClientContext should be cast to Android.Context for backward compatibility
 */
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
	
	public SQLiteAPI getSQLiteAPI();
	
	public void runInUIThread(Runnable run);

	public void runInUIThread(Runnable run, long delay);
}
