#include "MapRendererTypes.h"
#include "MapRendererTypes_private.h"

#include <QRegularExpression>
#include "ogr_spatialref.h"
#include "ogr_proj_p.h"
#include <proj.h>

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
    , primaryTopMarginFactor(8.0f)
    , secondaryTopMarginFactor(8.0f)
    , primaryBottomMarginFactor(8.0f)
    , secondaryBottomMarginFactor(8.0f)
    , primaryMinZoomLevel(ZoomLevel::ZoomLevel0)
    , secondaryMinZoomLevel(ZoomLevel::ZoomLevel0)
    , primaryMaxZoomLevel(ZoomLevel::ZoomLevel22)
    , secondaryMaxZoomLevel(ZoomLevel::ZoomLevel22)
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
    if (projection != Projection::HOMV2 && projection != Projection::OSTEREO && projection != Projection::TM)
    {
        parameters->lonBounds = PointD(-1e38, 1e38); // Without longitude bounds
        parameters->latBounds = parameters->lonBounds; // Without latitude bounds
        parameters->semiMajorAxisAndInverseFlattening.x = 6378.137; // Semi-major axis of WGS84 ellipsoid (km)
        parameters->semiMajorAxisAndInverseFlattening.y = 298.257223563; // Inverse flattening of WGS84 ellipsoid
        parameters->semiMajorAxisAndSemiMinorAxis.x = parameters->semiMajorAxisAndInverseFlattening.x;
        parameters->semiMajorAxisAndSemiMinorAxis.y = Utilities::getSemiMinorAxis(
            parameters->semiMajorAxisAndInverseFlattening, parameters->squaredEccentricities);
        parameters->refLonLat.x = 0.0; // Longitude of central meridian
        parameters->refLonLat.y = 0.0; // Latitude of origin
        parameters->falseEastingAndNorthing.x = 0.0; // False easting (km)
        parameters->falseEastingAndNorthing.y = 0.0; // False northing (km)
        parameters->scaleFactor = 1.0; // Grid scale factor
        Utilities::getIntermediateConstantsTM(
            parameters->semiMajorAxisAndInverseFlattening, 1.0, parameters->constants1, parameters->constants2);
    }
    switch (projection)
    {
        case Projection::UTM:
        case Projection::MGRS:
            parameters->falseEastingAndNorthing.x = 500.0; // False easting (km)
            parameters->falseEastingAndNorthing.y = 10000.0; // False northing (km)
            parameters->scaleFactor = 0.9996; // Grid scale factor
        case Projection::HOMV2:
        case Projection::OSTEREO:
        case Projection::TM:
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
            parameters->minZoom = ZoomLevel0;
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
            parameters->minZoom = ZoomLevel0;
            parameters->maxZoomForFloat = ZoomLevel12;
            parameters->maxZoomForMixed = ZoomLevel14;
    }

    return *this;
}

OsmAnd::GridConfiguration& OsmAnd::GridConfiguration::setPrimaryEllipsoidParameters(
    const PointD& helmertTranslationsXY, const PointD& helmertTranslationsZW,
    const PointD& helmertRotationsXY, const PointD& helmertRotationsZScale)
{
    setEllipsoidParameters(0,
        helmertTranslationsXY, helmertTranslationsZW, helmertRotationsXY, helmertRotationsZScale);
    return *this;
}

OsmAnd::GridConfiguration& OsmAnd::GridConfiguration::setSecondaryEllipsoidParameters(
    const PointD& helmertTranslationsXY, const PointD& helmertTranslationsZW,
    const PointD& helmertRotationsXY, const PointD& helmertRotationsZScale)
{
    setEllipsoidParameters(1,
        helmertTranslationsXY, helmertTranslationsZW, helmertRotationsXY, helmertRotationsZScale);
    return *this;
}

OsmAnd::GridConfiguration& OsmAnd::GridConfiguration::setEllipsoidParameters(
    const int gridIndex, const PointD& helmertTranslationsXY, const PointD& helmertTranslationsZW,
    const PointD& helmertRotationsXY, const PointD& helmertRotationsZScale)
{
    auto parameters = &gridParameters[gridIndex];
    parameters->helmertTranslations.x = helmertTranslationsXY.x;
    parameters->helmertTranslations.y = helmertTranslationsXY.y;
    parameters->helmertTranslations.z = helmertTranslationsZW.x;
    parameters->helmertRotations.x = helmertRotationsXY.x;
    parameters->helmertRotations.y = helmertRotationsXY.y;
    parameters->helmertRotations.z = helmertRotationsZScale.x;
    parameters->helmertScale = helmertRotationsZScale.y;
    return *this;
}

