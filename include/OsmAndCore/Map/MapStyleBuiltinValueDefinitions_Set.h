#if !defined(DECLARE_BUILTIN_VALUEDEF)
#   define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name, isComplex)
#endif // !defined(DECLARE_BUILTIN_VALUEDEF)

DECLARE_BUILTIN_VALUEDEF(INPUT_TEST, Input, Boolean, "test", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_TAG, Input, String, "tag", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_VALUE, Input, String, "value", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_ADDITIONAL, Input, String, "additional", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_MINZOOM, Input, Integer, "minzoom", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_MAXZOOM, Input, Integer, "maxzoom", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_NIGHT_MODE, Input, Boolean, "nightMode", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_LAYER, Input, Integer, "layer", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_POINT, Input, Boolean, "point", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_AREA, Input, Boolean, "area", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_CYCLE, Input, Boolean, "cycle", false)

DECLARE_BUILTIN_VALUEDEF(INPUT_TEXT_LENGTH, Input, Integer, "textLength", false)
DECLARE_BUILTIN_VALUEDEF(INPUT_NAME_TAG, Input, String, "nameTag", false)

DECLARE_BUILTIN_VALUEDEF(OUTPUT_DISABLE, Output, Boolean, "disable", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_NAME_TAG2, Output, String, "nameTag2", false)

DECLARE_BUILTIN_VALUEDEF(OUTPUT_CLASS, Output, String, "rClass", false)

DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_BOOL_VALUE, Output, Boolean, "attrBoolValue", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_INT_VALUE, Output, Integer, "attrIntValue", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_FLOAT_VALUE, Output, Float, "attrFloatValue", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_STRING_VALUE, Output, String, "attrStringValue", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_COLOR_VALUE, Output, Color, "attrColorValue", false)

// order - no sense to make it float
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ORDER, Output, Integer, "order", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_OBJECT_TYPE, Output, Integer, "objectType", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_IGNORE_POLYGON_AREA, Output, Boolean, "ignorePolygonArea", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_IGNORE_POLYGON_AS_POINT_AREA, Output, Boolean, "ignorePolygonAsPointArea", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_LEVEL, Output, Integer, "shadowLevel", false)

// Text&Icon properties
DECLARE_BUILTIN_VALUEDEF(OUTPUT_INTERSECTS_WITH, Output, String, "intersectsWith", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_INTERSECTION_SIZE_FACTOR, Output, Float, "intersectionSizeFactor", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_INTERSECTION_SIZE, Output, Float, "intersectionSize", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_INTERSECTION_MARGIN, Output, Float, "intersectionMargin", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON, Output, String, "icon", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_2, Output, String, "icon_2", false)

// Text properties
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_WRAP_WIDTH, Output, Integer, "textWrapWidth", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_DY, Output, Integer, "textDy", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_HALO_RADIUS, Output, Integer, "textHaloRadius", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_HALO_COLOR, Output, Color, "textHaloColor", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_SIZE, Output, Integer, "textSize", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_ORDER, Output, Integer, "textOrder", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_MIN_DISTANCE, Output, Float, "textMinDistance", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_SHIELD, Output, String, "textShield", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_COLOR, Output, Color, "textColor", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_BOLD, Output, Boolean, "textBold", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_ITALIC, Output, Boolean, "textItalic", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_ON_PATH, Output, Boolean, "textOnPath", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_PLACEMENT, Output, String, "textPlacement", false)

// Icon properties
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON__3, Output, String, "icon__3", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON__2, Output, String, "icon__2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON__1, Output, String, "icon__1", false)

// Alredy declared above, under Text&Icon properties
//DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_2, Output, String, "icon_2", false)

DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_3, Output, String, "icon_3", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_4, Output, String, "icon_4", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_5, Output, String, "icon_5", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_ORDER, Output, Integer, "iconOrder", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHIELD, Output, String, "shield", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_MIN_DISTANCE, Output, Float, "iconMinDistance", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_INTERSECTION_SIZE, Output, Float, "iconVisibleSize", true) //TODO: Obsolete, superseded by "intersectionSize"
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_SHIFT_PX, Output, Float, "icon_shift_px", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_SHIFT_PY, Output, Float, "icon_shift_py", false)

// polygon/way:

// - Layer_minus2
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR__2, Output, Color, "color__2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH__2, Output, Float, "strokeWidth__2", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT__2, Output, String, "pathEffect__2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP__2, Output, String, "cap__2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_JOIN__2, Output, String, "join__2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_HMARGIN__2, Output, Float, "hmargin__2", true)

// - Layer_minus1
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR__1, Output, Color, "color__1", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH__1, Output, Float, "strokeWidth__1", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT__1, Output, String, "pathEffect__1", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP__1, Output, String, "cap__1", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_JOIN__1, Output, String, "join__1", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_HMARGIN__1, Output, Float, "hmargin__1", true)

// - Layer_0
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_0, Output, Color, "color_0", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_0, Output, Float, "strokeWidth_0", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_0, Output, String, "pathEffect_0", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_0, Output, String, "cap_0", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_JOIN_0, Output, String, "join_0", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_HMARGIN_0, Output, Float, "hmargin_0", true)

// - Layer_1
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR, Output, Color, "color", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH, Output, Float, "strokeWidth", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT, Output, String, "pathEffect", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP, Output, String, "cap", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_JOIN, Output, String, "join", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_HMARGIN, Output, Float, "hmargin", true)

// - Layer_2
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_2, Output, Color, "color_2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_2, Output, Float, "strokeWidth_2", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_2, Output, String, "pathEffect_2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_2, Output, String, "cap_2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_JOIN_2, Output, String, "join_2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_HMARGIN_2, Output, Float, "hmargin_2", true)

// - Layer_3
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_3, Output, Color, "color_3", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_3, Output, Float, "strokeWidth_3", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_3, Output, String, "pathEffect_3", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_3, Output, String, "cap_3", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_JOIN_3, Output, String, "join_3", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_HMARGIN_3, Output, Float, "hmargin_3", true)

// - Layer_4
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_4, Output, Color, "color_4", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_4, Output, Float, "strokeWidth_4", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_4, Output, String, "pathEffect_4", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_4, Output, String, "cap_4", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_JOIN_4, Output, String, "join_4", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_HMARGIN_4, Output, Float, "hmargin_4", true)

// - Layer_5
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_5, Output, Color, "color_5", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_5, Output, Float, "strokeWidth_5", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_5, Output, String, "pathEffect_5", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_5, Output, String, "cap_5", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_JOIN_5, Output, String, "join_5", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_HMARGIN_5, Output, Float, "hmargin_5", true)

DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADER, Output, String, "shader", false)

DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_COLOR, Output, Color, "shadowColor", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_RADIUS, Output, Float, "shadowRadius", true)

DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_ICON, Output, String, "pathIcon", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_ICON_STEP, Output, Float, "pathIconStep", true)
