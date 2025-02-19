#include "GridMarksProvider_P.h"
#include "GridMarksProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapMarker.h"
#include "MapRenderer.h"

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
{
}

OsmAnd::GridMarksProvider_P::~GridMarksProvider_P()
{
}

void OsmAnd::GridMarksProvider_P::calculateGridMarks(const PointI& target31, const bool isPrimary,
    const double gap, const double refLon, QHash<int, PointD>& marksX, QHash<int, PointD>& marksY)
{
    auto marksCount = static_cast<int>(std::min(static_cast<double>(GRID_MARKS_PER_AXIS),
        isPrimary ? _gridConfiguration.getPrimaryMaxMarksPerAxis(gap)
        : _gridConfiguration.getSecondaryMaxMarksPerAxis(gap)));
    double halfCount = marksCount / 2;
    auto location = (isPrimary ? _gridConfiguration.getPrimaryGridLocation(target31, &refLon)
        : _gridConfiguration.getSecondaryGridLocation(target31, &refLon)) / gap;
    location.x = (std::round(location.x) - halfCount) * gap;
    location.y = (std::round(location.y) - halfCount) * gap;
    for (int i = 0; i < marksCount; i++)
    {
        auto locationN = location;
        bool withX = isPrimary ? _gridConfiguration.getPrimaryGridCoordinateX(locationN.x)
            : _gridConfiguration.getSecondaryGridCoordinateX(locationN.x);
        bool withY = isPrimary ? _gridConfiguration.getPrimaryGridCoordinateY(locationN.y)
            : _gridConfiguration.getSecondaryGridCoordinateY(locationN.y);
        PointF coord(static_cast<float>(locationN.x), static_cast<float>(locationN.y));
        if (withX)
            marksX.insert(*reinterpret_cast<int*>(&coord.x), locationN);
        if (withY)
            marksY.insert(*reinterpret_cast<int*>(&coord.y), locationN);
        location.x += gap;
        location.y += gap;
    }
}

void OsmAnd::GridMarksProvider_P::addGridMarks(const PointI& target31, const int zone, const bool isPrimary,
    const bool isAxisY, const float offset, QHash<int, PointD>& marks, QSet<int>& availableIds)
{
    auto type = isPrimary ? (isAxisY ? MapMarker::PositionType::PrimaryGridY : MapMarker::PositionType::PrimaryGridX)
        : (isAxisY ? MapMarker::PositionType::SecondaryGridY : MapMarker::PositionType::SecondaryGridX);

    OsmAnd::MapMarkerBuilder builder;
    builder.setBaseOrder(std::numeric_limits<int>::min() + (isPrimary ? 10 : 20));
    builder.setPosition(target31);
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
        builder.setCaptionTopSpace(isAxisY ? -offset : offset);
        const auto newMarker = builder.buildAndAddToCollection(_markersCollection);
        newMarker->setPositionType(type);
        newMarker->setAdditionalPosition(isAxisY ? coordinates.y : coordinates.x);
        availableIds.erase(itId);
    }
}

void OsmAnd::GridMarksProvider_P::setPrimaryStyle(const TextRasterizer::Style& style, const float offset)
{
    primaryStyle = style;
    primaryMarksOffset = offset;
}

void OsmAnd::GridMarksProvider_P::setSecondaryStyle(const TextRasterizer::Style& style, const float offset)
{
    secondaryStyle = style;
    secondaryMarksOffset = offset;
}

void OsmAnd::GridMarksProvider_P::setPrimary(const bool withValues, const QString& northernSuffix,
    const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix)
{
    withPrimaryValues = withValues;
    primaryNorthernHemisphereSuffix = northernSuffix;
    primarySouthernHemisphereSuffix = southernSuffix;
    primaryEasternHemisphereSuffix = easternSuffix;
    primaryWesternHemisphereSuffix = westernSuffix;
}

void OsmAnd::GridMarksProvider_P::setSecondary(const bool withValues, const QString& northernSuffix,
    const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix)
{
    withSecondaryValues = withValues;
    secondaryNorthernHemisphereSuffix = northernSuffix;
    secondarySouthernHemisphereSuffix = southernSuffix;
    secondaryEasternHemisphereSuffix = easternSuffix;
    secondaryWesternHemisphereSuffix = westernSuffix;
}