OsmAnd::GridConfiguration& OsmAnd::GridConfiguration::setPrimaryProjectionConstants(
    const Projection projection,
    const PointD& lonBounds,
    const PointD& latBounds,
    const PointD& semiMajorAxisAndInverseFlattening,
    const PointD& refLonLat,
    const PointD& falseEastingAndNorthing,
    const PointD& scaleFactor)
{
    setProjectionConstants(0, projection,
        lonBounds, latBounds, semiMajorAxisAndInverseFlattening, refLonLat, falseEastingAndNorthing, scaleFactor);
    return *this;
}

OsmAnd::GridConfiguration& OsmAnd::GridConfiguration::setSecondaryProjectionConstants(
    const Projection projection,
    const PointD& lonBounds,
    const PointD& latBounds,
    const PointD& semiMajorAxisAndInverseFlattening,
    const PointD& refLonLat,
    const PointD& falseEastingAndNorthing,
    const PointD& scaleFactor)
{
    setProjectionConstants(1, projection,
        lonBounds, latBounds, semiMajorAxisAndInverseFlattening, refLonLat, falseEastingAndNorthing, scaleFactor);
    return *this;
}

OsmAnd::GridConfiguration& OsmAnd::GridConfiguration::setProjectionConstants(
    const int gridIndex,
    const Projection projection,
    const PointD& lonBounds,
    const PointD& latBounds,
    const PointD& semiMajorAxisAndInverseFlattening,
    const PointD& refLonLat,
    const PointD& falseEastingAndNorthing,
    const PointD& scaleFactor)
{
    auto parameters = &gridParameters[gridIndex];
    parameters->lonBounds = lonBounds;
    parameters->latBounds = latBounds;
    parameters->semiMajorAxisAndInverseFlattening = semiMajorAxisAndInverseFlattening;
    parameters->semiMajorAxisAndSemiMinorAxis.x = semiMajorAxisAndInverseFlattening.x;
    parameters->semiMajorAxisAndSemiMinorAxis.y =
        Utilities::getSemiMinorAxis(semiMajorAxisAndInverseFlattening, parameters->squaredEccentricities);
    parameters->refLonLat = refLonLat;
    parameters->falseEastingAndNorthing = falseEastingAndNorthing;
    parameters->scaleFactor = scaleFactor.x;
    switch (projection)
    {
        case Projection::HOMV2:
            Utilities::getIntermediateConstantsHOMV2(semiMajorAxisAndInverseFlattening, refLonLat,
                scaleFactor, parameters->falseEastingAndNorthing, parameters->constants1, parameters->constants2);
            break;
        case Projection::OSTEREO:
            Utilities::getIntermediateConstantsOS(semiMajorAxisAndInverseFlattening, refLonLat,
                scaleFactor.x, parameters->constants1, parameters->constants2);
            break;
        case Projection::TM:
            Utilities::getIntermediateConstantsTM(semiMajorAxisAndInverseFlattening,
                scaleFactor.x, parameters->constants1, parameters->constants2);
            break;
        default:
    }
    return *this;
}

OsmAnd::PointD OsmAnd::GridConfiguration::getPrimaryGridReference(
    const PointI& location31) const
{
    return getLocationReference(0, primaryProjection, location31);
}

OsmAnd::PointD OsmAnd::GridConfiguration::getSecondaryGridReference(
    const PointI& location31) const
{
    return getLocationReference(1, secondaryProjection, location31);
}

OsmAnd::PointD OsmAnd::GridConfiguration::getPrimaryGridLocation(
    const PointI& location31, const PointD* referenceDeg /* = nullptr */) const
{
    return projectLocation(0, primaryProjection, location31, referenceDeg);
}

OsmAnd::PointD OsmAnd::GridConfiguration::getSecondaryGridLocation(
    const PointI& location31, const PointD* referenceDeg /* = nullptr */) const
{
    return projectLocation(1, secondaryProjection, location31, referenceDeg);
}

OsmAnd::PointD OsmAnd::GridConfiguration::getLocationReference(
    const int gridIndex, const Projection projection, const PointI& location31) const
{
    PointD result;
    switch (projection)
    {
        case Projection::HOMV2: // Hotine Oblique Mercator central meridian (degrees)
        case Projection::OSTEREO: // Oblique Stereographic central meridian (degrees)
        case Projection::TM: // TM central meridian (degrees)
            result = gridParameters[gridIndex].refLonLat * 180.0 / M_PI;
            break;
        case Projection::UTM: // UTM zone central meridian (degrees)
        case Projection::MGRS:
            Utilities::getZoneUTM(Utilities::getAnglesFrom31(PointI(
                location31.x < 0 ? location31.x + 1 + INT32_MAX : location31.x,
                location31.y < 0 ? location31.y + 1 + INT32_MAX : location31.y)), &result.x);
            result.y = 0.0;
            break;
        case Projection::Mercator: // EPSG:3857 prime meridian
        default: // EPSG:4326 prime meridian
            result.x = 0.0;
            result.y = 0.0;
    }
    return result;
}

