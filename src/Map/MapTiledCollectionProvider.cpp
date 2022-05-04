#include "MapTiledCollectionProvider.h"

#include "Utilities.h"
#include "BillboardRasterMapSymbol.h"
#include "MapDataProviderHelpers.h"
#include "LatLon.h"
#include "QRunnableFunctor.h"
#include "MapMarkerBuilder.h"
#include "MapMarkersCollection.h"
#include "SkiaUtilities.h"

static const int kSkipTilesZoom = 13;
static const int kSkipTileDivider = 16;

OsmAnd::MapTiledCollectionProvider::MapTiledCollectionProvider()
{
}

OsmAnd::MapTiledCollectionProvider::~MapTiledCollectionProvider()
{
}

OsmAnd::MapMarker::PinIconVerticalAlignment OsmAnd::MapTiledCollectionProvider::getPinIconVerticalAlignment() const
{
    return OsmAnd::MapMarker::PinIconVerticalAlignment::CenterVertical;
}

OsmAnd::MapMarker::PinIconHorisontalAlignment OsmAnd::MapTiledCollectionProvider::getPinIconHorisontalAlignment() const
{
    return OsmAnd::MapMarker::PinIconHorisontalAlignment::CenterHorizontal;
}

bool OsmAnd::MapTiledCollectionProvider::supportsNaturalObtainData() const
{
    return true;
}

uint32_t OsmAnd::MapTiledCollectionProvider::getTileId(const AreaI& bbox31, const PointI& point)
{
    const auto divX = bbox31.width() / kSkipTileDivider;
    const auto divY = bbox31.height() / kSkipTileDivider;
    const auto tx = static_cast<uint32_t>(floor((point.x - bbox31.left()) / divX));
    const auto ty = static_cast<uint32_t>(floor((point.y - bbox31.top()) / divY));
    return tx + ty * kSkipTileDivider;
}

OsmAnd::AreaD OsmAnd::MapTiledCollectionProvider::calculateRect(double x, double y, double width, double height)
{
    double left = x - width / 2.0;
    double top = y - height / 2.0;
    double right = left + width;
    double bottom = top + height;
    return AreaD(top, left, bottom, right);
}

bool OsmAnd::MapTiledCollectionProvider::intersects(
    CollectionQuadTree& boundIntersections,
    double x,
    double y,
    double width,
    double height)
{
    QList<AreaD> result;
    const auto visibleRect = calculateRect(x, y, width, height);
    boundIntersections.query(visibleRect, result);
    for (const auto& r : result)
        if (r.intersects(visibleRect))
            return true;

    boundIntersections.insert(visibleRect, visibleRect);
    return false;
}

