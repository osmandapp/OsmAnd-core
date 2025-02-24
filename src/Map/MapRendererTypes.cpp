#include "MapRendererTypes.h"
#include "MapRendererTypes_private.h"

OsmAnd::MapLayerConfiguration::MapLayerConfiguration()
    : opacityFactor(1.0f)
{
}

OsmAnd::SymbolSubsectionConfiguration::SymbolSubsectionConfiguration()
    : opacityFactor(1.0f)
{
}

OsmAnd::ElevationConfiguration::ElevationConfiguration()
    : dataScaleFactor(1.0f)
    , slopeAlgorithm(SlopeAlgorithm::Horn)
    , visualizationStyle(VisualizationStyle::HillshadeCombined)
    , visualizationAlpha(1.0f)
    , visualizationZ(1.0f)
    , hillshadeSunAngle(45.0f)
    , hillshadeSunAzimuth(315.0f)
    , zScaleFactor(1.0f)
{
    setVisualizationColorMapPreset(ColorMapPreset::GrayscaleHillshade);
}

OsmAnd::ElevationConfiguration& OsmAnd::ElevationConfiguration::resetVisualizationColorMap()
{
    for (auto& colorMapEntry : visualizationColorMap)
    {
        colorMapEntry = { 0.0f, FColorRGBA() };
    }

    return *this;
}

OsmAnd::ElevationConfiguration& OsmAnd::ElevationConfiguration::setVisualizationColorMapPreset(
    ColorMapPreset colorMapPreset)
{
    resetVisualizationColorMap();

    switch (colorMapPreset)
    {
        case ColorMapPreset::GrayscaleHillshade:
            visualizationColorMap[0] = {   0.0f, FColorRGBA(0.0f, 0.0f, 0.0f, 0.6f) };
            visualizationColorMap[1] = { 179.0f, FColorRGBA(0.0f, 0.0f, 0.0f, 0.0f) };
            visualizationColorMap[2] = { 180.0f, FColorRGBA(1.0f, 1.0f, 1.0f, 0.0f) };
            visualizationColorMap[3] = { 255.0f, FColorRGBA(1.0f, 1.0f, 1.0f, 0.2f) };
            break;
        case ColorMapPreset::GrayscaleSlopeDegrees:
            visualizationColorMap[0] = {   0.0f, FColorRGBA(1.0f, 1.0f, 1.0f, 0.0f) };
            visualizationColorMap[1] = {  90.0f, FColorRGBA(0.0f, 0.0f, 0.0f, 1.0f) };
            break;
        case ColorMapPreset::TerrainSlopeDegrees:
            visualizationColorMap[0] = {   0.00f, FColorRGB( 74.0f / 255.0f, 165.0f / 255.0f,  61.0f / 255.0f) };
            visualizationColorMap[1] = {   7.00f, FColorRGB(117.0f / 255.0f, 190.0f / 255.0f, 100.0f / 255.0f) };
            visualizationColorMap[2] = {  15.07f, FColorRGB(167.0f / 255.0f, 220.0f / 255.0f, 145.0f / 255.0f) };
            visualizationColorMap[3] = {  35.33f, FColorRGB(245.0f / 255.0f, 211.0f / 255.0f, 163.0f / 255.0f) };
            visualizationColorMap[4] = {  43.85f, FColorRGB(229.0f / 255.0f, 149.0f / 255.0f, 111.0f / 255.0f) };
            visualizationColorMap[5] = {  50.33f, FColorRGB(235.0f / 255.0f, 178.0f / 255.0f, 152.0f / 255.0f) };
            visualizationColorMap[6] = {  55.66f, FColorRGB(244.0f / 255.0f, 216.0f / 255.0f, 201.0f / 255.0f) };
            visualizationColorMap[7] = {  69.00f, FColorRGB(251.0f / 255.0f, 247.0f / 255.0f, 240.0f / 255.0f) };
            break;
    }

    return *this;
}

