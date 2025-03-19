#include "GridMarksProvider_P.h"
#include "GridMarksProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapMarker.h"
#include "MapRenderer.h"

# define MIN_ZOOM_LEVEL_SIDE_MARKS 5

# define GRID_MARKS_PER_AXIS 20
# define MAP_CHANGE_FACTOR_PART 4

OsmAnd::GridMarksProvider_P::GridMarksProvider_P(
    GridMarksProvider* const owner_)
	: _markersCollection(std::make_shared<MapMarkersCollection>())
	, owner(owner_)
    , withPrimaryValues(false)
    , withSecondaryValues(false)
    , primaryMarksOffset(1.0)
    , secondaryMarksOffset(1.0)
    , _target31(PointI(-1, -1))
    , _primaryZone(-1)
    , _secondaryZone(-1)
{
}

OsmAnd::GridMarksProvider_P::~GridMarksProvider_P()
{
}

void OsmAnd::GridMarksProvider_P::calculateGridMarks(const bool isPrimary,
    const double gap, const double refLon, QHash<int, PointD>& marksX, QHash<int, PointD>& marksY)
{
    auto marksCount = static_cast<int>(std::min(static_cast<double>(GRID_MARKS_PER_AXIS),
        isPrimary ? _gridConfiguration.getPrimaryMaxMarksPerAxis(gap)
        : _gridConfiguration.getSecondaryMaxMarksPerAxis(gap)));
    double halfCount = marksCount / 2;
    auto location = (isPrimary ? _gridConfiguration.getPrimaryGridLocation(_target31, &refLon)
        : _gridConfiguration.getSecondaryGridLocation(_target31, &refLon)) / gap;
    PointD centerLocation(std::round(location.x), std::round(location.y));
    location.x = (centerLocation.x - halfCount) * gap;
    location.y = (centerLocation.y - halfCount) * gap;
    centerLocation *= gap;
    for (int i = 0; i < marksCount; i++)
    {
        auto locationN = location;
        bool withX = isPrimary ? _gridConfiguration.getPrimaryGridCoordinateX(locationN.x)
            : _gridConfiguration.getSecondaryGridCoordinateX(locationN.x);
        bool withY = isPrimary ? _gridConfiguration.getPrimaryGridCoordinateY(locationN.y)
            : _gridConfiguration.getSecondaryGridCoordinateY(locationN.y);
        PointF coord(static_cast<float>(locationN.x), static_cast<float>(locationN.y));
        if (withX)
            marksX.insert(*reinterpret_cast<int*>(&coord.x), PointD(locationN.x, centerLocation.y));
        if (withY)
            marksY.insert(*reinterpret_cast<int*>(&coord.y), PointD(centerLocation.x, locationN.y));
        location.x += gap;
        location.y += gap;
    }
}

