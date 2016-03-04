#ifndef _OSMAND_RENDER_RULES_H
#define _OSMAND_RENDER_RULES_H

#include <string>
#include <vector>
#include <map>

#include "Common.h"
#include "common2.h"
#include "mapObjects.h"


/**
 * Parse the color string, and return the corresponding color-int.
 * Supported formats are:
 * #RRGGBB
 * #AARRGGBB
 */
int parseColor(string colorString) ;
string colorToString(int color);

const static int TRUE_VALUE = 1;
const static int FALSE_VALUE = 0;

// Foward declaration of classes
class RenderingRuleProperty;
class RenderingRule;
class RenderingRulesStorage;
class RenderingRulesHandler;
class RenderingRulesStorageResolver;
class RenderingRulesStorageProperties;
class RenderingRuleSearchRequest;

class RenderingRuleProperty
{
private:
	int type;
	const static int INT_TYPE = 1;
	const static int FLOAT_TYPE = 2;
	const static int STRING_TYPE = 3;
	const static int COLOR_TYPE = 4;
	const static int BOOLEAN_TYPE = 5;

public:
	bool input;
	std::string attrName;
	// order in
	int id;
	// use for custom rendering rule properties
	string name;
	string description;
	vector<string> possibleValues;

	RenderingRuleProperty(std::string& name, int type, bool input, int id = -1) :
			type(type), input(input), attrName(name), id(id) {
	}

	bool isFloat() {
		return type == FLOAT_TYPE;
	}

	bool isColor() {
		return type == COLOR_TYPE;
	}

	bool isString() {
		return type == STRING_TYPE;
	}

	bool isIntParse() {
		return type == INT_TYPE || type == STRING_TYPE || type == COLOR_TYPE || type == BOOLEAN_TYPE;
	}

	int parseIntValue(string value) {
		if (type == INT_TYPE) {
			size_t colon = value.find_first_of(':');
			if(colon != std::string::npos) {
				int res = 0;
				if(colon > 0) {
					res += atoi(value.substr(0, colon).c_str());
				}
				res += atoi(value.substr(colon + 1).c_str());
				return res;
			}
			return atoi(value.c_str());
		} else if (type == BOOLEAN_TYPE) {
			return value == "true" ? TRUE_VALUE : FALSE_VALUE;
		} else if (type == STRING_TYPE) {
			// requires dictionary to parse
			return -1;
		} else if (type == COLOR_TYPE) {
			return parseColor(value);
		} else {
			return -1;
		}
	}

	float parseFloatValue(string value) {
		if (type == FLOAT_TYPE) {
			size_t colon = value.find_first_of(':');
			if(colon != std::string::npos) {
				float res = 0;
				if(colon > 0) {
					res += atof(value.substr(0, colon).c_str());
				}
				res += atof(value.substr(colon + 1).c_str());
				return res;
			}
			return atof(value.c_str());
		} else {
			return -1;
		}
	}

	static RenderingRuleProperty* createOutputIntProperty(string name) {
		return new RenderingRuleProperty(name, INT_TYPE, false);
	}

	static RenderingRuleProperty* createOutputBooleanProperty(string name) {
		return new RenderingRuleProperty(name, BOOLEAN_TYPE, false);
	}

	static RenderingRuleProperty* createInputBooleanProperty(string name) {
		return new RenderingRuleProperty(name, BOOLEAN_TYPE, true);
	}

	static RenderingRuleProperty* createOutputFloatProperty(string name) {
		return new RenderingRuleProperty(name, FLOAT_TYPE, false);
	}

	static RenderingRuleProperty* createOutputStringProperty(string name) {
		return new RenderingRuleProperty(name, STRING_TYPE, false);
	}

	static RenderingRuleProperty* createInputIntProperty(string name) {
		return new RenderingRuleProperty(name, INT_TYPE, true);
	}

	static RenderingRuleProperty* createInputColorProperty(string name) {
		return new RenderingRuleProperty(name, COLOR_TYPE, true);
	}

	static RenderingRuleProperty* createOutputColorProperty(string name) {
		return new RenderingRuleProperty(name, COLOR_TYPE, false);
	}

	static RenderingRuleProperty* createInputStringProperty(string name) {
		return new RenderingRuleProperty(name, STRING_TYPE, true);
	}

};