bool OsmAnd::ElevationConfiguration::isValid() const
{
    return (visualizationStyle == VisualizationStyle::None
            && slopeAlgorithm == SlopeAlgorithm::None)
        || ((visualizationStyle == VisualizationStyle::SlopeDegrees
                || visualizationStyle == VisualizationStyle::SlopePercents)
            && slopeAlgorithm != SlopeAlgorithm::None)
        || ((visualizationStyle == VisualizationStyle::HillshadeTraditional
                || visualizationStyle == VisualizationStyle::HillshadeIgor
                || visualizationStyle == VisualizationStyle::HillshadeCombined
                || visualizationStyle == VisualizationStyle::HillshadeMultidirectional)
            && slopeAlgorithm != SlopeAlgorithm::None);
}

OsmAnd::GridConfiguration::GridConfiguration()
    : primaryGrid(false)
    , secondaryGrid(false)
    , primaryProjection(Projection::WGS84)
    , secondaryProjection(Projection::WGS84)
    , primaryGap(180.0f)
    , secondaryGap(1.0f)
    , primaryGranularity(0.0f)
    , secondaryGranularity(1.0f)
    , primaryThickness(2.0f)
    , secondaryThickness(1.0f)
    , primaryFormat(Format::Decimal)
    , secondaryFormat(Format::Decimal)
{
    setProjectionParameters();
}

OsmAnd::GridConfiguration& OsmAnd::GridConfiguration::setProjectionParameters()
{
    setProjectionParameters(0, primaryProjection);
    setProjectionParameters(1, secondaryProjection);
    return *this;
}

OsmAnd::GridConfiguration& OsmAnd::GridConfiguration::setProjectionParameters(
    const int gridIndex, const Projection projection)
{
    auto parameters = &gridParameters[gridIndex];
    switch (projection)
    {
        case Projection::UTM:
            parameters->factorX1 = 0.0f;
            parameters->factorX2 = 1.0f;
            parameters->factorX3 = 0.0f;
            parameters->offsetX = 0.0f;
            parameters->factorY1 = 0.0f;
            parameters->factorY2 = 1.0f;
            parameters->factorY3 = 0.0f;
            parameters->offsetY = 0.0f;
            parameters->minZoom = ZoomLevel9;
            parameters->maxZoomForFloat = ZoomLevel12;
            parameters->maxZoomForMixed = ZoomLevel14;
            break;
        case Projection::Mercator:
            parameters->factorX1 = 0.0f;
            parameters->factorX2 = 0.0f;
            parameters->factorX3 = 1.0f;
            parameters->offsetX = 0.0f;
            parameters->factorY1 = 0.0f;
            parameters->factorY2 = 0.0f;
            parameters->factorY3 = 1.0f;
            parameters->offsetY = 0.0f;
            parameters->minZoom = ZoomLevel2;
            parameters->maxZoomForFloat = ZoomLevel12;
            parameters->maxZoomForMixed = ZoomLevel14;
            break;
        default: // EPSG:4326 (WGS84 Lat/Lon degrees)
            const auto radToDeg = static_cast<float>(180.0 / M_PI);
            parameters->factorX1 = radToDeg;
            parameters->factorX2 = 0.0f;
            parameters->factorX3 = 0.0f;
            parameters->offsetX = 0.0f;
            parameters->factorY1 = radToDeg;
            parameters->factorY2 = 0.0f;
            parameters->factorY3 = 0.0f;
            parameters->offsetY = 0.0f;
            parameters->minZoom = ZoomLevel2;
            parameters->maxZoomForFloat = ZoomLevel12;
            parameters->maxZoomForMixed = ZoomLevel14;
    }

    return *this;
}

double OsmAnd::GridConfiguration::getPrimaryGridReference(
    const PointI& location31) const
{
    return getLocationReference(primaryProjection, location31);
}

double OsmAnd::GridConfiguration::getSecondaryGridReference(
    const PointI& location31) const
{
    return getLocationReference(secondaryProjection, location31);
}

OsmAnd::PointD OsmAnd::GridConfiguration::getPrimaryGridLocation(
    const PointI& location31, const double* referenceDeg /* = nullptr */) const
{
    return projectLocation(primaryProjection, location31, referenceDeg);
}

OsmAnd::PointD OsmAnd::GridConfiguration::getSecondaryGridLocation(
    const PointI& location31, const double* referenceDeg /* = nullptr */) const
{
    return projectLocation(secondaryProjection, location31, referenceDeg);
}

