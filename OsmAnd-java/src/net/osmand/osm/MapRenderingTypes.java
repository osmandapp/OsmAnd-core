package net.osmand.osm;

import gnu.trove.list.array.TIntArrayList;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.TreeMap;


import net.osmand.PlatformUtil;
import net.osmand.data.AmenityType;
import net.osmand.osm.OSMSettings.OSMTagKey;

import org.apache.commons.logging.Log;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;


/**
 * SOURCE : http://wiki.openstreetmap.org/wiki/Map_Features
 * 
 * Describing types of polygons :
 * 1. Last 2 bits define type of element : polygon, polyline, point 
 */
public class MapRenderingTypes {

	private static final Log log = PlatformUtil.getLog(MapRenderingTypes.class);
	
	
	private static char TAG_DELIMETER = '/'; //$NON-NLS-1$
	
	private String resourceName = null;

	// stored information to convert from osm tags to int type
	private Map<String, MapRulType> types = null;
	private List<MapRulType> typeList = new ArrayList<MapRenderingTypes.MapRulType>();
	private List<MapRouteTag> routeTags = new ArrayList<MapRenderingTypes.MapRouteTag>();
	
	private Map<AmenityType, Map<String, String>> amenityTypeNameToTagVal = null;
	private Map<String, AmenityType> amenityNameToType = null;
	

	private MapRulType nameRuleType;
	private MapRulType coastlineRuleType;
	
	public MapRenderingTypes(String fileName){
		this.resourceName = fileName;
	}
	
	
	private static MapRenderingTypes DEFAULT_INSTANCE = null;
	
	public static MapRenderingTypes getDefault() {
		if(DEFAULT_INSTANCE == null){
			DEFAULT_INSTANCE = new MapRenderingTypes(null);
		}
		return DEFAULT_INSTANCE;
	}

	public Map<String, MapRulType> getEncodingRuleTypes(){
		checkIfInitNeeded();
		return types;
	}

	private void checkIfInitNeeded() {
		if (types == null) {
			types = new LinkedHashMap<String, MapRulType>();
			typeList.clear();
			nameRuleType = new MapRulType();
			nameRuleType.tag = "name";
			nameRuleType.onlyNameRef = true;
			nameRuleType.additional = false; 
			registerRuleType("name", null, nameRuleType);
			init();
		}
	}
	
	public MapRulType getTypeByInternalId(int id) {
		return typeList.get(id);
	}
	
	private MapRulType registerRuleType(String tag, String val, MapRulType rt){
		String keyVal = constructRuleKey(tag, val);
		if("natural".equals(tag) && "coastline".equals(val)) {
			coastlineRuleType = rt;
		}
		if(types.containsKey(keyVal)){
			if(types.get(keyVal).onlyNameRef ) {
				rt.id = types.get(keyVal).id;
				types.put(keyVal, rt);
				typeList.set(rt.id, rt);
				return rt;
			} else {
				throw new RuntimeException("Duplicate " + keyVal);
			}
		} else {
			rt.id = types.size();
			types.put(keyVal, rt);
			typeList.add(rt);
			return rt;
		}
	}
	
	public MapRulType getNameRuleType() {
		getEncodingRuleTypes();
		return nameRuleType;
	}
	
	public MapRulType getCoastlineRuleType() {
		getEncodingRuleTypes();
		return coastlineRuleType;
	}
	