class RenderingRule
{
public:
	std::vector<RenderingRuleProperty*> properties;
	std::vector<int> intProperties;
	std::vector<RenderingRule*> attrRefs;
	std::vector<float> floatProperties;
	std::vector<RenderingRule*> ifElseChildren;
	std::vector<RenderingRule*> ifChildren;
	bool isGroup;

	RenderingRule(map<string, string>& attrs, bool isGroup, RenderingRulesStorage* storage);
	void printDebugRenderingRule(string indent, RenderingRulesStorage * st);
private :
	inline int getPropertyIndex(string property) {
		for (uint i = 0; i < properties.size(); i++) {
			RenderingRuleProperty* prop = properties[i];
			if (prop->attrName == property) {
				return i;
			}
		}
		return -1;
	}

public :
	string getStringPropertyValue(string property, RenderingRulesStorage* storage);

	float getFloatPropertyValue(string property);

	string getColorPropertyValue(string property);

	int getIntPropertyValue(string property);
};




class RenderingRulesStorageResolver {

public:
	virtual RenderingRulesStorage* resolve(string name, RenderingRulesStorageResolver* ref) = 0;

	virtual ~RenderingRulesStorageResolver() {}
};


class RenderingRulesStorageProperties
{
public:
	RenderingRuleProperty* R_TEST;
	RenderingRuleProperty* R_DISABLE;
	RenderingRuleProperty* R_TEXT_LENGTH;
	RenderingRuleProperty* R_REF;
	RenderingRuleProperty* R_TEXT_SHIELD;
	RenderingRuleProperty* R_SHIELD;
	RenderingRuleProperty* R_SHADOW_RADIUS;
	RenderingRuleProperty* R_SHADOW_COLOR;
	RenderingRuleProperty* R_SHADER;
	RenderingRuleProperty* R_ONEWAY_ARROWS_COLOR;

	RenderingRuleProperty* R_CAP_5;
	RenderingRuleProperty* R_CAP_4;
	RenderingRuleProperty* R_CAP_3;
	RenderingRuleProperty* R_CAP_2;
	RenderingRuleProperty* R_CAP;
	RenderingRuleProperty* R_CAP_0;
	RenderingRuleProperty* R_CAP__1;
	RenderingRuleProperty* R_CAP__2;

	RenderingRuleProperty* R_PATH_EFFECT_5;
	RenderingRuleProperty* R_PATH_EFFECT_4;
	RenderingRuleProperty* R_PATH_EFFECT_3;
	RenderingRuleProperty* R_PATH_EFFECT_2;
	RenderingRuleProperty* R_PATH_EFFECT;
	RenderingRuleProperty* R_PATH_EFFECT_0;
	RenderingRuleProperty* R_PATH_EFFECT__1;
	RenderingRuleProperty* R_PATH_EFFECT__2;

	RenderingRuleProperty* R_STROKE_WIDTH_5;
	RenderingRuleProperty* R_STROKE_WIDTH_4;
	RenderingRuleProperty* R_STROKE_WIDTH_3;
	RenderingRuleProperty* R_STROKE_WIDTH_2;
	RenderingRuleProperty* R_STROKE_WIDTH;
	RenderingRuleProperty* R_STROKE_WIDTH_0;
	RenderingRuleProperty* R_STROKE_WIDTH__1;
	RenderingRuleProperty* R_STROKE_WIDTH__2;

	RenderingRuleProperty* R_COLOR_5;
	RenderingRuleProperty* R_COLOR_4;
	RenderingRuleProperty* R_COLOR_3;	
	RenderingRuleProperty* R_COLOR_2;
	RenderingRuleProperty* R_COLOR;
	RenderingRuleProperty* R_COLOR_0;
	RenderingRuleProperty* R_COLOR__1;
	RenderingRuleProperty* R_COLOR__2;
	