void OsmAnd::GridMarksProvider_P::applyMapChanges(IMapRenderer* renderer)
{
    AreaI visibleBBoxShifted;
    auto mapZoomLevel = renderer->getVisibleArea(&visibleBBoxShifted);
    bool changed = _mapZoomLevel != mapZoomLevel;
    if (!changed && _visibleBBoxShifted != visibleBBoxShifted)
    {
        const AreaI64 visibleBBoxShifted64(_visibleBBoxShifted);
        auto shTL = visibleBBoxShifted64.topLeft - visibleBBoxShifted.topLeft;
        auto shBR = visibleBBoxShifted64.bottomRight - visibleBBoxShifted.bottomRight;
        auto lim = PointI64(visibleBBoxShifted64.width(), visibleBBoxShifted64.height()) / MAP_CHANGE_FACTOR_PART;
        changed = abs(shTL.x) > lim.x || abs(shTL.y) > lim.y || abs(shBR.x) > lim.x || abs(shBR.y) > lim.y;
    }

    if (changed)
    {
        _visibleBBoxShifted = visibleBBoxShifted;
    
        PointI target31;
        renderer->getGridConfiguration(&_gridConfiguration, &target31, &mapZoomLevel);
        _mapZoomLevel = mapZoomLevel;

        const bool withPrimary = mapZoomLevel >= _gridConfiguration.gridParameters[0].minZoom && (withPrimaryValues
            || !primaryNorthernHemisphereSuffix.isEmpty() || !primarySouthernHemisphereSuffix.isEmpty()
            || !primaryEasternHemisphereSuffix.isEmpty() || !primaryWesternHemisphereSuffix.isEmpty());
        const bool withSecondary = mapZoomLevel >= _gridConfiguration.gridParameters[1].minZoom && (withSecondaryValues
            || !secondaryNorthernHemisphereSuffix.isEmpty() || !secondarySouthernHemisphereSuffix.isEmpty()
            || !secondaryEasternHemisphereSuffix.isEmpty() || !secondaryWesternHemisphereSuffix.isEmpty());
                
        int primaryZone = 0;
        int secondaryZone = 0;
        if (withPrimary && _gridConfiguration.primaryProjection == GridConfiguration::Projection::UTM)
            primaryZone = Utilities::getCodedZoneUTM(target31, false);
        if (withSecondary && _gridConfiguration.secondaryProjection == GridConfiguration::Projection::UTM)
            secondaryZone = primaryZone == 0 ? Utilities::getCodedZoneUTM(target31, false) : primaryZone;
        const bool keepPrimary = primaryZone == _primaryZone;
        const bool keepSecondary = secondaryZone == _secondaryZone;
        
        PointD refLons;
        const auto gaps = _gridConfiguration.getCurrentGaps(target31, mapZoomLevel, &refLons);
        QHash<int, PointD> primaryMarksX, primaryMarksY, secondaryMarksX, secondaryMarksY;
        if (withPrimary)
            calculateGridMarks(target31, true, gaps.x, refLons.x, primaryMarksX, primaryMarksY);
        if (withSecondary)
            calculateGridMarks(target31, false, gaps.y, refLons.y, secondaryMarksX, secondaryMarksY);

        QWriteLocker scopedLocker(&_markersLock);

        QSet<int> availableIds;
        for (int markIdx = 0; markIdx < GRID_MARKS_PER_AXIS * 4; markIdx++)
        {
            availableIds.insert(markIdx);
        }

        QList<std::shared_ptr<MapMarker>> markersToRemove;

        for (const auto& marker : _markersCollection->getMarkers())
        {
            auto coord = static_cast<float>(marker->getAdditionalPosition());
            auto key = *reinterpret_cast<int*>(&coord);
            auto type = marker->getPositionType();
            auto value = marker->getAdditionalPosition();
            bool isPresent = true;
            if (keepPrimary && type == MapMarker::PositionType::PrimaryGridX && primaryMarksX.contains(key))
                primaryMarksX.remove(key);
            else if (keepPrimary && type == MapMarker::PositionType::PrimaryGridY && primaryMarksY.contains(key))
                primaryMarksY.remove(key);
            else if (keepSecondary && type == MapMarker::PositionType::SecondaryGridX && secondaryMarksX.contains(key))
                secondaryMarksX.remove(key);
            else if (keepSecondary && type == MapMarker::PositionType::SecondaryGridY && secondaryMarksY.contains(key))
                secondaryMarksY.remove(key);
            else
            {
                markersToRemove.append(marker);
                isPresent = false;
            }
            if (isPresent)
            {
                marker->setPosition(target31);
                availableIds.remove(marker->markerId);
            }
        }

        if (!primaryMarksX.isEmpty())
            addGridMarks(target31, primaryZone, true, false, primaryMarksOffset, primaryMarksX, availableIds);

        if (!primaryMarksY.isEmpty())
            addGridMarks(target31, primaryZone, true, true, primaryMarksOffset, primaryMarksY, availableIds);

        if (!secondaryMarksX.isEmpty())
            addGridMarks(target31, secondaryZone, false, false, primaryMarksOffset, secondaryMarksX, availableIds);

        if (!secondaryMarksY.isEmpty())
            addGridMarks(target31, secondaryZone, false, true, primaryMarksOffset, secondaryMarksY, availableIds);

        for (const auto& marker : markersToRemove)
        {
            _markersCollection->removeMarker(marker);
        }

        _primaryZone = primaryZone;
        _secondaryZone = secondaryZone;
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
