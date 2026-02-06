#include "VectorLineArrowsProvider.h"
#include "VectorLineArrowsProvider_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::VectorLineArrowsProvider::VectorLineArrowsProvider(
    const std::shared_ptr<VectorLinesCollection>& collection)
    : _p(new VectorLineArrowsProvider_P(this, collection))
{
    _p->init();
}

OsmAnd::VectorLineArrowsProvider::~VectorLineArrowsProvider()
{
}

void OsmAnd::VectorLineArrowsProvider::removeLineMarkers(int lineId)
{
    _p->removeLineMarkers(lineId);
}

void OsmAnd::VectorLineArrowsProvider::removeAllMarkers()
{
    _p->removeAllMarkers();
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::VectorLineArrowsProvider::getProvidedDataKeys() const
{
    return _p->getProvidedDataKeys();
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
    if (pOutMetric)
        pOutMetric->reset();

    return _p->obtainData(request, outData);
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
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::VectorLineArrowsProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::VectorLineArrowsProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}