	private MapRulType getRelationalTagValue(String tag, String val) {
		MapRulType rType = getRuleType(tag, val);
		if(rType != null && rType.relation) {
			return rType;
		}
		return null;
	}
	
	
	public Map<MapRulType, String> getRelationPropogatedTags(Relation relation) {
		Map<MapRulType, String> propogated = new LinkedHashMap<MapRulType, String>();
		Map<String, String> ts = relation.getTags();
		Iterator<Entry<String, String>> its = ts.entrySet().iterator();
		while(its.hasNext()) {
			Entry<String, String> ev = its.next();
			MapRulType rule = getRelationalTagValue(ev.getKey(), ev.getValue());
			if(rule != null) {
				String value = ev.getValue();
				if(rule.targetTagValue != null) {
					rule = rule.targetTagValue;
					if(rule.getValue() != null) {
						value = rule.getValue();
					}
				}
				if (rule.names != null) {
					for (int i = 0; i < rule.names.length; i++) {
						String tag = rule.names[i].tag.substring(rule.namePrefix.length());
						if(ts.containsKey(tag)) {
							propogated.put(rule.names[i], ts.get(tag));
						}
					}
				}
				propogated.put(rule, value);
			}
			addParsedSpecialTags(propogated, ev);
		}
		return propogated;
	}
	
	public void addParsedSpecialTags(Map<MapRulType,String> propogated, Entry<String,String> ev) {
		if ("osmc:symbol".equals(ev.getKey())) {
			String[] tokens = ev.getValue().split(":", 6);
			if (tokens.length > 0) {
				String symbol_name = "osmc_symbol_" + tokens[0];
				MapRulType rt = getRuleType(symbol_name, "");
				if(rt != null) {
					propogated.put(rt, "");
					if (tokens.length > 2 && rt.names != null) {
						String symbol = "osmc_symbol_" + tokens[1] + "_" + tokens[2];
						String name = "\u00A0";
						if (tokens.length > 3 && tokens[3].trim().length() > 0) {
							name = tokens[3];
						}
						for(int k = 0; k < rt.names.length; k++) {
							if(rt.names[k].tag.equals(symbol)) {
								propogated.put(rt.names[k], name);
							}
						}
					}
				}
			}
		}
	}
	
	// if type equals 0 no need to save that point
	public boolean encodeEntityWithType(Entity e, int zoom, TIntArrayList outTypes, 
			TIntArrayList outaddTypes, Map<MapRulType, String> namesToEncode, List<MapRulType> tempList) {
		outTypes.clear();
		outaddTypes.clear();
		namesToEncode.clear();
		tempList.clear();
		tempList.add(nameRuleType);

		boolean area = "yes".equals(e.getTag("area")) || "true".equals(e.getTag("area"));

		Collection<String> tagKeySet = e.getTagKeySet();
		for (String tag : tagKeySet) {
			String val = e.getTag(tag);
			MapRulType rType = getRuleType(tag, val);
			if (rType != null) {
				if (rType.minzoom > zoom) {
					continue;
				}
				if(rType.targetTagValue != null) {
					rType = rType.targetTagValue;
				}
				rType.updateFreq();
				if (rType.names != null) {
					for (int i = 0; i < rType.names.length; i++) {
						tempList.add(rType.names[i]);
					}
				}

				if (!rType.onlyNameRef) {
					if (rType.additional) {
						outaddTypes.add(rType.id);
					} else {
						outTypes.add(rType.id);
					}
				}
			}
		}
		for(MapRulType mt : tempList){
			String val = e.getTag(mt.tag);
			if(val != null && val.length() > 0){
				namesToEncode.put(mt, val);
			}
		}
		return area;
	}

	private MapRulType getRuleType(String tag, String val) {
		Map<String, MapRulType> types = getEncodingRuleTypes();
		MapRulType rType = types.get(constructRuleKey(tag, val));
		if (rType == null) {
			rType = types.get(constructRuleKey(tag, null));
		}
		return rType;
	}
	
	
	public Map<AmenityType, Map<String, String>> getAmenityTypeNameToTagVal() {
		if (amenityTypeNameToTagVal == null) {
			Map<String, MapRulType> types = getEncodingRuleTypes();
			amenityTypeNameToTagVal = new LinkedHashMap<AmenityType, Map<String, String>>();
			for(MapRulType type : types.values()){
				if(type.poiCategory != null && type.targetTagValue == null) {
					if(!amenityTypeNameToTagVal.containsKey(type.poiCategory)) {
						amenityTypeNameToTagVal.put(type.poiCategory, new TreeMap<String, String>());
					}
					String name = type.value;
					if (name != null) {
						if (type.poiPrefix != null) {
							name = type.poiPrefix + name;
							amenityTypeNameToTagVal.get(type.poiCategory).put(name, type.tag + " " + type.value);
						} else {
							amenityTypeNameToTagVal.get(type.poiCategory).put(name, type.tag);
						}
					}
				}
			}
		}
		return amenityTypeNameToTagVal;
	}
	