	RenderingRuleProperty* R_TEXT_ITALIC;
	RenderingRuleProperty* R_TEXT_BOLD;
	RenderingRuleProperty* R_TEXT_ORDER;
	RenderingRuleProperty* R_ICON_ORDER;
	RenderingRuleProperty* R_TEXT_MIN_DISTANCE;
	RenderingRuleProperty* R_TEXT_ON_PATH;
	RenderingRuleProperty* R_ICON_SHIFT_PX;
	RenderingRuleProperty* R_ICON_SHIFT_PY;
	RenderingRuleProperty* R_ICON_1;
	RenderingRuleProperty* R_ICON;
	RenderingRuleProperty* R_ICON2;
	RenderingRuleProperty* R_ICON3;
	RenderingRuleProperty* R_ICON4;
	RenderingRuleProperty* R_ICON5;
	RenderingRuleProperty* R_ICON_VISIBLE_SIZE;
	RenderingRuleProperty* R_INTERSECTION_MARGIN;
	RenderingRuleProperty* R_INTERSECTION_SIZE_FACTOR;
	RenderingRuleProperty* R_LAYER;
	RenderingRuleProperty* R_ORDER;
	RenderingRuleProperty* R_TAG;
	RenderingRuleProperty* R_VALUE;
	RenderingRuleProperty* R_MINZOOM;
	RenderingRuleProperty* R_SHADOW_LEVEL;
	RenderingRuleProperty* R_MAXZOOM;
	RenderingRuleProperty* R_NIGHT_MODE;
	RenderingRuleProperty* R_TEXT_DY;
	RenderingRuleProperty* R_TEXT_SIZE;
	RenderingRuleProperty* R_TEXT_COLOR;
	RenderingRuleProperty* R_TEXT_HALO_RADIUS;
	RenderingRuleProperty* R_TEXT_HALO_COLOR;
	RenderingRuleProperty* R_TEXT_WRAP_WIDTH;
	RenderingRuleProperty* R_ADDITIONAL;
	RenderingRuleProperty* R_OBJECT_TYPE;
	RenderingRuleProperty* R_POINT;
	RenderingRuleProperty* R_AREA;
	RenderingRuleProperty* R_CYCLE;
	RenderingRuleProperty* R_NAME_TAG;
	RenderingRuleProperty* R_NAME_TAG2;
	RenderingRuleProperty* R_ATTR_INT_VALUE;
	RenderingRuleProperty* R_ATTR_COLOR_VALUE;
	RenderingRuleProperty* R_ATTR_BOOL_VALUE;
	RenderingRuleProperty* R_ATTR_STRING_VALUE;

	UNORDERED(map)<string, RenderingRuleProperty*> properties;
	vector<RenderingRuleProperty*> rules;
	vector<RenderingRuleProperty*> customRules;

	inline RenderingRuleProperty* getProperty(const char* st) {
		UNORDERED(map)<std::string, RenderingRuleProperty*>::iterator i = properties.find(st);
		if (i == properties.end()) {
			return NULL;
		}
		return (*i).second;
	}

	inline RenderingRuleProperty* getProperty(std::string& st) {
		UNORDERED(map)<std::string, RenderingRuleProperty*>::iterator i = properties.find(st);
		if (i == properties.end()) {
			return NULL;
		}
		return (*i).second;
	}

	RenderingRuleProperty* registerRule(RenderingRuleProperty* p) {
		RenderingRuleProperty* pr = getProperty(p->attrName);
		if (pr != NULL) {
			return pr;
		}
		RenderingRuleProperty* ps = registerRuleInternal(p);
		customRules.push_back(ps);
		return ps;
	}

	void merge(RenderingRulesStorageProperties& props) {
		vector<RenderingRuleProperty*>::iterator t = props.customRules.begin();
		for (; t != props.customRules.end(); t++) {
			customRules.push_back(*t);
			properties[(*t)->attrName] = *t;
		}
	}

	RenderingRulesStorageProperties(bool initDefault) {
		if (initDefault) {
			createDefaultProperties();
		}
	}

	~RenderingRulesStorageProperties() {
		vector<RenderingRuleProperty*>::iterator it = rules.begin();
		for (; it != rules.end(); it++) {
			delete *it;
		}
	}

	RenderingRuleProperty* registerRuleInternal(RenderingRuleProperty* p) {
		if (getProperty(p->attrName) == NULL) {
			properties[p->attrName] = p;
			p->id = rules.size();
			rules.push_back(p);
		}
		return getProperty(p->attrName);
	}