double OsmAnd::GridConfiguration::getLocationReference(
    const Projection projection, const PointI& location31) const
{
    double result;
    switch (projection)
    {
        case Projection::UTM: // UTM zone central meridian (degrees)
            Utilities::getZoneUTM(Utilities::getAnglesFrom31(location31), &result);
            break;
        case Projection::Mercator: // EPSG:3857 prime meridian
            result = 0.0;
            break;
        default: // EPSG:4326 prime meridian
            result = 0.0;
    }
    return result;
}

OsmAnd::PointD OsmAnd::GridConfiguration::projectLocation(
    const Projection projection, const PointI& location31, const double* referenceDeg /* = nullptr */) const
{
    PointD result;
    switch (projection)
    {
        case Projection::UTM: // UTM easting and northing coordinates (100 kilometers)
            {
                const auto lonlat = Utilities::getAnglesFrom31(location31);
                double refLon;
                if (referenceDeg)
                    refLon = *referenceDeg;
                else
                    Utilities::getZoneUTM(lonlat, &refLon);
                result = Utilities::getCoordinatesUTM(lonlat, refLon * M_PI / 180.0) / 100.0;
            }
            break;
        case Projection::Mercator: // EPSG:3857 X and Y coordinates (100 map kilometers)
            result = Utilities::metersFrom31(PointD(location31)) / 100000.0;
            break;
        default: // EPSG:4326 longitude and latitude (degrees)
            {
                const auto lonlat = Utilities::getAnglesFrom31(location31);
                result = lonlat * 180.0 / M_PI;
            }
    }
    return result;
}

OsmAnd::PointI OsmAnd::GridConfiguration::getPrimaryGridLocation31(
    const PointD& location) const
{
    return unProjectLocation(primaryProjection, location);
}

OsmAnd::PointI OsmAnd::GridConfiguration::getSecondaryGridLocation31(
    const PointD& location) const
{
    return unProjectLocation(secondaryProjection, location);
}

OsmAnd::PointI OsmAnd::GridConfiguration::unProjectLocation(
    const Projection projection, const PointD& location) const
{
    PointI result(-1, -1);
    switch (projection)
    {
        case Projection::UTM: // TODO: unproject UTM to 31-coordinates
            break;
        case Projection::Mercator: // Get 31-coordinates from EPSG:3857 X and Y coordinates in 100 map kilometers
            result = Utilities::metersTo31(location * 100000.0);
            break;
        default: // Get 31-coordinates from EPSG:4326 longitude and latitude in degrees
            result = Utilities::get31FromAngles(location * M_PI / 180.0);
    }
    return result;
}

bool OsmAnd::GridConfiguration::getPrimaryGridCoordinateX(double& coordinate) const
{
    return getCoordinateX(primaryProjection, coordinate);
}

bool OsmAnd::GridConfiguration::getPrimaryGridCoordinateY(double& coordinate) const
{
    return getCoordinateY(primaryProjection, coordinate);
}

bool OsmAnd::GridConfiguration::getSecondaryGridCoordinateX(double& coordinate) const
{
    return getCoordinateX(secondaryProjection, coordinate);
}

bool OsmAnd::GridConfiguration::getSecondaryGridCoordinateY(double& coordinate) const
{
    return getCoordinateY(secondaryProjection, coordinate);
}

bool OsmAnd::GridConfiguration::getCoordinateX(const Projection projection, double& coordinate) const
{
    bool result = false;
    switch (projection)
    {
        case Projection::UTM: // UTM easting coordinate (100 kilometers)
            result = true;
            break;
        case Projection::Mercator: // EPSG:3857 coordinate X in 100 map kilometers
            {
                const auto half = M_PI * 63.78137;
                if (coordinate > half)
                    coordinate -= 2.0 * half;
                else if (coordinate < -half)
                    coordinate += 2.0 * half;
                result = true;
            }
            break;
        default: // EPSG:4326 longitude in degrees
            if (coordinate >= 180.0)
                coordinate -= 360.0;
            else if (coordinate < -180.0)
                coordinate += 360.0;
            result = true;
    }
    return result;
}

bool OsmAnd::GridConfiguration::getCoordinateY(const Projection projection, double& coordinate) const
{
    bool result = false;
    switch (projection)
    {
        case Projection::UTM: // UTM northing coordinate (100 kilometers)
            result = true;
            break;
        case Projection::Mercator: // EPSG:3857 coordinate Y in 100 map kilometers
            {
                const auto half = M_PI * 63.78137;
                if (coordinate >= -half && coordinate <= half)
                    result = true;
            }
            break;
        default: // EPSG:4326 latitude in degrees
            if (coordinate >= -85.0 && coordinate <= 85.0)
                result = true;
    }
    return result;
}