	public Map<String, AmenityType> getAmenityNameToType(){
		if(amenityNameToType == null){
			amenityNameToType = new LinkedHashMap<String, AmenityType>();
			Map<AmenityType, Map<String, String>> map = getAmenityTypeNameToTagVal();
			Iterator<Entry<AmenityType, Map<String, String>>> iter = map.entrySet().iterator();
			while(iter.hasNext()){
				Entry<AmenityType, Map<String, String>> e = iter.next();
				for(String t : e.getValue().keySet()){
					if (t != null) {
						if (amenityNameToType.containsKey(t)) {
							System.err.println("Conflict " + t + " " + amenityNameToType.get(t) + " <> " + e.getKey());
						}
						amenityNameToType.put(t, e.getKey());
					}
				}
			}
		}
		return amenityNameToType; 
	}
	
	private void init(){
		InputStream is;
		try {
			if(resourceName == null){
				is = MapRenderingTypes.class.getResourceAsStream("rendering_types.xml"); //$NON-NLS-1$
			} else {
				is = new FileInputStream(resourceName);
			}
			long time = System.currentTimeMillis();
			XmlPullParser parser = PlatformUtil.newXMLPullParser();
			int tok;
			parser.setInput(is, "UTF-8");
			String poiParentCategory = null;
			String poiParentPrefix  = null;
			while ((tok = parser.next()) != XmlPullParser.END_DOCUMENT) {
				if (tok == XmlPullParser.START_TAG) {
					String name = parser.getName();
					if (name.equals("category")) { //$NON-NLS-1$
						poiParentCategory = parser.getAttributeValue("","poi_category");
						poiParentPrefix = parser.getAttributeValue("","poi_prefix");
						String tag = parser.getAttributeValue("","poi_tag");
						if (tag != null) {
							MapRulType rtype = new MapRulType();
							rtype.poiCategory = AmenityType.valueOf(poiParentCategory.toUpperCase());
							rtype.poiSpecified = true;
							rtype.poiPrefix = poiParentPrefix;
							rtype.tag = tag;
							registerRuleType(tag, null, rtype);
						}
					} else if (name.equals("type")) {
						parseTypeFromXML(parser, poiParentCategory, poiParentPrefix);
					} else if (name.equals("routing_type")) {
						parseRouteTagFromXML(parser);
					}
				}
			}
			
			log.info("Time to init " + (System.currentTimeMillis() - time)); //$NON-NLS-1$
			is.close();
		} catch (IOException e) {
			log.error("Unexpected error", e); //$NON-NLS-1$
			e.printStackTrace();
			throw new RuntimeException(e);
		} catch (RuntimeException e) {
			log.error("Unexpected error", e); //$NON-NLS-1$
			e.printStackTrace();
			throw e;
		} catch (XmlPullParserException e) {
			log.error("Unexpected error", e); //$NON-NLS-1$
			e.printStackTrace();
			throw new RuntimeException(e);
		}
	}

	private void parseRouteTagFromXML(XmlPullParser parser) {
		MapRouteTag rtype = new MapRouteTag();
		String mode = parser.getAttributeValue("", "mode"); //$NON-NLS-1$
		rtype.tag = parser.getAttributeValue("", "tag"); //$NON-NLS-1$
		rtype.value = parser.getAttributeValue("", "value"); //$NON-NLS-1$
		rtype.base = Boolean.parseBoolean(parser.getAttributeValue("", "base"));
		rtype.register = "register".equalsIgnoreCase(mode);
		rtype.amend = "amend".equalsIgnoreCase(mode);
		rtype.text = "text".equalsIgnoreCase(mode);
		rtype.relation = "relation".equalsIgnoreCase(parser.getAttributeValue("", "relation"));
		routeTags.add(rtype);
	}
	