	void createDefaultProperties() {
		R_TEST = registerRuleInternal(RenderingRuleProperty::createInputBooleanProperty("test"));
		R_DISABLE = registerRuleInternal(RenderingRuleProperty::createOutputBooleanProperty("disable"));
		R_TAG = registerRuleInternal(RenderingRuleProperty::createInputStringProperty("tag"));
		R_VALUE = registerRuleInternal(RenderingRuleProperty::createInputStringProperty("value"));
		R_ADDITIONAL = registerRuleInternal(RenderingRuleProperty::createInputStringProperty("additional"));
		R_MINZOOM = registerRuleInternal(RenderingRuleProperty::createInputIntProperty("minzoom"));
		R_MAXZOOM = registerRuleInternal(RenderingRuleProperty::createInputIntProperty("maxzoom"));
		R_NIGHT_MODE = registerRuleInternal(RenderingRuleProperty::createInputBooleanProperty("nightMode"));
		R_LAYER = registerRuleInternal(RenderingRuleProperty::createInputIntProperty("layer"));
		R_POINT = registerRuleInternal(RenderingRuleProperty::createInputBooleanProperty("point"));
		R_AREA = registerRuleInternal(RenderingRuleProperty::createInputBooleanProperty("area"));
		R_CYCLE = registerRuleInternal(RenderingRuleProperty::createInputBooleanProperty("cycle"));
		R_INTERSECTION_MARGIN = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("intersectionMargin"));
		R_INTERSECTION_SIZE_FACTOR = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("intersectionSizeFactor"));

		R_TEXT_LENGTH = registerRuleInternal(RenderingRuleProperty::createInputIntProperty("textLength"));
		R_NAME_TAG = registerRuleInternal(RenderingRuleProperty::createInputStringProperty("nameTag"));
		R_NAME_TAG2 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("nameTag2"));

		R_ATTR_INT_VALUE = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("attrIntValue"));
		R_ATTR_BOOL_VALUE = registerRuleInternal(RenderingRuleProperty::createOutputBooleanProperty("attrBoolValue"));
		R_ATTR_COLOR_VALUE = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("attrColorValue"));
		R_ATTR_STRING_VALUE = registerRuleInternal(
				RenderingRuleProperty::createOutputStringProperty("attrStringValue"));

		// order - no sense to make it float
		R_ORDER = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("order"));
		R_OBJECT_TYPE = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("objectType"));
		R_SHADOW_LEVEL = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("shadowLevel"));

		// text properties
		R_TEXT_WRAP_WIDTH = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("textWrapWidth"));
		R_TEXT_DY = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("textDy"));
		R_TEXT_HALO_RADIUS = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("textHaloRadius"));
		R_TEXT_HALO_COLOR = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("textHaloColor"));
		R_TEXT_SIZE = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("textSize"));
		R_TEXT_ORDER = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("textOrder"));
		R_TEXT_MIN_DISTANCE = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("textMinDistance"));
		R_TEXT_SHIELD = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("textShield"));

		R_TEXT_COLOR = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("textColor"));
		R_TEXT_BOLD = registerRuleInternal(RenderingRuleProperty::createOutputBooleanProperty("textBold"));
		R_TEXT_ITALIC = registerRuleInternal(RenderingRuleProperty::createOutputBooleanProperty("textItalic"));
		R_TEXT_ON_PATH = registerRuleInternal(RenderingRuleProperty::createOutputBooleanProperty("textOnPath"));

		// point
		R_ICON_SHIFT_PX = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("icon_shift_px"));
		R_ICON_SHIFT_PY = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("icon_shift_py"));
		R_ICON_1 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("icon__1"));
		R_ICON = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("icon"));
		R_ICON2 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("icon_2"));
		R_ICON3 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("icon_3"));
		R_ICON4 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("icon_4"));
		R_ICON5 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("icon_5"));
		R_ICON_ORDER = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("iconOrder"));
		R_SHIELD = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("shield"));
		R_ICON_VISIBLE_SIZE = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("iconVisibleSize"));

		// polygon/way
		R_COLOR = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("color"));
		R_COLOR_5 = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("color_5"));
		R_COLOR_4 = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("color_4"));
		R_COLOR_3 = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("color_3"));
		R_COLOR_2 = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("color_2"));		
		R_COLOR_0 = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("color_0"));
		R_COLOR__1 = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("color__1"));
		R_COLOR__2 = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("color__2"));

		R_STROKE_WIDTH = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("strokeWidth"));
		R_STROKE_WIDTH_2 = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("strokeWidth_2"));
		R_STROKE_WIDTH_3 = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("strokeWidth_3"));
		R_STROKE_WIDTH_4 = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("strokeWidth_4"));
		R_STROKE_WIDTH_5 = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("strokeWidth_5"));
		R_STROKE_WIDTH_0 = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("strokeWidth_0"));
		R_STROKE_WIDTH__1 = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("strokeWidth__1"));
		R_STROKE_WIDTH__2 = registerRuleInternal(RenderingRuleProperty::createOutputFloatProperty("strokeWidth__2"));

		R_PATH_EFFECT = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("pathEffect"));
		R_PATH_EFFECT_2 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("pathEffect_2"));
		R_PATH_EFFECT_4 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("pathEffect_4"));
		R_PATH_EFFECT_5 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("pathEffect_5"));
		R_PATH_EFFECT_3 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("pathEffect_3"));
		R_PATH_EFFECT_0 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("pathEffect_0"));
		R_PATH_EFFECT__1 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("pathEffect__1"));
		R_PATH_EFFECT__2 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("pathEffect__2"));

		R_CAP = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("cap"));
		R_CAP_2 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("cap_2"));
		R_CAP_4 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("cap_4"));
		R_CAP_5 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("cap_5"));
		R_CAP_3 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("cap_3"));
		R_CAP_0 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("cap_0"));
		R_CAP__1 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("cap__1"));
		R_CAP__2 = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("cap__2"));

		R_SHADER = registerRuleInternal(RenderingRuleProperty::createOutputStringProperty("shader"));
		R_SHADOW_COLOR = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("shadowColor"));
		R_SHADOW_RADIUS = registerRuleInternal(RenderingRuleProperty::createOutputIntProperty("shadowRadius"));
		R_ONEWAY_ARROWS_COLOR = registerRuleInternal(RenderingRuleProperty::createOutputColorProperty("onewayArrowsColor"));
	}

};
static string A_DEFAULT_COLOR="defaultColor";
static string A_SHADOW_RENDERING="shadowRendering";

