#include "MapStyleBuiltinValueDefinitions.h"

#include "MapStyleValueDefinition.h"

OsmAnd::MapStyleBuiltinValueDefinitions::MapStyleBuiltinValueDefinitions()
    #define DECLARE_BUILTIN_VALUEDEF(varname, valueClass, dataType, name) \
        varname(new OsmAnd::MapStyleValueDefinition( \
            OsmAnd::MapStyleValueClass::valueClass, \
            OsmAnd::MapStyleValueDataType::dataType, \
            QString::fromLatin1(name)))
    : DECLARE_BUILTIN_VALUEDEF(INPUT_TEST, Input, Boolean, "test")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_TAG, Input, String, "tag")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_VALUE, Input, String, "value")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_ADDITIONAL, Input, String, "additional")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_MINZOOM, Input, Integer, "minzoom")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_MAXZOOM, Input, Integer, "maxzoom")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_NIGHT_MODE, Input, Boolean, "nightMode")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_LAYER, Input, Integer, "layer")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_POINT, Input, Boolean, "point")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_AREA, Input, Boolean, "area")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_CYCLE, Input, Boolean, "cycle")

    , DECLARE_BUILTIN_VALUEDEF(INPUT_TEXT_LENGTH, Input, Integer, "textLength")
    , DECLARE_BUILTIN_VALUEDEF(INPUT_NAME_TAG, Input, String, "nameTag")

    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_DISABLE, Output, Boolean, "disable")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_NAME_TAG2, Output, String, "nameTag2")

    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_INT_VALUE, Output, Integer, "attrIntValue")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_BOOL_VALUE, Output, Boolean, "attrBoolValue")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_COLOR_VALUE, Output, Color, "attrColorValue")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ATTR_STRING_VALUE, Output, String, "attrStringValue")

    // order - no sense to make it float
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ORDER, Output, Integer, "order")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_OBJECT_TYPE, Output, Integer, "objectType")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_LEVEL, Output, Integer, "shadowLevel")

    // text properties
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_WRAP_WIDTH, Output, Integer, "textWrapWidth")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_DY, Output, Integer, "textDy")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_HALO_RADIUS, Output, Integer, "textHaloRadius")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_SIZE, Output, Integer, "textSize")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_ORDER, Output, Integer, "textOrder")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_MIN_DISTANCE, Output, Integer, "textMinDistance")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_SHIELD, Output, String, "textShield")

    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_COLOR, Output, Color, "textColor")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_BOLD, Output, Boolean, "textBold")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_TEXT_ON_PATH, Output, Boolean, "textOnPath")

    // point
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON, Output, String, "icon")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_ICON_ORDER, Output, Integer, "iconOrder")

    // polygon/way
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR, Output, Color, "color")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_2, Output, Color, "color_2")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_3, Output, Color, "color_3")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR_0, Output, Color, "color_0")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_COLOR__1, Output, Color, "color__1")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH, Output, Float, "strokeWidth")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_2, Output, Float, "strokeWidth_2")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_3, Output, Float, "strokeWidth_3")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH_0, Output, Float, "strokeWidth_0")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_STROKE_WIDTH__1, Output, Float, "strokeWidth__1")

    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT, Output, String, "pathEffect")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_2, Output, String, "pathEffect_2")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_3, Output, String, "pathEffect_3")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT_0, Output, String, "pathEffect_0")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_PATH_EFFECT__1, Output, String, "pathEffect__1")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP, Output, String, "cap")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_2, Output, String, "cap_2")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_3, Output, String, "cap_3")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP_0, Output, String, "cap_0")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_CAP__1, Output, String, "cap__1")

    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADER, Output, String, "shader")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_COLOR, Output, Color, "shadowColor")
    , DECLARE_BUILTIN_VALUEDEF(OUTPUT_SHADOW_RADIUS, Output, Integer, "shadowRadius")
    #undef DECLARE_BUILTIN_VALUEDEF
{
}

OsmAnd::MapStyleBuiltinValueDefinitions::~MapStyleBuiltinValueDefinitions()
{
}