double OsmAnd::GridConfiguration::getPrimaryMaxMarksPerAxis(const double gap) const
{
    return getMaxMarksPerAxis(primaryProjection, gap);
}

double OsmAnd::GridConfiguration::getSecondaryMaxMarksPerAxis(const double gap) const
{
    return getMaxMarksPerAxis(secondaryProjection, gap);
}

double OsmAnd::GridConfiguration::getMaxMarksPerAxis(const Projection projection, const double gap) const
{
    double result;
    switch (projection)
    {
        case Projection::UTM: // No need to limit UTM marks
            result = std::numeric_limits<double>::max();
            break;
        case Projection::Mercator: // Get maximum number of EPSG:3857 marks (gap in 100 kilometers)
            {
                const auto earth = 2.0 * M_PI * 63.78137;
                result = std::floor(earth / gap);
            }
            break;
        default: // Get maximum number of EPSG:4326 marks (gap in degrees)
            result = std::floor(360.0 / gap);
    }
    return result;
}

QString OsmAnd::GridConfiguration::getPrimaryGridMarkX(const PointD& coordinates, const int zone) const
{
    return getMarkX(primaryProjection, primaryFormat, coordinates, zone);
}

QString OsmAnd::GridConfiguration::getPrimaryGridMarkY(const PointD& coordinates, const int zone) const
{
    return getMarkY(primaryProjection, primaryFormat, coordinates, zone);
}

QString OsmAnd::GridConfiguration::getSecondaryGridMarkX(const PointD& coordinates, const int zone) const
{
    return getMarkX(secondaryProjection, secondaryFormat, coordinates, zone);
}

QString OsmAnd::GridConfiguration::getSecondaryGridMarkY(const PointD& coordinates, const int zone) const
{
    return getMarkY(secondaryProjection, secondaryFormat, coordinates, zone);
}

QString OsmAnd::GridConfiguration::getMarkX(
    const Projection projection, const Format format, const PointD& coordinates, const int zone) const
{
    QString result;
    switch (projection)
    {
        case Projection::UTM: // UTM easting coordinate (zone, hemisphere, meters)
            {
                PointI zoneUTM(zone & 63, zone >> 6);
                QString hemisphere = zoneUTM.y < 13 ? QStringLiteral("S") : QStringLiteral("N");
                result =
                    QStringLiteral("%1%2 %3").arg(zoneUTM.x).arg(hemisphere).arg(coordinates.x * 100000.0, 0, 'f', 0);
            }
            break;
        case Projection::Mercator: // EPSG:3857 coordinate X in map kilometers
            result = QStringLiteral("%1").arg(coordinates.x * 100.0, 0, 'f', 3);
            Utilities::removeTrailingZeros(result);
            break;
        default: // EPSG:4326 longitude (degrees / degrees + minutes + seconds / degrees + minutes)
            if (format == Format::Decimal)
            {
                QString str = QStringLiteral("%1").arg(std::abs(coordinates.x), 0, 'f', 5);
                Utilities::removeTrailingZeros(str);
                result = str % QStringLiteral("°");
            }
            else if (format == Format::DMS)
                result = Utilities::getDegreeMinuteSecondString(coordinates.x);
            else
                result = Utilities::getDegreeMinuteString(coordinates.x);

    }
    return result;
}