class RenderingRulesStorage
{

private:
	friend class RenderingRulesHandler;
	UNORDERED(map)<std::string, int> dictionaryMap;
	std::vector<std::string> dictionary;
	const static int SHIFT_TAG_VAL = 16;
	// TODO make private
public:
	const static int SIZE_STATES = 7;
	UNORDERED(map)<int, RenderingRule*>* tagValueGlobalRules;
	map<std::string, RenderingRule*> renderingAttributes;
	map<std::string, std::string> renderingConstants;
	std::vector<RenderingRule*> childRules;
public:
	RenderingRulesStorageProperties PROPS;
	// No rules for multipolygon !!!
	const static int MULTI_POLYGON_TYPE = 0;

	const static int POINT_RULES = 1;
	const static int LINE_RULES = 2;
	const static int POLYGON_RULES = 3;
	const static int TEXT_RULES = 4;
	const static int ORDER_RULES = 5;
	RenderingRulesStorage(const void* storage, bool createDefProperties = true) : 
			PROPS(createDefProperties), storageId(storage) {
		tagValueGlobalRules = new UNORDERED(map)<int, RenderingRule*>[SIZE_STATES];
		if(createDefProperties) {
			getDictionaryValue("");
		}
	}

	~RenderingRulesStorage() {
		for (std::vector<RenderingRule*>::iterator rr = childRules.begin(); rr != childRules.end(); rr++) {
			delete *rr;
		}
		delete[] tagValueGlobalRules;
	}
	const void* storageId;

	RenderingRule* getRule(int state, int itag, int ivalue);

	void registerGlobalRule(RenderingRule* rr, int state, int key);

	int registerString(string d) {
		int res;
		dictionaryMap[d] = res = dictionary.size();
		dictionary.push_back(d);
		return res;
	}

	inline std::string getDictionaryValue(int i) {
		if (i < 0) {
			return std::string();
		}
		return dictionary.at(i);
	}