void OsmAnd::GridMarksProvider_P::addGridMarks(const int zone, const bool isPrimary,
    const bool isAxisY, const bool isExtra, const float offset, QHash<int, PointD>& marks, QSet<int>& availableIds)
{
    auto type = isPrimary ?
        (isAxisY
            ? (centerPrimaryMarks || _mapZoomLevel < MIN_ZOOM_LEVEL_SIDE_MARKS ? PositionType::PrimaryGridYMiddle
                : (isExtra ? PositionType::PrimaryGridYLast : PositionType::PrimaryGridYFirst))
            : (centerPrimaryMarks || _mapZoomLevel < MIN_ZOOM_LEVEL_SIDE_MARKS ? PositionType::PrimaryGridXMiddle
                : (isExtra ? PositionType::PrimaryGridXLast : PositionType::PrimaryGridXFirst)))
        : (isAxisY
            ? (centerSecondaryMarks || _mapZoomLevel < MIN_ZOOM_LEVEL_SIDE_MARKS ? PositionType::SecondaryGridYMiddle
                : (isExtra ? PositionType::SecondaryGridYLast : PositionType::SecondaryGridYFirst))
            : (centerSecondaryMarks || _mapZoomLevel < MIN_ZOOM_LEVEL_SIDE_MARKS ? PositionType::SecondaryGridXMiddle
                : (isExtra ? PositionType::SecondaryGridXLast : PositionType::SecondaryGridXFirst)));

    OsmAnd::MapMarkerBuilder builder;
    builder.setBaseOrder(std::numeric_limits<int>::min() + (isPrimary ? 10 : 20));
    builder.setPosition(_target31);
    builder.setCaptionStyle(isPrimary ? primaryStyle : secondaryStyle);
    for (const auto& markEntry : rangeOf(constOf(marks)))
    {
        auto itId = availableIds.begin();
        auto coordinates = markEntry.value();
        auto suffix = isPrimary
            ? (isAxisY
                ? (coordinates.y < 0.0 ? primarySouthernHemisphereSuffix : primaryNorthernHemisphereSuffix)
                : (coordinates.x < 0.0 ? primaryWesternHemisphereSuffix : primaryEasternHemisphereSuffix))
            : (isAxisY
                ? (coordinates.y < 0.0 ? secondarySouthernHemisphereSuffix : secondaryNorthernHemisphereSuffix)
                : (coordinates.x < 0.0 ? secondaryWesternHemisphereSuffix : secondaryEasternHemisphereSuffix));
        QString mark;
        if (withPrimaryValues && isPrimary)
        {
            mark = isAxisY ? _gridConfiguration.getPrimaryGridMarkY(coordinates, zone)
                : _gridConfiguration.getPrimaryGridMarkX(coordinates, zone);
        } else if (withSecondaryValues && !isPrimary)
        {
            mark = isAxisY ? _gridConfiguration.getSecondaryGridMarkY(coordinates, zone)
                : _gridConfiguration.getSecondaryGridMarkX(coordinates, zone);
        }
        builder.setCaption(mark % suffix);
        builder.setMarkerId(*itId);
        builder.setCaptionTopSpace(offset);
        const auto newMarker = builder.buildAndAddToCollection(_markersCollection);
        newMarker->setPositionType(type);
        newMarker->setAdditionalPosition(isAxisY ? coordinates.y : coordinates.x);
        availableIds.erase(itId);
    }
}

void OsmAnd::GridMarksProvider_P::setPrimaryStyle(
    const TextRasterizer::Style& style, const float offset, bool center)
{
    primaryStyle = style;
    primaryMarksOffset = offset;
    centerPrimaryMarks = center;
    _primaryZone = -1;
    _target31 = PointI(-1, -1);
}

void OsmAnd::GridMarksProvider_P::setSecondaryStyle(
    const TextRasterizer::Style& style, const float offset, bool center)
{
    secondaryStyle = style;
    secondaryMarksOffset = offset;
    centerSecondaryMarks = center;
    _primaryZone = -1;
    _target31 = PointI(-1, -1);
}

void OsmAnd::GridMarksProvider_P::setPrimary(const bool withValues, const QString& northernSuffix,
    const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix)
{
    withPrimaryValues = withValues;
    primaryNorthernHemisphereSuffix = northernSuffix;
    primarySouthernHemisphereSuffix = southernSuffix;
    primaryEasternHemisphereSuffix = easternSuffix;
    primaryWesternHemisphereSuffix = westernSuffix;
    _primaryZone = -1;
    _target31 = PointI(-1, -1);
}

void OsmAnd::GridMarksProvider_P::setSecondary(const bool withValues, const QString& northernSuffix,
    const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix)
{
    withSecondaryValues = withValues;
    secondaryNorthernHemisphereSuffix = northernSuffix;
    secondarySouthernHemisphereSuffix = southernSuffix;
    secondaryEasternHemisphereSuffix = easternSuffix;
    secondaryWesternHemisphereSuffix = westernSuffix;
    _primaryZone = -1;
    _target31 = PointI(-1, -1);
}