QString OsmAnd::GridConfiguration::getMarkY(
    const Projection projection, const Format format, const PointD& coordinates, const int zone) const
{
    QString result;
    switch (projection)
    {
        case Projection::UTM: // UTM northing coordinate (zone, hemisphere, meters)
            {
                const PointI zoneUTM(zone & 63, zone >> 6);
                const QString hemisphere = zoneUTM.y < 13 ? QStringLiteral("S") : QStringLiteral("N");
                const auto coordinate = std::round(coordinates.y * 100000.0) - (zoneUTM.y < 13 ? 0.0 : 10000000.0);
                result = QStringLiteral("%1%2 %3").arg(zoneUTM.x).arg(hemisphere).arg(coordinate, 0, 'f', 0);
            }
            break;
        case Projection::Mercator: // EPSG:3857 coordinate Y in map kilometers
            result = QStringLiteral("%1").arg(coordinates.y * 100.0, 0, 'f', 3);
            Utilities::removeTrailingZeros(result);
            break;
        default: // EPSG:4326 latitude (degrees / degrees + minutes + seconds / degrees + minutes)
            if (format == Format::Decimal)
            {
                QString str = QStringLiteral("%1").arg(std::abs(coordinates.y), 0, 'f', 5);
                Utilities::removeTrailingZeros(str);
                result = str % QStringLiteral("°");
            }
            else if (format == Format::DMS)
                result = Utilities::getDegreeMinuteSecondString(coordinates.y);
            else
                result = Utilities::getDegreeMinuteString(coordinates.y);
}
    return result;
}

OsmAnd::PointD OsmAnd::GridConfiguration::getCurrentGaps(
    const PointI& target31, const ZoomLevel& zoomLevel, PointD* refLons_ /* = nullptr */) const
{
    PointD result(primaryGap, secondaryGap);
    PointD refLons(NAN, NAN);
    const auto shift = MaxZoomLevel - zoomLevel;
    PointI tile31(target31.x >> shift, target31.y >> shift);
    tile31.x <<= shift;
    tile31.y <<= shift;
    auto tileSize31 = 1u << shift >> 1;
    PointI startTile31(tile31.x, tile31.y + tileSize31);
    PointI centerTile31(tile31.x + tileSize31, tile31.y + tileSize31);
    if (primaryGranularity > 0.0f)
    {
        refLons.x = getPrimaryGridReference(centerTile31);
        auto tileBegin = getPrimaryGridLocation(startTile31, &refLons.x);
        auto tileCenter = getPrimaryGridLocation(centerTile31, &refLons.x);
        auto cellSize = fabs(tileCenter.x - tileBegin.x) * 2.0 * primaryGranularity;
        result.x = primaryFormat == Format::DMS ? Utilities::snapToGridDMS(cellSize)
            : (primaryFormat == Format::DM ? Utilities::snapToGridDM(cellSize)
                : Utilities::snapToGridDecimal(cellSize));
    }
    if (secondaryGranularity > 0.0f)
    {
        auto difFactor =
            primaryGranularity / secondaryGranularity;
        if (primaryGranularity == 0.0 || primaryProjection != primaryProjection
            || difFactor - std::floor(difFactor) > 0.0f)
        {
            refLons.y = getSecondaryGridReference(centerTile31);
            auto tileBegin = getSecondaryGridLocation(startTile31, &refLons.y);
            auto tileCenter = getSecondaryGridLocation(centerTile31, &refLons.y);
            auto cellSize =
                fabs(tileCenter.x - tileBegin.x) * 2.0 * secondaryGranularity;
            result.y = secondaryFormat == Format::DMS ? Utilities::snapToGridDMS(cellSize)
                : (secondaryFormat == Format::DM ? Utilities::snapToGridDM(cellSize)
                    : Utilities::snapToGridDecimal(cellSize));
            }
        else
            result.y = result.x / static_cast<double>(difFactor);
    }

    if (refLons_)
        *refLons_ = refLons;

    result.x = correctGap(primaryProjection, result.x);
    result.y = correctGap(secondaryProjection, result.y);

    return result;
}

double OsmAnd::GridConfiguration::correctGap(const Projection projection, const double gap) const
{
    double result;
    switch (projection)
    {
        case Projection::UTM: // No need to correct UTM gap
            result = gap;
            break;
        case Projection::Mercator: // Get gap for EPSG:3857 that fit (in 100 kilometers)
            {
                const auto half = M_PI * 63.78137;
                result = std::round(half / gap) < 2.0 ? half : gap;
            }
            break;
        default: // Get gap for EPSG:4326 that fit (in degrees)
            result = 180.0 / std::max(std::round(180.0 / gap), 1.0);
    }
    return result;
}

bool OsmAnd::GridConfiguration::isValid() const
{
    return (primaryProjection == Projection::WGS84
        || primaryProjection == Projection::UTM
        || primaryProjection == Projection::Mercator) &&
        (secondaryProjection == Projection::WGS84
        || secondaryProjection == Projection::UTM
        || secondaryProjection == Projection::Mercator);
}
