package net.osmand.data;


import java.util.Comparator;

import net.osmand.Collator;
import net.osmand.PlatformUtil;

public class MapObjectComparator implements Comparator<MapObject>{
	private final boolean en;
	Collator collator = PlatformUtil.primaryCollator();
	public MapObjectComparator(boolean en){
		this.en = en;
	}
	
	@Override
	public int compare(MapObject o1, MapObject o2) {
		if(en){
			return collator.compare(o1.getEnName(), o2.getEnName());
		} else {
			return collator.compare(o1.getName(), o2.getName());
		}
	}
}