	public List<MapRouteTag> getRouteTags() {
		checkIfInitNeeded();
		return routeTags;
	}

	private void parseTypeFromXML(XmlPullParser parser, String poiParentCategory, String poiParentPrefix) {
		MapRulType rtype = new MapRulType();
		String val = parser.getAttributeValue("", "minzoom"); //$NON-NLS-1$
		rtype.minzoom = 15;
		if (val != null) {
			rtype.minzoom = Integer.parseInt(val);
		}
		rtype.tag = parser.getAttributeValue("", "tag"); //$NON-NLS-1$
		rtype.value = parser.getAttributeValue("", "value"); //$NON-NLS-1$
		if (rtype.value != null && rtype.value.length() == 0) { //$NON-NLS-1$
			rtype.value = null;
		}
		registerRuleType(rtype.tag, rtype.value, rtype);
		rtype.additional = Boolean.parseBoolean(parser.getAttributeValue("", "additional")); //$NON-NLS-1$
		rtype.relation = Boolean.parseBoolean(parser.getAttributeValue("", "relation")); //$NON-NLS-1$
		rtype.namePrefix = parser.getAttributeValue("", "namePrefix"); //$NON-NLS-1$
		rtype.nameCombinator = parser.getAttributeValue("", "nameCombinator"); //$NON-NLS-1$
		if(rtype.namePrefix == null){
			rtype.namePrefix = "";
		}
		
		String v = parser.getAttributeValue("", "nameTags");
		if (v != null) {
			String[] names = v.split(",");
			if (names.length == 0) {
				names = new String[] { "name" };
			}
			rtype.names = new MapRulType[names.length];
			for (int i = 0; i < names.length; i++) {
				String tagName = names[i];
				if(rtype.namePrefix.length() > 0) {
					tagName = rtype.namePrefix + tagName;
				}
				MapRulType mt = types.get(constructRuleKey(tagName, null));
				if (mt == null) {
					mt = new MapRulType();
					mt.tag = tagName;
					mt.onlyNameRef = true;
					mt.additional = false;
					registerRuleType(tagName, null, mt);
				}
				rtype.names[i] = mt;
			}
		}
		String targetTag = parser.getAttributeValue("", "target_tag");
		String targetValue = parser.getAttributeValue("", "target_value");
		if (targetTag != null || targetValue != null) {
			if (targetTag == null) {
				targetTag = rtype.tag;
			}
			if (targetValue == null) {
				targetValue = rtype.value;
			}
			rtype.targetTagValue = types.get(constructRuleKey(targetTag, targetValue));
			if (rtype.targetTagValue == null) {
				throw new RuntimeException("Illegal target tag/value " + targetTag + " " + targetValue);
			}
		}
		if (poiParentCategory != null) {
			rtype.poiCategory = AmenityType.valueOf(poiParentCategory.toUpperCase());
			rtype.poiSpecified = true;
		}
		if (poiParentPrefix != null) {
			rtype.poiPrefix = poiParentPrefix;
		}

		String poiCategory = parser.getAttributeValue("", "poi_category");
		if (poiCategory != null) {
			rtype.poiSpecified = true;
			if (poiCategory.length() == 0) {
				rtype.poiCategory = null;
			} else {
				rtype.poiCategory = AmenityType.valueOf(poiCategory.toUpperCase());
			}
		}
		String poiPrefix = parser.getAttributeValue("", "poi_prefix");
		if (poiPrefix != null) {
			rtype.poiPrefix = poiPrefix;
		}
	}
	
	public static String constructRuleKey(String tag, String val) {
		if(val == null || val.length() == 0){
			return tag;
		}
		return tag + TAG_DELIMETER + val;
	}
	
	protected static String getTagKey(String tagValue) {
		int i = tagValue.indexOf(TAG_DELIMETER);
		if(i >= 0){
			return tagValue.substring(0, i);
		}
		return tagValue;
	}
	
