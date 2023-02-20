#include "Color.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkColor.h>
#include "restore_internal_warnings.h"

float OsmAnd::FColorARGB::getRGBDelta(const FColorARGB& other) const
{
    return FColorRGB(*this).getRGBDelta(FColorRGB(other));
}

float OsmAnd::FColorRGBA::getRGBDelta(const FColorRGBA& other) const
{
    return FColorRGB(*this).getRGBDelta(FColorRGB(other));
}

float OsmAnd::ColorARGB::getRGBDelta(const ColorARGB& other) const
{
    return ColorRGB(*this).getRGBDelta(ColorRGB(other));
}

float OsmAnd::ColorRGBA::getRGBDelta(const ColorRGBA& other) const
{
    return ColorRGB(*this).getRGBDelta(ColorRGB(other));
}

OsmAnd::ColorHSV OsmAnd::ColorRGB::toHSV() const
{
    ColorHSV hsv;
    SkRGBToHSV(r, g, b, hsv.value);
    return hsv;
}
