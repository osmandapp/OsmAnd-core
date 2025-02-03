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
    : primaryProjection(Projection::UTM)
    , secondaryProjection(Projection::UTM)
    , primaryGap(1.0f)
    , secondaryGap(0.5f)
    , primaryGranularity(0.5f)
    , secondaryGranularity(0.25f)
    , primaryThickness(2.0f)
    , secondaryThickness(1.0f)
    , primaryColor({0.0f, 1.0f, 1.0f, 0.0f})
    , secondaryColor({0.0f, 1.0f, 1.0f, 1.0f})
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
    double refLonDeg;
    auto lonlat = Utilities::getAnglesFrom31(location31);
    switch (projection)
    {
        case Projection::UTM: // UTM easting and northing coordinates (100 kilometers)
            if (referenceDeg)
                refLonDeg = *referenceDeg;
            else
                Utilities::getZoneUTM(lonlat, &refLonDeg);
            result = Utilities::getCoordinatesUTM(lonlat, refLonDeg * M_PI / 180.0) / 100.0;
            break;
        case Projection::Mercator: // EPSG:3857 X and Y coordinates (100 map kilometers)
            result = Utilities::metersFrom31(PointD(location31)) / 100000.0;
            break;
        default: // EPSG:4326 longitude and latitude (degrees)
            result = lonlat * 180.0 / M_PI;
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
