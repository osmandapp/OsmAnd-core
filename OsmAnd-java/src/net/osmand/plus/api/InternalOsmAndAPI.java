package net.osmand.plus.api;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

import net.osmand.NativeLibrary;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlSerializer;

public interface InternalOsmAndAPI {

	public XmlSerializer newSerializer();

	public XmlPullParser newPullParser();
	
	public String getPackageName();

	public InputStream openAsset(String name) throws IOException;
	
	public File getAppDir();
	
	public File getAppDir(String extend);
	
	public NativeLibrary getNativeLibrary();

	public boolean accessibilityEnabled();
	
	public boolean accessibilityExtensions();
}
