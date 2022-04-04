#include "VectorLinesCollection.h"
#include "VectorLinesCollection_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::VectorLinesCollection::VectorLinesCollection()
    : _p(new VectorLinesCollection_P(this))
{
}

OsmAnd::VectorLinesCollection::~VectorLinesCollection()
{
}

QList<std::shared_ptr<OsmAnd::VectorLine>> OsmAnd::VectorLinesCollection::getLines() const
{
    return _p->getLines();
}

bool OsmAnd::VectorLinesCollection::removeLine(const std::shared_ptr<VectorLine>& line)
{
    return _p->removeLine(line);
}

void OsmAnd::VectorLinesCollection::removeAllLines()
{
    _p->removeAllLines();
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::VectorLinesCollection::getProvidedDataKeys() const
{
    return _p->getProvidedDataKeys();
}

bool OsmAnd::VectorLinesCollection::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::VectorLinesCollection::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();

    return _p->obtainData(request, outData);
}

bool OsmAnd::VectorLinesCollection::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::VectorLinesCollection::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::VectorLinesCollection::getMinZoom() const
{
    return OsmAnd::MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::VectorLinesCollection::getMaxZoom() const
{
    return OsmAnd::MaxZoomLevel;
}

std::shared_ptr<OsmAnd::VectorLineArrowsProvider> OsmAnd::VectorLinesCollection::createArrowsProvider()
{
    return std::make_shared<VectorLineArrowsProvider>(shared_from_this());
}
