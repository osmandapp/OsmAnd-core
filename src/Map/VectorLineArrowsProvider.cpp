#include "VectorLineArrowsProvider.h"
#include "VectorLineArrowsProvider_P.h"

#include "MapDataProviderHelpers.h"
#include "VectorLine.h"
#include "VectorLinesCollection.h"
#include "Logging.h"

OsmAnd::VectorLineArrowsProvider::VectorLineArrowsProvider(
    const std::shared_ptr<const VectorLinesCollection>& vectorLinesCollection_)
    : _p(new VectorLineArrowsProvider_P(this))
    , vectorLinesCollection(vectorLinesCollection_)
{
}

OsmAnd::VectorLineArrowsProvider::~VectorLineArrowsProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::VectorLineArrowsProvider::getMinZoom() const
{
    return ZoomLevel6;
}

OsmAnd::ZoomLevel OsmAnd::VectorLineArrowsProvider::getMaxZoom() const
{
    return MaxZoomLevel;
}

bool OsmAnd::VectorLineArrowsProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::VectorLineArrowsProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::VectorLineArrowsProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::VectorLineArrowsProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
}

OsmAnd::VectorLineArrowsProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledSymbolsProvider::Data(tileId_, zoom_, symbolsGroups_, pRetainableCacheMetadata_)
{
}

OsmAnd::VectorLineArrowsProvider::Data::~Data()
{
    release();
}

OsmAnd::VectorLineArrowsProvider::SymbolsGroup::SymbolsGroup(
    const std::shared_ptr<VectorLineArrowsProvider_P>& providerP,
    const std::shared_ptr<VectorLine>& vectorLine)
    : _providerP(providerP)
    , _vectorLine(vectorLine)
{
}

OsmAnd::VectorLineArrowsProvider::SymbolsGroup::~SymbolsGroup()
{
}

bool OsmAnd::VectorLineArrowsProvider::SymbolsGroup::obtainSharingKey(SharingKey& outKey) const
{
    return false;
}

bool OsmAnd::VectorLineArrowsProvider::SymbolsGroup::obtainSortingKey(SortingKey& outKey) const
{
    if (const auto vectorLine = _vectorLine.lock())
    {
        outKey = static_cast<SharingKey>(vectorLine->baseOrder + 1000000);
        return true;
    }
    return false;
}

QString OsmAnd::VectorLineArrowsProvider::SymbolsGroup::toString() const
{
    return {};
}

bool OsmAnd::VectorLineArrowsProvider::SymbolsGroup::updatesPresent()
{
    const auto providerP = _providerP.lock();
    const auto vectorLine = _vectorLine.lock();
    if (providerP && vectorLine)
    {
        int version;
        if (providerP->getVectorLineDataVersion(vectorLine, version))
            return version != vectorLine->getVersion();
    }

    return false;
}

OsmAnd::IUpdatableMapSymbolsGroup::UpdateResult OsmAnd::VectorLineArrowsProvider::SymbolsGroup::update(const MapState& mapState)
{
    return updatesPresent() ? UpdateResult::All : UpdateResult::None;
}

bool OsmAnd::VectorLineArrowsProvider::SymbolsGroup::supportsResourcesRenew()
{
    return false;
}