OsmAnd::PointD OsmAnd::GridConfiguration::projectLocation(const int gridIndex, const Projection projection,
    const PointI& location31, const PointD* referenceDeg /* = nullptr */) const
{
    PointD result;
    switch (projection)
    {
        case Projection::HOMV2: // Hotine Oblique Mercator easting and northing coordinates (100 kilometers)
            {
                const auto lonlat = Utilities::getAnglesFrom31(location31);
                auto parameters = &gridParameters[gridIndex];
                const auto ellipsoidLonLat = Utilities::getEllipsoidCoordinates(
                    lonlat,
                    6378.137,
                    parameters->semiMajorAxisAndSemiMinorAxis,
                    parameters->squaredEccentricities,
                    parameters->helmertTranslations,
                    parameters->helmertRotations,
                    parameters->helmertScale);
                result = Utilities::getCoordinatesHOMV2(
                    ellipsoidLonLat,
                    parameters->falseEastingAndNorthing,
                    parameters->constants1,
                    parameters->constants2) / 100.0;
            }
            break;
        case Projection::OSTEREO: // Oblique Stereographic easting and northing coordinates (100 kilometers)
            {
                const auto lonlat = Utilities::getAnglesFrom31(location31);
                auto parameters = &gridParameters[gridIndex];
                const auto ellipsoidLonLat = Utilities::getEllipsoidCoordinates(
                    lonlat,
                    6378.137,
                    parameters->semiMajorAxisAndSemiMinorAxis,
                    parameters->squaredEccentricities,
                    parameters->helmertTranslations,
                    parameters->helmertRotations,
                    parameters->helmertScale);
                result = Utilities::getCoordinatesOS(
                    ellipsoidLonLat,
                    parameters->refLonLat,
                    parameters->falseEastingAndNorthing,
                    parameters->constants1,
                    parameters->constants2) / 100.0;
            }
            break;
        case Projection::TM: // TM easting and northing coordinates (100 kilometers)
            {
                const auto lonlat = Utilities::getAnglesFrom31(location31);
                auto parameters = &gridParameters[gridIndex];
                const auto ellipsoidLonLat = Utilities::getEllipsoidCoordinates(
                    lonlat,
                    6378.137,
                    parameters->semiMajorAxisAndSemiMinorAxis,
                    parameters->squaredEccentricities,
                    parameters->helmertTranslations,
                    parameters->helmertRotations,
                    parameters->helmertScale);
                result = Utilities::getCoordinatesTM(
                    ellipsoidLonLat,
                    parameters->refLonLat,
                    parameters->falseEastingAndNorthing,
                    parameters->constants1,
                    parameters->constants2) / 100.0;
            }
            break;
        case Projection::UTM: // UTM easting and northing coordinates (100 kilometers)
        case Projection::MGRS:
            {
                const auto lonlat = Utilities::getAnglesFrom31(location31);
                double refLon;
                if (referenceDeg)
                    refLon = referenceDeg->x;
                else
                    Utilities::getZoneUTM(lonlat, &refLon);
                auto parameters = &gridParameters[gridIndex];
                result = Utilities::getCoordinatesTM(
                    lonlat,
                    PointD(refLon * M_PI / 180.0, parameters->refLonLat.y),
                    parameters->falseEastingAndNorthing,
                    parameters->constants1,
                    parameters->constants2) / 100.0;
            }
            break;
        case Projection::Mercator: // EPSG:3857 X and Y coordinates (100 map kilometers)
            result = Utilities::metersFrom31(PointD(location31)) / 100000.0;
            break;
        default: // EPSG:4326 longitude and latitude (degrees)
            {
                const bool isOverX = location31.x < 0;
                const bool isOverY = location31.y < 0;
                const auto lonlat = Utilities::getAnglesFrom31(PointI(
                    isOverX ? location31.x + 1 + INT32_MAX : location31.x,
                    isOverY ? location31.y + 1 + INT32_MAX : location31.y));
                result = (lonlat - PointD(isOverX ? M_PI * 2.0 : 0.0, isOverY ? M_PI : 0.0)) * 180.0 / M_PI;
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
        case Projection::HOMV2: // TODO: unproject to 31-coordinates
        case Projection::OSTEREO:
        case Projection::TM:
        case Projection::UTM:
        case Projection::MGRS:
            break;
        case Projection::Mercator: // Get 31-coordinates from EPSG:3857 X and Y coordinates in 100 map kilometers
            result = Utilities::metersTo31(location * 100000.0);
            break;
        default: // Get 31-coordinates from EPSG:4326 longitude and latitude in degrees
            result = Utilities::get31FromAngles(location * M_PI / 180.0);
    }
    return result;
}

OsmAnd::PointD OsmAnd::GridConfiguration::getPrimaryGridFullturnLocation(
    const PointD& location) const
{
    return getFullturnLocation(primaryProjection, location);
}

OsmAnd::PointD OsmAnd::GridConfiguration::getSecondaryGridFullturnLocation(
    const PointD& location) const
{
    return getFullturnLocation(secondaryProjection, location);
}

OsmAnd::PointD OsmAnd::GridConfiguration::getFullturnLocation(
    const Projection projection, const PointD& location) const
{
    PointD result;
    switch (projection)
    {
        case Projection::HOMV2: // Return coordinates unchanged
        case Projection::OSTEREO:
        case Projection::TM:
        case Projection::UTM:
        case Projection::MGRS:
            result = location;
            break;
        case Projection::Mercator: // EPSG:3857 X and Y full-turn coordinates (100 map kilometers)
            {
                const auto half = M_PI * 6378137;
                const auto full = 2.0 * half;
                result = location + PointD(location.x < -half ? full : -full, location.y < -half ? full : -full);
            }
            break;
        default: // EPSG:4326 full-turn longitude and latitude (degrees)
            result = location + PointD(location.x < -180.0 ? 360.0 : -360.0, location.y < -90.0 ? 180.0 : -180.0);
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
        case Projection::HOMV2: // Easting coordinate (100 kilometers)
        case Projection::OSTEREO:
        case Projection::TM:
        case Projection::UTM:
        case Projection::MGRS:
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
        case Projection::HOMV2: // Northing coordinate (100 kilometers)
        case Projection::OSTEREO:
        case Projection::TM:
        case Projection::UTM:
        case Projection::MGRS:
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
        case Projection::HOMV2: // No need to limit marks
        case Projection::OSTEREO:
        case Projection::TM:
        case Projection::UTM:
        case Projection::MGRS:
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
    return getMarkY(0, primaryProjection, primaryFormat, coordinates, zone);
}

QString OsmAnd::GridConfiguration::getSecondaryGridMarkX(const PointD& coordinates, const int zone) const
{
    return getMarkX(secondaryProjection, secondaryFormat, coordinates, zone);
}

QString OsmAnd::GridConfiguration::getSecondaryGridMarkY(const PointD& coordinates, const int zone) const
{
    return getMarkY(1, secondaryProjection, secondaryFormat, coordinates, zone);
}

QString OsmAnd::GridConfiguration::getMarkX(
    const Projection projection, const Format format, const PointD& coordinates, const int zone) const
{
    QString result;
    switch (projection)
    {
        case Projection::HOMV2: // Hotine Oblique Mercator easting coordinate (meters)
        case Projection::OSTEREO: // Oblique Stereographic easting coordinate (meters)
        case Projection::TM: // TM easting coordinate (meters)
            result = QStringLiteral("%1").arg(coordinates.x * 100000.0, 0, 'f', 0);
            break;
        case Projection::UTM: // UTM easting coordinate (zone, hemisphere, meters)
            {
                PointI zoneUTM(zone & 63, zone >> 6);
                if (zoneUTM.y == 12 && coordinates.y >= 100.0)
                    zoneUTM.y = 13;
                else if (zoneUTM.y == 13 && coordinates.y < 100.0)
                    zoneUTM.y = 12;
                // Display MGRS latitude band letters instead of usual UTM hemisphere indicator
                //QString hemisphere = zoneUTM.y < 13 ? QStringLiteral("S") : QStringLiteral("N");
                QString zoneLetter = Utilities::getMGRSLetter(zoneUTM.y - 1);
                result =
                    QStringLiteral("%1%2 %3").arg(zoneUTM.x).arg(zoneLetter).arg(coordinates.x * 100000.0, 0, 'f', 0);
            }
            break;
        case Projection::MGRS: // MGRS easting coordinate (zone, square, meters)
            {
                PointI zoneUTM(zone & 63, zone >> 6);
                QString zoneLetter = Utilities::getMGRSLetter(zoneUTM.y - 1);
                auto zoneCoords = coordinates;
                if (zoneUTM.y == 12 && coordinates.y >= 100.0)
                    zoneCoords.y = 99.999999;
                else if (zoneUTM.y == 13 && coordinates.y < 100.0)
                    zoneCoords.y = 100.0;
                QString column = Utilities::getMGRSSquareColumn(zoneUTM, zoneCoords);
                QString row = Utilities::getMGRSSquareRow(zoneUTM, zoneCoords);
                const auto coordinate = coordinates.x - std::floor(coordinates.x);
                result = QStringLiteral("%1%2 %3%4 %5").arg(zoneUTM.x).arg(zoneLetter).arg(column).arg(row).arg(
                    coordinate * 100000.0, 5, 'f', 0, QLatin1Char('0'));
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

QString OsmAnd::GridConfiguration::getMarkY(const int gridIndex,
    const Projection projection, const Format format, const PointD& coordinates, const int zone) const
{
    QString result;
    switch (projection)
    {
        case Projection::HOMV2: // Hotine Oblique Mercator northing coordinate (meters)
        case Projection::OSTEREO: // Oblique Stereographic northing coordinate (meters)
        case Projection::TM: // TM northing coordinate (meters)
            result = QStringLiteral("%1").arg(coordinates.y * 100000.0, 0, 'f', 0);
            break;
        case Projection::UTM: // UTM northing coordinate (zone, hemisphere, meters)
            {
                PointI zoneUTM(zone & 63, zone >> 6);
                if (zoneUTM.y == 12 && coordinates.y >= 100.0)
                    zoneUTM.y = 13;
                else if (zoneUTM.y == 13 && coordinates.y < 100.0)
                    zoneUTM.y = 12;
                // Display MGRS latitude band letters instead of usual UTM hemisphere indicator
                //QString hemisphere = zoneUTM.y < 13 ? QStringLiteral("S") : QStringLiteral("N");
                QString zoneLetter = Utilities::getMGRSLetter(zoneUTM.y - 1);
                const auto coordinate = std::round(coordinates.y * 100000.0) - (zoneUTM.y < 13 ? 0.0 : 10000000.0);
                result = QStringLiteral("%1%2 %3").arg(zoneUTM.x).arg(zoneLetter).arg(coordinate, 0, 'f', 0);
            }
            break;
        case Projection::MGRS: // MGRS northing coordinate (zone, square, meters)
            {
                PointI zoneUTM(zone & 63, zone >> 6);
                if (zoneUTM.y == 12 && coordinates.y >= 100.0)
                    zoneUTM.y = 13;
                else if (zoneUTM.y == 13 && coordinates.y < 100.0)
                    zoneUTM.y = 12;
                QString zoneLetter = Utilities::getMGRSLetter(zoneUTM.y - 1);
                QString column = Utilities::getMGRSSquareColumn(zoneUTM, coordinates);
                QString row = Utilities::getMGRSSquareRow(zoneUTM, coordinates);
                const auto coordinate = coordinates.y - std::floor(coordinates.y);
                result = QStringLiteral("%1%2 %3%4 %5").arg(zoneUTM.x).arg(zoneLetter).arg(column).arg(row).arg(
                    coordinate * 100000.0, 5, 'f', 0, QLatin1Char('0'));
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
    const PointI& target31, const ZoomLevel& zoomLevel,
    PointD* refLons_ /* = nullptr */, PointD* refLats_ /* = nullptr */) const
{
    PointD result(primaryGap, secondaryGap);
    PointD refLonLat;
    PointD refLons(NAN, NAN);
    PointD refLats(NAN, NAN);
    const auto shift = MaxZoomLevel - zoomLevel;
    PointI tile31(target31.x >> shift, target31.y >> shift);
    tile31.x <<= shift;
    tile31.y <<= shift;
    auto tileSize31 = 1u << shift >> 1;
    PointI startTile31(tile31.x, tile31.y + tileSize31);
    PointI centerTile31(tile31.x + tileSize31, tile31.y + tileSize31);
    if (primaryGranularity > 0.0f)
    {
        refLonLat = getPrimaryGridReference(centerTile31);
        refLons.x = refLonLat.x;
        refLats.x = refLonLat.y;
        auto tileBegin = getPrimaryGridLocation(startTile31, &refLonLat);
        auto tileCenter = getPrimaryGridLocation(centerTile31, &refLonLat);
        auto cellSize = fabs(tileCenter.x - tileBegin.x) * 2.0 * primaryGranularity;
        result.x = primaryFormat == Format::DMS && primaryProjection == Projection::WGS84
            ? Utilities::snapToGridDMS(cellSize)
            : (primaryFormat == Format::DM && primaryProjection == Projection::WGS84
                ? Utilities::snapToGridDM(cellSize) : Utilities::snapToGridDecimal(cellSize));
    }
    if (secondaryGranularity > 0.0f)
    {
        auto difFactor =
            primaryGranularity / secondaryGranularity;
        if (primaryGranularity == 0.0 || primaryProjection != secondaryProjection
            || difFactor - std::floor(difFactor) > 0.0f)
        {
            refLonLat = getSecondaryGridReference(centerTile31);
            refLons.y = refLonLat.x;
            refLats.y = refLonLat.y;
            auto tileBegin = getSecondaryGridLocation(startTile31, &refLonLat);
            auto tileCenter = getSecondaryGridLocation(centerTile31, &refLonLat);
            auto cellSize = fabs(tileCenter.x - tileBegin.x) * 2.0 * secondaryGranularity;
            result.y = secondaryFormat == Format::DMS && secondaryProjection == Projection::WGS84
                ? Utilities::snapToGridDMS(cellSize)
                : (secondaryFormat == Format::DM && secondaryProjection == Projection::WGS84
                    ? Utilities::snapToGridDM(cellSize) : Utilities::snapToGridDecimal(cellSize));
            }
        else
            result.y = result.x / static_cast<double>(difFactor);
    }

    if (refLons_)
        *refLons_ = refLons;

    if (refLats_)
        *refLats_ = refLats;

    result.x = correctGap(primaryProjection, result.x);
    result.y = correctGap(secondaryProjection, result.y);

    return result;
}

double OsmAnd::GridConfiguration::correctGap(const Projection projection, const double gap) const
{
    double result;
    switch (projection)
    {
        case Projection::HOMV2: // No need to correct gap
        case Projection::OSTEREO:
        case Projection::TM:
        case Projection::UTM:
        case Projection::MGRS:
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
        || primaryProjection == Projection::HOMV2
        || primaryProjection == Projection::OSTEREO
        || primaryProjection == Projection::TM
        || primaryProjection == Projection::UTM
        || primaryProjection == Projection::MGRS
        || primaryProjection == Projection::Mercator) &&
        (secondaryProjection == Projection::WGS84
        || secondaryProjection == Projection::HOMV2
        || secondaryProjection == Projection::OSTEREO
        || secondaryProjection == Projection::TM
        || secondaryProjection == Projection::UTM
        || secondaryProjection == Projection::MGRS
        || secondaryProjection == Projection::Mercator);
}

class OsmAnd::CoordinateTransformer_P Q_DECL_FINAL
{
private:
    int towsg84_epsg;
    OGRSpatialReference source_crs;
    OGRSpatialReference target_crs;
    OGRCoordinateTransformation* forwardTransform;
    OGRCoordinateTransformation* backwardTransform;

public:
    CoordinateTransformer_P(const QString& projResourcesPath, int epsg_number, int towsg84_epsg_number);
    ~CoordinateTransformer_P();

    bool fromLonLat(PointD& location);
    bool toLonLat(PointD& location);
    bool getConstants(const GridConfiguration::Projection projection,
        PointD& lonBounds, PointD& latBounds, PointD& semiMajorAxisAndInverseFlattening,
        PointD& refLonLat, PointD& falseEastingAndNorthing, PointD& scaleFactor);
    bool getEllipsoidParameters(PointD& helmertTranslationsXY, PointD& helmertTranslationsZW,
        PointD& helmertRotationsXY, PointD& helmertRotationsZScale);
};

OsmAnd::CoordinateTransformer_P::CoordinateTransformer_P(
    const QString& projResourcesPath, int epsg_number, int towsg84_epsg_number)
    : forwardTransform(nullptr)
    , backwardTransform(nullptr)
{
    auto projSearchPath = projResourcesPath.toUtf8();
    const char* projPaths[] = { projSearchPath.constData(), NULL };
    OSRSetPROJSearchPaths(projPaths);

    auto res = source_crs.importFromEPSG(4326);
    if (res != OGRERR_NONE)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to create source GDAL CRS.");
        return;
    }
    source_crs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    
    res = target_crs.importFromEPSG(epsg_number);
    if (res != OGRERR_NONE)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to create target GDAL CRS.");
        return;
    }
    target_crs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    forwardTransform = OGRCreateCoordinateTransformation(&source_crs, &target_crs);
    if (!forwardTransform)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to create forward GDAL transformation.");
        return;
    }

    backwardTransform = OGRCreateCoordinateTransformation(&target_crs, &source_crs);
    if (!backwardTransform)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to create backward GDAL transformation.");
        return;
    }

    towsg84_epsg = towsg84_epsg_number;
}

OsmAnd::CoordinateTransformer_P::~CoordinateTransformer_P()
{
    if (forwardTransform)
        OGRCoordinateTransformation::DestroyCT(forwardTransform);
    if (backwardTransform)
        OGRCoordinateTransformation::DestroyCT(backwardTransform);
}

bool OsmAnd::CoordinateTransformer_P::fromLonLat(PointD& location)
{
    if (forwardTransform)
        return forwardTransform->Transform(1, &location.x, &location.y);
    return false;
}

bool OsmAnd::CoordinateTransformer_P::toLonLat(PointD& location)
{
    if (backwardTransform)
        return backwardTransform->Transform(1, &location.x, &location.y);
    return false;
}

bool OsmAnd::CoordinateTransformer_P::getConstants(
    const GridConfiguration::Projection projection, PointD& lonBounds, PointD& latBounds,
    PointD& semiMajorAxisAndInverseFlattening, PointD& refLonLat, PointD& falseEastingAndNorthing,
    PointD& scaleFactor)
{
    if (!forwardTransform || !backwardTransform)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constants: not initialized.");
        return false;
    }

    if (!target_crs.IsProjected())
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constants: can't be projected.");
        return false;
    }

    const char* pszProjection = target_crs.GetAttrValue("PROJECTION");
    if (!pszProjection)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constants: unknown projection.");
        return false;
    }

    if (projection == GridConfiguration::Projection::HOMV2
        && std::string(pszProjection) != SRS_PT_HOTINE_OBLIQUE_MERCATOR_AZIMUTH_CENTER)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error,
            "Failed to get CRS constants: not an Hotine Oblique Mercator projection.");
        return false;
    }

    if (projection == GridConfiguration::Projection::OSTEREO
        && std::string(pszProjection) != SRS_PT_OBLIQUE_STEREOGRAPHIC)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error,
            "Failed to get CRS constants: not an Oblique Stereographic projection.");
        return false;
    }

    if (projection == GridConfiguration::Projection::TM
        && std::string(pszProjection) != SRS_PT_TRANSVERSE_MERCATOR)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error,
            "Failed to get CRS constants: not a Transverse Mercator projection.");
        return false;
    }

    const auto radFactor = M_PI / 180.0;

    const char* pszAreaName = nullptr;

    if (!target_crs.GetAreaOfUse(&lonBounds.x, &latBounds.x, &lonBounds.y, &latBounds.y, &pszAreaName))
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constants: undefined area.");
        return false;
    }
    else
    {
        lonBounds *= radFactor;
        latBounds *= radFactor;
    }

    OGRErr err;

    refLonLat.x = target_crs.GetProjParm(SRS_PP_CENTRAL_MERIDIAN, 0.0, &err) * radFactor;
    if (err != OGRERR_NONE)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constant: central meridian.");
        return false;
    }
    refLonLat.y = target_crs.GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, 0.0, &err) * radFactor;
    if (err != OGRERR_NONE)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constant: latitude of origin.");
        return false;
    }
    semiMajorAxisAndInverseFlattening.x = target_crs.GetSemiMajor(&err) / 1000.0;
    if (err != OGRERR_NONE)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constant: semi-major axis.");
        return false;
    }
    semiMajorAxisAndInverseFlattening.y = target_crs.GetInvFlattening(&err);
    if (err != OGRERR_NONE)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constant: inverse flattening.");
        return false;
    }
    falseEastingAndNorthing.x = target_crs.GetProjParm(SRS_PP_FALSE_EASTING, 0.0, &err) / 1000.0;
    if (err != OGRERR_NONE)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constant: false easting.");
        return false;
    }
    falseEastingAndNorthing.y = target_crs.GetProjParm(SRS_PP_FALSE_NORTHING, 0.0, &err) / 1000.0;
    if (err != OGRERR_NONE)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constant: false northing.");
        return false;
    }
    scaleFactor.x = target_crs.GetProjParm(SRS_PP_SCALE_FACTOR, 1.0, &err);
    if (err != OGRERR_NONE)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constant: scale factor.");
        return false;
    }
    if (projection == GridConfiguration::Projection::HOMV2)
    {
        scaleFactor.y = target_crs.GetProjParm(SRS_PT_HOTINE_OBLIQUE_MERCATOR_AZIMUTH_CENTER, 0.0, &err) * radFactor;
        if (err != OGRERR_NONE)
        {
            LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constant: azimuth center.");
            return false;
        }
    }
    else
        scaleFactor.y = scaleFactor.x;

    return true;
}

