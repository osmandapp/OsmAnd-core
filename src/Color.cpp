#include "Color.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkColor.h>
#include "restore_internal_warnings.h"

OsmAnd::ColorHSV OsmAnd::ColorRGB::toHSV() const
{
    ColorHSV hsv;
    SkRGBToHSV(r, g, b, hsv.value);
    return hsv;
}
