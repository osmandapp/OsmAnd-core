#ifdef DECLARE_BUILTIN_VALUEDEF

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

DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_INT_VALUE, Output, Integer, "attrIntValue", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_BOOL_VALUE, Output, Boolean, "attrBoolValue", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_COLOR_VALUE, Output, Color, "attrColorValue", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_STRING_VALUE, Output, String, "attrStringValue", false)

// order - no sense to make it float
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ORDER, Output, Integer, "order", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_OBJECT_TYPE, Output, Integer, "objectType", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_LEVEL, Output, Integer, "shadowLevel", false)

// Text&Icon properties
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_OR_ICON_INTERSECTS_WITH, Output, String, "intersectsWith", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_OR_ICON_INTERSECTED_BY, Output, String, "intersectedBy", false)

// Text properties
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_WRAP_WIDTH, Output, Integer, "textWrapWidth", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_DY, Output, Integer, "textDy", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_HALO_RADIUS, Output, Integer, "textHaloRadius", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_HALO_COLOR, Output, Color, "textHaloColor", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_SIZE, Output, Integer, "textSize", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_ORDER, Output, Integer, "textOrder", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_MIN_DISTANCE_X, Output, Integer, "textMinDistance", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_SHIELD, Output, String, "textShield", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_COLOR, Output, Color, "textColor", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_BOLD, Output, Boolean, "textBold", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_ON_PATH, Output, Boolean, "textOnPath", false)

// Icon properties
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON, Output, String, "icon", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_ORDER, Output, Integer, "iconOrder", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_SHIELD, Output, String, "shield", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_INTERSECTION_SIZE, Output, Float, "iconVisibleSize", true)

// polygon/way:

// - Set_0
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR, Output, Color, "color", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH, Output, Float, "strokeWidth", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT, Output, String, "pathEffect", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP, Output, String, "cap", false)

// - Set_1
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_2, Output, Color, "color_2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_2, Output, Float, "strokeWidth_2", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_2, Output, String, "pathEffect_2", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_2, Output, String, "cap_2", false)

// - Set_minus1
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_0, Output, Color, "color_0", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_0, Output, Float, "strokeWidth_0", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_0, Output, String, "pathEffect_0", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_0, Output, String, "cap_0", false)

// - Set_minus2
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR__1, Output, Color, "color__1", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH__1, Output, Float, "strokeWidth__1", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT__1, Output, String, "pathEffect__1", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP__1, Output, String, "cap__1", false)

// - Set_3
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_3, Output, Color, "color_3", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_3, Output, Float, "strokeWidth_3", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_3, Output, String, "pathEffect_3", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_3, Output, String, "cap_3", false)

// - Set_4
DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_4, Output, Color, "color_4", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_4, Output, Float, "strokeWidth_4", true)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_4, Output, String, "pathEffect_4", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_4, Output, String, "cap_4", false)

DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADER, Output, String, "shader", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_COLOR, Output, Color, "shadowColor", false)
DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_RADIUS, Output, Integer, "shadowRadius", true)

#endif // DECLARE_BUILTIN_VALUEDEF