bool OsmAnd::CoordinateTransformer_P::getEllipsoidParameters(
    PointD& helmertTranslationsXY, PointD& helmertTranslationsZW,
    PointD& helmertRotationsXY, PointD& helmertRotationsZScale)
{
    if (!forwardTransform || !backwardTransform)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get CRS constants: not initialized.");
        helmertRotationsZScale.y = 1.0;
        return false;
    }

    bool isPositionVector = true;
    double coeffs[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    target_crs.AddGuessedTOWGS84();
    bool isFound = target_crs.GetTOWGS84(coeffs) == OGRERR_NONE;
    if (!isFound && towsg84_epsg > 0)
    {
        isPositionVector = false;
        auto ctx = OSRGetProjTLSContext();
        auto pj = proj_create(ctx,
            QStringLiteral("urn:ogc:def:coordinateOperation:EPSG::%1").arg(towsg84_epsg).toUtf8().constData());
        if (!pj)
        {
            LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to create PROJ coordinate operation.");
            helmertRotationsZScale.y = 1.0;
            return false;
        }
        auto projStr = proj_as_proj_string(ctx, pj, PJ_PROJ_5, nullptr);
        if (projStr)
        {
            auto pipeline = QString::fromUtf8(projStr);
            int helmertPos = pipeline.indexOf(QStringLiteral("+proj=helmert"));
            if (helmertPos != -1) {
                isFound = true;
                int nextStepPos = pipeline.indexOf(QStringLiteral("+step"), helmertPos);
                QStringRef helmertStep = (nextStepPos != -1) 
                    ? pipeline.midRef(helmertPos, nextStepPos - helmertPos)
                    : pipeline.midRef(helmertPos);
                isPositionVector = helmertStep.indexOf(QStringLiteral("+convention=position_vector")) != -1;
                QRegularExpression paramRegex(QStringLiteral("\\+([a-z_]+)=([-0-9.eE+]+)"));
                QRegularExpressionMatchIterator i = paramRegex.globalMatch(helmertStep);
                double rates[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
                double epoch = 0.0;
                bool hasEpoch = false;
                while (i.hasNext())
                {
                    auto match = i.next();
                    auto key = match.capturedRef(1);
                    bool ok = true;
                    double value = match.capturedRef(2).toDouble(&ok);
                    if (ok)
                    {
                        if (key == QStringLiteral("x"))
                            coeffs[0] = value;
                        else if (key == QStringLiteral("y"))
                            coeffs[1] = value;
                        else if (key == QStringLiteral("z"))
                            coeffs[2] = value;
                        else if (key == QStringLiteral("rx"))
                            coeffs[3] = value;
                        else if (key == QStringLiteral("ry"))
                            coeffs[4] = value;
                        else if (key == QStringLiteral("rz"))
                            coeffs[5] = value;
                        else if (key == QStringLiteral("s"))
                            coeffs[6] = value;
                        else if (key == QStringLiteral("dx"))
                            rates[0] = value;
                        else if (key == QStringLiteral("dy"))
                            rates[1] = value;
                        else if (key == QStringLiteral("dz"))
                            rates[2] = value;
                        else if (key == QStringLiteral("drx"))
                            rates[3] = value;
                        else if (key == QStringLiteral("dry"))
                            rates[4] = value;
                        else if (key == QStringLiteral("drz"))
                            rates[5] = value;
                        else if (key == QStringLiteral("ds"))
                            rates[6] = value;
                        else if (key == QStringLiteral("t_epoch"))
                        {
                            epoch = value;
                            hasEpoch = true;
                        }
                    }
                }
                if (hasEpoch)
                {
                    auto now = QDateTime::currentDateTimeUtc();
                    int year = now.date().year();
                    QDateTime s(QDate(year, 1, 1), QTime(0, 0));
                    QDateTime f(QDate(year + 1, 1, 1), QTime(0, 0));
                    auto dt = static_cast<double>(s.msecsTo(now)) / static_cast<double>(s.msecsTo(f)) + year - epoch;
                    for(int j = 0; j < 7; j++)
                    {
                        coeffs[j] += (rates[j] * dt);
                    }
                }
            }
        }
        proj_destroy(pj);
    }

    if (!isFound)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Error, "Failed to get Helmert shift parameters.");
        helmertRotationsZScale.y = 1.0;
        return false;
    }

    const auto radFactor = M_PI / (180.0 * 3600.0);

    helmertTranslationsXY.x = -coeffs[0] / 1000.0;
    helmertTranslationsXY.y = -coeffs[1] / 1000.0;
    helmertTranslationsZW.x = -coeffs[2] / 1000.0;
    helmertRotationsXY.x = (isPositionVector ? -coeffs[3] : coeffs[3]) * radFactor;
    helmertRotationsXY.y = (isPositionVector ? -coeffs[4] : coeffs[4]) * radFactor;
    helmertRotationsZScale.x = (isPositionVector ? -coeffs[5] : coeffs[5]) * radFactor;
    helmertRotationsZScale.y = 1.0 + (-coeffs[6] * 1e-6);

    return true;
}

