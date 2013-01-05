package net.osmand.plus.api;

import java.io.File;
import java.util.List;

import net.osmand.binary.BinaryMapIndexReader;
import net.osmand.map.ITileSource;
import net.osmand.map.TileSourceManager.TileSourceTemplate;
import net.osmand.plus.OsmandSettings.DayNightMode;

public interface InternalToDoAPI {

	public void forceMapRendering();

	public BinaryMapIndexReader[] getRoutingMapFiles();
	
	public void setDayNightMode(DayNightMode val);

	public ITileSource newSqliteTileSource(File dir, List<TileSourceTemplate> knownTemplates);


}