	protected static String getValueKey(String tagValue) {
		int i = tagValue.indexOf(TAG_DELIMETER);
		if(i >= 0){
			return tagValue.substring(i + 1);
		}
		return null;
	}
	
	public String getAmenitySubtypePrefix(String tag, String val){
		Map<String, MapRulType> rules = getEncodingRuleTypes();
		MapRulType rt = rules.get(constructRuleKey(tag, val));
		if(rt != null && rt.poiPrefix != null) {
			return rt.poiPrefix;
		}
		rt = rules.get(constructRuleKey(tag, null));
		if(rt != null && rt.poiPrefix != null) {
			return rt.poiPrefix;
		}
		return null;
	}
	
	public String getAmenitySubtype(String tag, String val){
		String prefix = getAmenitySubtypePrefix(tag, val);
		if(prefix != null){
			return prefix + val;
		}
		return val;
	}
	
	public AmenityType getAmenityType(String tag, String val){
		// register amenity types
		Map<String, MapRulType> rules = getEncodingRuleTypes();
		MapRulType rt = rules.get(constructRuleKey(tag, val));
		if(rt != null && rt.poiSpecified) {
			return rt.poiCategory;
		}
		rt = rules.get(constructRuleKey(tag, null));
		if(rt != null && rt.poiSpecified) {
			return rt.poiCategory;
		}
		return null;
	}
	
	public static class MapRouteTag {
		boolean relation;
		String tag;
		String value;
		boolean register;
		boolean amend;
		boolean base; 
		boolean text;
		
	}
	
	public static class MapRulType {
		MapRulType[] names;
		String tag;
		String value;
		int minzoom;
		boolean additional;
		boolean relation;
		MapRulType targetTagValue;
		boolean onlyNameRef;
		
		// inner id
		private int id;
		int freq;
		int targetId;
		
		String poiPrefix;
		String namePrefix ="";
		String nameCombinator = null;
		AmenityType poiCategory;
		boolean poiSpecified;
		
		
		public MapRulType(){
		}
		
		public String poiPrefix(){
			return poiPrefix;
		}
		
		public AmenityType getPoiCategory() {
			return poiCategory;
		}
		
		public String getTag() {
			return tag;
		}
		
		public int getTargetId() {
			return targetId;
		}
		
		public int getInternalId() {
			return id;
		}
		
		public void setTargetId(int targetId) {
			this.targetId = targetId;
		}
		
		public MapRulType getTargetTagValue() {
			return targetTagValue;
		}
		
		public String getValue() {
			return value;
		}
		
		public int getMinzoom() {
			return minzoom;
		}
		
		public boolean isAdditional() {
			return additional;
		}
		
		public boolean isOnlyNameRef() {
			return onlyNameRef;
		}
		
		public boolean isRelation() {
			return relation;
		}
		
		public int getFreq() {
			return freq;
		}
		
		public int updateFreq(){
			return ++freq;
		}
		
		@Override
		public String toString() {
			return tag + " " + value;
		}
	}
	
	// TODO Move to Routing Attributes and finalize 
	// HIGHWAY special attributes :
	// o/oneway			1 bit
	// f/free toll 		1 bit
	// r/roundabout  	2 bit (+ 1 bit direction)
	// s/max speed   	3 bit [0 - 30km/h, 1 - 50km/h, 2 - 70km/h, 3 - 90km/h, 4 - 110km/h, 5 - 130 km/h, 6 >, 7 - 0/not specified]
	// a/vehicle access 4 bit   (height, weight?) - one bit bicycle
	// p/parking      	1 bit
	// c/cycle oneway 	1 bit
	// ci/inside city   1 bit
	// ENCODING :  ci|c|p|aaaa|sss|rr|f|o - 14 bit
	