QList<std::shared_ptr<OsmAnd::MapSymbolsGroup>> OsmAnd::MapTiledCollectionProvider::buildMapSymbolsGroups(
    const TileId tileId,
    const ZoomLevel zoom,
    double scale)
{
    QReadLocker scopedLocker(&_lock);

    const auto collection = std::make_shared<MapMarkersCollection>();

    auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
    const auto stepX = tileBBox31.width() / kSkipTileDivider * 4;
    const auto stepY = tileBBox31.height() / kSkipTileDivider * 4;
    auto extendedTileBBox31 = tileBBox31.getEnlargedBy(stepY, stepX, stepY, stepX);

    QSet<uint32_t> skippedTiles;
    bool zoomFilter = zoom <= kSkipTilesZoom;
    const auto tileSize31 = (1u << (ZoomLevel::MaxZoomLevel - zoom));
    const auto from31toPixelsScale = static_cast<double>(getReferenceTileSizeOnScreenInPixels()) / tileSize31;
    CollectionQuadTree boundIntersections(AreaD(tileBBox31).getEnlargedBy(tileBBox31.width() / 2), 4);
    const auto showCaptions = shouldShowCaptions();
    const auto& captionStyle = getCaptionStyle();
    const auto captionTopSpace = getCaptionTopSpace();
    const auto baseOrder = getBaseOrder();
    const auto& hiddenPoints = getHiddenPoints();
    
    int pointsCount = getPointsCount();
    const auto& tilePoints = getTilePoints(tileId, zoom);
    int tilePointsCount = tilePoints.count();
    for (int i = 0; i < pointsCount + tilePointsCount; i++)
    {
        int it = i - pointsCount;
        const auto& data = it >=0 && tilePointsCount > it ? tilePoints[it] : nullptr;
        if (i >= pointsCount && !data)
            continue;
        
        const auto pos31 = i < pointsCount ? getPoint31(i) : data->getPoint31();
        if (extendedTileBBox31.contains(pos31) && !hiddenPoints.contains(pos31))
        {
            if (zoomFilter)
            {
                const auto tileId = getTileId(extendedTileBBox31, pos31);
                if (!skippedTiles.contains(tileId))
                    skippedTiles.insert(tileId);
                else
                    continue;
            }

            // TODO: Would be better to get just bitmap size here
            double estimatedIconSize = 48. * scale;
            const double iconSize31 = estimatedIconSize / from31toPixelsScale;
            bool intr = intersects(boundIntersections, pos31.x, pos31.y, iconSize31, iconSize31);

            if (!tileBBox31.contains(pos31))
                continue;

            MapMarkerBuilder builder;
            builder.setIsAccuracyCircleSupported(false)
                .setBaseOrder(baseOrder)
                .setIsHidden(false)
                .setPosition(pos31)
                .setPinIconVerticalAlignment(getPinIconVerticalAlignment())
                .setPinIconHorisontalAlignment(getPinIconHorisontalAlignment());

            sk_sp<const SkImage> img;
            QString caption;
            if (showCaptions)
                caption = i < pointsCount ? getCaption(i) : data->getCaption();

            if (intr)
            {
                img = i < pointsCount ? getImageBitmap(i, false) : data->getImageBitmap(false);
                builder.setBaseOrder(builder.getBaseOrder() + 1);
            }
            else if (showCaptions && !caption.isEmpty())
            {
                img = i < pointsCount ? getImageBitmap(i) : data->getImageBitmap();
                builder.setCaption(caption);
                builder.setCaptionStyle(captionStyle);
                builder.setCaptionTopSpace(captionTopSpace);
            }
            else
            {
                img = i < pointsCount ? getImageBitmap(i) : data->getImageBitmap();
            }
            builder.setPinIcon(img);
            builder.buildAndAddToCollection(collection);
        }
    }

    QList<std::shared_ptr<MapSymbolsGroup>> mapSymbolsGroups;
    for (const auto& marker : collection->getMarkers())
    {
        const auto mapSymbolGroup = marker->createSymbolsGroup();
        mapSymbolsGroups.push_back(qMove(mapSymbolGroup));
    }
    return mapSymbolsGroups;
}

bool OsmAnd::MapTiledCollectionProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<OsmAnd::Metric>* const pOutMetric)
{
    const auto& req = MapDataProviderHelpers::castRequest<MapTiledCollectionProvider::Request>(request);
    if (pOutMetric)
        pOutMetric->reset();
    
    if (req.zoom > getMaxZoom() || req.zoom < getMinZoom())
    {
        outData.reset();
        return true;
    }
    
    const auto tileId = req.tileId;
    const auto zoom = req.zoom;

    QReadLocker scopedLocker(&_lock);
    
    const auto mapSymbolsGroups = buildMapSymbolsGroups(tileId, zoom, getScale());
    outData.reset(new Data(tileId, zoom, mapSymbolsGroups));
    return true;
}

bool OsmAnd::MapTiledCollectionProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::MapTiledCollectionProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
}

OsmAnd::MapTiledCollectionProvider::Data::Data(
    const OsmAnd::TileId tileId_,
    const OsmAnd::ZoomLevel zoom_,
    const QList< std::shared_ptr<OsmAnd::MapSymbolsGroup> >& symbolsGroups_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledSymbolsProvider::Data(tileId_, zoom_, symbolsGroups_, pRetainableCacheMetadata_)
{
}

OsmAnd::MapTiledCollectionProvider::Data::~Data()
{
    release();
}