	inline int getDictionaryValue(std::string s) {
		UNORDERED(map)<std::string, int>::iterator it = dictionaryMap.find(s);
		if(it == dictionaryMap.end()) {
			return registerString(s);
		}
		return it->second;
	}

	void parseRulesFromXmlInputStream(const char* filename, RenderingRulesStorageResolver* resolver);

	RenderingRule* getRenderingAttributeRule(string attribute) {
		return renderingAttributes[attribute];
	}

	inline string getStringValue(int i) {
		return dictionary[i];
	}

	void printDebug(int state);

private:
	RenderingRule* createTagValueRootWrapperRule(int tagValueKey, RenderingRule* previous);


	inline string getValueString(int tagValueKey) {
		return getStringValue(tagValueKey & ((1 << SHIFT_TAG_VAL) - 1));
	}

	inline string getTagString(int tagValueKey) {
		return getStringValue(tagValueKey >> SHIFT_TAG_VAL);
	}

	inline int getTagValueKey(string tag, string value) {
		int itag = getDictionaryValue(tag);
		int ivalue = getDictionaryValue(value);
		return (itag << SHIFT_TAG_VAL) | ivalue;
	}
};


class RenderingRuleSearchRequest
{
private :
	RenderingRulesStorageProperties* PROPS;
	vector<int> values;
	vector<float> fvalues;
	vector<int> savedValues;
	vector<float> savedFvalues;
	bool searchResult;
	MapDataObject* obj;

	bool searchInternal(int state, int tagKey, int valueKey, bool loadOutput);
	bool visitRule(RenderingRule* rule, bool loadOutput);
	void loadOutputProperties(RenderingRule* rule, bool override);
	bool checkInputProperties(RenderingRule* rule);
public:
	RenderingRulesStorage* storage;

	RenderingRuleSearchRequest(RenderingRulesStorage* storage);

	~RenderingRuleSearchRequest();

	int getIntPropertyValue(RenderingRuleProperty* prop);

	int getIntPropertyValue(RenderingRuleProperty* prop, int def);

	std::string getStringPropertyValue(RenderingRuleProperty* prop);

	float getFloatPropertyValue(RenderingRuleProperty* prop);

	float getFloatPropertyValue(RenderingRuleProperty* prop, float def);

	void setStringFilter(RenderingRuleProperty* p, std::string filter);

	void setIntFilter(RenderingRuleProperty* p, int filter);

	void clearIntvalue(RenderingRuleProperty* p);

	void setBooleanFilter(RenderingRuleProperty* p, bool filter);

	RenderingRulesStorageProperties* props();

	bool searchRule(int state);

	bool search(int state, bool loadOutput);

	void clearState();

	void saveState();

	void setInitialTagValueZoom(std::string tag, std::string value, int zoom, MapDataObject* obj);

	void setTagValueZoomLayer(std::string tag, std::string val, int zoom, int layer, MapDataObject* obj);

	void externalInitialize(int* values, float* fvalues,	int* savedValues,	float* savedFvalues);

	void printDebugResult();

	void externalInitialize(vector<int>& vs, vector<float>& fvs, vector<int>& sVs, vector<float>& sFvs);

	bool isSpecified(RenderingRuleProperty* p);

	bool searchRenderingAttribute(string attribute);


};


class BasePathRenderingRulesStorageResolver : public RenderingRulesStorageResolver {
public:
	string path;
	BasePathRenderingRulesStorageResolver(string  path) : path(path) {

	}
	virtual RenderingRulesStorage* resolve(string name, RenderingRulesStorageResolver* ref) {
		string file = path;
		file += name;
		file+=".render.xml";
		RenderingRulesStorage* st = new RenderingRulesStorage(file.c_str());
		st->parseRulesFromXmlInputStream(file.c_str(), this);
		return st;
	}
	virtual ~BasePathRenderingRulesStorageResolver() {}
};

float getDensityValue(RenderingContext* rc, RenderingRuleSearchRequest* render, RenderingRuleProperty* prop, float defValue); 

float getDensityValue(RenderingContext* rc, RenderingRuleSearchRequest* render, RenderingRuleProperty* prop) ;



#endif