	public final static byte RESTRICTION_NO_RIGHT_TURN = 1;
	public final static byte RESTRICTION_NO_LEFT_TURN = 2;
	public final static byte RESTRICTION_NO_U_TURN = 3;
	public final static byte RESTRICTION_NO_STRAIGHT_ON = 4;
	public final static byte RESTRICTION_ONLY_RIGHT_TURN = 5;
	public final static byte RESTRICTION_ONLY_LEFT_TURN = 6;
	public final static byte RESTRICTION_ONLY_STRAIGHT_ON = 7;
	

	public static boolean isOneWayWay(int highwayAttributes){
		return (highwayAttributes & 1) > 0;
	}
	
	public static boolean isRoundabout(int highwayAttributes){
		return ((highwayAttributes >> 2) & 1) > 0;
	}
	
	// return 0 if not defined
	public static int getMaxSpeedIfDefined(int highwayAttributes){
		switch((highwayAttributes >> 4) & 7) {
		case 0:
			return 20;
		case 1:
			return 40;
		case 2:
			return 60;
		case 3:
			return 80;
		case 4:
			return 100;
		case 5:
			return 120;
		case 6:
			return 140;
		case 7:
			return 0;
		
		}
		return 0;
	}
	

	public static int getHighwayAttributes(Entity e){
		int attr = 0;
		// cycle oneway
		attr <<= 1;
		String c = e.getTag("cycleway"); //$NON-NLS-1$
		if(c != null && ("opposite_lane".equals(c) || "opposite_track".equals(c) || "opposite".equals(c))){ //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
			attr |= 1;
		}
		// parking
		attr <<= 1;
		String park = e.getTag("service"); //$NON-NLS-1$
		if("parking_aisle".equals(park)){ //$NON-NLS-1$
			attr |= 1;
		}

		// ACCESS not complete (should be redesigned)
		// vehicle access (not implement yet) 
		attr <<= 3;
		boolean hgv = "yes".equals(e.getTag("hgv")); //$NON-NLS-1$ //$NON-NLS-2$
		if(hgv){
			attr |= 3;
		} else {
			boolean goods = "yes".equals(e.getTag("goods")); //$NON-NLS-1$ //$NON-NLS-2$
			if(goods){
				attr |= 2;
			} else {
				attr |= 1;
			}
		}
		// bicycle access
		attr <<= 1;
		
		
		// speed
		// very simple algorithm doesn't consider city rules (country rules) and miles per hour
		attr <<= 3;
		String maxspeed = e.getTag("maxspeed"); //$NON-NLS-1$
		if(maxspeed != null){
			int kmh = 0;
			try {
				kmh = Integer.parseInt(maxspeed);
			} catch (NumberFormatException es) {
			}
			if(kmh <= 0){
				attr |= 7;
			} else if(kmh <= 30){
				attr |= 0;
			} else if(kmh <= 50){
				attr |= 1;
			} else if(kmh <= 70){
				attr |= 2;
			} else if(kmh <= 90){
				attr |= 3;
			} else if(kmh <= 110){
				attr |= 4;
			} else if(kmh <= 130){
				attr |= 5;
			} else {
				attr |= 6;
			}
		} else {
			attr |= 7;
		}
		
		
		// roundabout
		attr <<= 2;
		String jun = e.getTag(OSMTagKey.JUNCTION);
		if(jun != null){
			if("roundabout".equals(jun)){ //$NON-NLS-1$
				attr |= 1;
				if("clockwise".equals(e.getTag("direction"))){ //$NON-NLS-1$ //$NON-NLS-2$
					attr |= 2;
				}
			} 
		}
		
		// toll
		String toll = e.getTag(OSMTagKey.TOLL);
		attr <<= 1;
		if(toll != null){
			if("yes".equals(toll)){ //$NON-NLS-1$
				attr |= 1;
			}
		}
		
		// oneway
		String one = e.getTag(OSMTagKey.ONEWAY);
		attr <<= 1;
		if(one != null && (one.equals("yes") || one.equals("1") || one.equals("-1"))){  //$NON-NLS-1$ //$NON-NLS-2$//$NON-NLS-3$
 			attr |= 1;
 		}
		return attr;
	}

	
}