void OsmAnd::GridMarksProvider_P::applyMapChanges(IMapRenderer* renderer)
{
    AreaI visibleBBoxShifted;
    PointI target31;
    auto mapZoomLevel = renderer->getVisibleArea(&visibleBBoxShifted, &target31);
    bool changed = _mapZoomLevel != mapZoomLevel;
    if (!changed && _visibleBBoxShifted != visibleBBoxShifted)
    {
        const AreaI64 visibleBBoxShifted64(_visibleBBoxShifted);
        auto shTL = visibleBBoxShifted64.topLeft - visibleBBoxShifted.topLeft;
        auto shBR = visibleBBoxShifted64.bottomRight - visibleBBoxShifted.bottomRight;
        auto lim = PointI64(visibleBBoxShifted64.width(), visibleBBoxShifted64.height()) / MAP_CHANGE_FACTOR_PART;
        changed = abs(shTL.x) > lim.x || abs(shTL.y) > lim.y || abs(shBR.x) > lim.x || abs(shBR.y) > lim.y;
    }

    auto zone = _primaryZone;
    if (_target31 != target31)
    {
        _target31 = target31;
        zone = Utilities::getCodedZoneUTM(target31, false);
    }

    bool keepPrimary = _primaryZone == zone;
    bool keepSecondary = _secondaryZone == zone;
    if (!keepPrimary || !keepSecondary)
        changed = true;

    if (changed)
    {
        _visibleBBoxShifted = visibleBBoxShifted;
        _primaryZone = zone;
        _secondaryZone = zone;
    
        renderer->getGridConfiguration(&_gridConfiguration, &mapZoomLevel);

        const bool withPrimary = mapZoomLevel >= _gridConfiguration.gridParameters[0].minZoom
            && mapZoomLevel >= _gridConfiguration.primaryMinZoomLevel
            && mapZoomLevel <= _gridConfiguration.primaryMaxZoomLevel && (withPrimaryValues
            || !primaryNorthernHemisphereSuffix.isEmpty() || !primarySouthernHemisphereSuffix.isEmpty()
            || !primaryEasternHemisphereSuffix.isEmpty() || !primaryWesternHemisphereSuffix.isEmpty());
        const bool withSecondary = mapZoomLevel >= _gridConfiguration.gridParameters[1].minZoom
            && mapZoomLevel >= _gridConfiguration.secondaryMinZoomLevel
            && mapZoomLevel <= _gridConfiguration.secondaryMaxZoomLevel && (withSecondaryValues
            || !secondaryNorthernHemisphereSuffix.isEmpty() || !secondarySouthernHemisphereSuffix.isEmpty()
            || !secondaryEasternHemisphereSuffix.isEmpty() || !secondaryWesternHemisphereSuffix.isEmpty());
                
        if (!withPrimary
            || (_gridConfiguration.primaryProjection != GridConfiguration::Projection::UTM
                && _gridConfiguration.primaryProjection != GridConfiguration::Projection::MGRS))
            keepPrimary = true;
        if (!withSecondary
            || (_gridConfiguration.secondaryProjection != GridConfiguration::Projection::UTM
                && _gridConfiguration.secondaryProjection != GridConfiguration::Projection::MGRS))
            keepSecondary = true;
    
        bool zoomChange = (mapZoomLevel < MIN_ZOOM_LEVEL_SIDE_MARKS && _mapZoomLevel >= MIN_ZOOM_LEVEL_SIDE_MARKS)
            || (mapZoomLevel >= MIN_ZOOM_LEVEL_SIDE_MARKS && _mapZoomLevel < MIN_ZOOM_LEVEL_SIDE_MARKS);

        if (zoomChange && !centerPrimaryMarks)
            keepPrimary = false;
        if (zoomChange && !centerSecondaryMarks)
            keepSecondary = false;

        _mapZoomLevel = mapZoomLevel;

        PointD refLons;
        const auto gaps = _gridConfiguration.getCurrentGaps(target31, mapZoomLevel, &refLons);
        QHash<int, PointD> primaryMarksX, primaryMarksY, secondaryMarksX, secondaryMarksY;
        QHash<int, PointD> primaryMarksXExtra, primaryMarksYExtra, secondaryMarksXExtra, secondaryMarksYExtra;
        if (withPrimary)
        {
            calculateGridMarks(true, gaps.x, refLons.x, primaryMarksX, primaryMarksY);
            if (!centerPrimaryMarks && _mapZoomLevel >= MIN_ZOOM_LEVEL_SIDE_MARKS)
            {
                primaryMarksXExtra = primaryMarksX;
                primaryMarksXExtra.detach();
                primaryMarksYExtra = primaryMarksY;
                primaryMarksYExtra.detach();
            }
        }
        if (withSecondary)
        {
            calculateGridMarks(false, gaps.y, refLons.y, secondaryMarksX, secondaryMarksY);
            if (!centerSecondaryMarks && _mapZoomLevel >= MIN_ZOOM_LEVEL_SIDE_MARKS)
            {
                secondaryMarksXExtra = secondaryMarksX;
                secondaryMarksXExtra.detach();
                secondaryMarksYExtra = secondaryMarksY;
                secondaryMarksYExtra.detach();
            }
        }

        QWriteLocker scopedLocker(&_markersLock);

        QSet<int> ids;
        const auto markCount =
            GRID_MARKS_PER_AXIS * ((centerPrimaryMarks || _mapZoomLevel < MIN_ZOOM_LEVEL_SIDE_MARKS ? 2 : 4)
                + (centerSecondaryMarks || _mapZoomLevel < MIN_ZOOM_LEVEL_SIDE_MARKS ? 2 : 4));
        for (int markIdx = 0; markIdx < markCount; markIdx++)
        {
            ids.insert(markIdx);
        }

        QList<std::shared_ptr<MapMarker>> markersToRemove;

        for (const auto& marker : _markersCollection->getMarkers())
        {
            auto coord = static_cast<float>(marker->getAdditionalPosition());
            auto key = *reinterpret_cast<int*>(&coord);
            auto type = marker->getPositionType();
            bool primaryX = type == PositionType::PrimaryGridXMiddle || type == PositionType::PrimaryGridXFirst;
            bool primaryY = type == PositionType::PrimaryGridYMiddle || type == PositionType::PrimaryGridYFirst;
            bool secondaryX = type == PositionType::SecondaryGridXMiddle || type == PositionType::SecondaryGridXFirst;
            bool secondaryY = type == PositionType::SecondaryGridYMiddle || type == PositionType::SecondaryGridYFirst;
            auto value = marker->getAdditionalPosition();
            bool isPresent = true;
            if (keepPrimary && primaryX && primaryMarksX.contains(key))
                primaryMarksX.remove(key);
            else if (keepPrimary && type == PositionType::PrimaryGridXLast && primaryMarksXExtra.contains(key))
                primaryMarksXExtra.remove(key);
            else if (keepPrimary && primaryY && primaryMarksY.contains(key))
                primaryMarksY.remove(key);
            else if (keepPrimary && type == PositionType::PrimaryGridYLast && primaryMarksYExtra.contains(key))
                primaryMarksYExtra.remove(key);
            else if (keepSecondary && secondaryX && secondaryMarksX.contains(key))
                secondaryMarksX.remove(key);
            else if (keepSecondary && type == PositionType::SecondaryGridXLast && secondaryMarksXExtra.contains(key))
                secondaryMarksXExtra.remove(key);
            else if (keepSecondary && secondaryY && secondaryMarksY.contains(key))
                secondaryMarksY.remove(key);
            else if (keepSecondary && type == PositionType::SecondaryGridYLast && secondaryMarksYExtra.contains(key))
                secondaryMarksYExtra.remove(key);
            else
            {
                markersToRemove.append(marker);
                isPresent = false;
            }
            if (isPresent)
            {
                marker->setPosition(target31);
                ids.remove(marker->markerId);
            }
        }

        if (!primaryMarksX.isEmpty())
            addGridMarks(_primaryZone, true, false, false, primaryMarksOffset, primaryMarksX, ids);

        if (!primaryMarksXExtra.isEmpty())
            addGridMarks(_primaryZone, true, false, true, primaryMarksOffset, primaryMarksXExtra, ids);

        if (!primaryMarksY.isEmpty())
            addGridMarks(_primaryZone, true, true, false, primaryMarksOffset, primaryMarksY, ids);

        if (!primaryMarksYExtra.isEmpty())
            addGridMarks(_primaryZone, true, true, true, primaryMarksOffset, primaryMarksYExtra, ids);

        if (!secondaryMarksX.isEmpty())
            addGridMarks(_secondaryZone, false, false, false, secondaryMarksOffset, secondaryMarksX, ids);

        if (!secondaryMarksXExtra.isEmpty())
            addGridMarks(_secondaryZone, false, false, true, secondaryMarksOffset, secondaryMarksXExtra,ids);

        if (!secondaryMarksY.isEmpty())
            addGridMarks(_secondaryZone, false, true, false, secondaryMarksOffset, secondaryMarksY, ids);

        if (!secondaryMarksYExtra.isEmpty())
            addGridMarks(_secondaryZone, false, true, true, secondaryMarksOffset, secondaryMarksYExtra, ids);

        for (const auto& marker : markersToRemove)
        {
            _markersCollection->removeMarker(marker);
        }
    }
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::GridMarksProvider_P::getProvidedDataKeys() const
{
    QReadLocker scopedLocker(&_markersLock);

    return _markersCollection->getProvidedDataKeys();
}

bool OsmAnd::GridMarksProvider_P::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData)
{
    QReadLocker scopedLocker(&_markersLock);

    return _markersCollection->obtainData(request, outData);
}

OsmAnd::ZoomLevel OsmAnd::GridMarksProvider_P::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::GridMarksProvider_P::getMaxZoom() const
{
    return MaxZoomLevel;
}