OsmAnd::CoordinateTransformer::CoordinateTransformer(
    const QString& projResourcesPath, int epsg_number, int towsg84_epsg_number /* = 0 */)
    : _p(new CoordinateTransformer_P(projResourcesPath, epsg_number, towsg84_epsg_number))
{
}

OsmAnd::CoordinateTransformer::~CoordinateTransformer() = default;

bool OsmAnd::CoordinateTransformer::fromLonLat(PointD& location)
{
    return _p->fromLonLat(location);
}

bool OsmAnd::CoordinateTransformer::toLonLat(PointD& location)
{
    return _p->toLonLat(location);
}

bool OsmAnd::CoordinateTransformer::getConstants(
    const GridConfiguration::Projection projection, PointD& lonBounds, PointD& latBounds,
    PointD& semiMajorAxisAndInverseFlattening, PointD& refLonLat, PointD& falseEastingAndNorthing,
    PointD& scaleFactor)
{
    return _p->getConstants(projection, lonBounds, latBounds, semiMajorAxisAndInverseFlattening, refLonLat,
        falseEastingAndNorthing, scaleFactor);
}

bool OsmAnd::CoordinateTransformer::getEllipsoidParameters(
    PointD& helmertTranslationsXY, PointD& helmertTranslationsZW,
    PointD& helmertRotationsXY, PointD& helmertRotationsZScale)
{
    return _p->getEllipsoidParameters(helmertTranslationsXY, helmertTranslationsZW,
        helmertRotationsXY, helmertRotationsZScale);
}
