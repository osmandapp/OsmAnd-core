#include "GeoBandSettings.h"

OsmAnd::GeoBandSettings::GeoBandSettings(
    const QString& unit_,
    const QString& unitFormatGeneral_,
    const QString& unitFormatPrecise_,
    const QString& internalUnit_,
    const float opacity_,
    const QString& colorProfilePath_,
    const QString& contourStyleName_,
    const QHash<ZoomLevel, QList<double>>& contourLevels_,
    const QHash<ZoomLevel, QStringList>& contourTypes_)
    : unit(unit_)
    , unitFormatGeneral(unitFormatGeneral_)
    , unitFormatPrecise(unitFormatPrecise_)
    , internalUnit(internalUnit_)
    , opacity(opacity_)
    , colorProfilePath(colorProfilePath_)
    , contourStyleName(contourStyleName_)
    , contourLevels(contourLevels_)
    , contourTypes(contourTypes_)
{
}

OsmAnd::GeoBandSettings::~GeoBandSettings()
{
}
