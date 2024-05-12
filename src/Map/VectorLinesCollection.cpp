#include "VectorLinesCollection.h"
#include "VectorLinesCollection_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::VectorLinesCollection::VectorLinesCollection(const bool hasVolumetricSymbols_ /* = false */)
    : _p(new VectorLinesCollection_P(this))
    , hasVolumetricSymbols(hasVolumetricSymbols_)
    , priority(std::numeric_limits<int64_t>::min())
{
}

OsmAnd::VectorLinesCollection::~VectorLinesCollection()
{
}

bool OsmAnd::VectorLinesCollection::isEmpty() const
{
    return _p->isEmpty();
}

bool OsmAnd::VectorLinesCollection::getLinesCount() const
{
    return _p->getLinesCount();
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

const std::shared_ptr<OsmAnd::VectorLineArrowsProvider> OsmAnd::VectorLinesCollection::getVectorLineArrowsProvider()
{
    auto arrowsProvider = _arrowsProvider.lock();
    if (!arrowsProvider)
    {
        arrowsProvider = std::make_shared<VectorLineArrowsProvider>(shared_from_this());
        _arrowsProvider = std::weak_ptr<VectorLineArrowsProvider>(arrowsProvider);
    }

    return arrowsProvider;
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
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::VectorLinesCollection::getMinZoom() const
{
    return OsmAnd::MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::VectorLinesCollection::getMaxZoom() const
{
    return OsmAnd::MaxZoomLevel;
}

int64_t OsmAnd::VectorLinesCollection::getPriority() const
{
    return priority;
}

void OsmAnd::VectorLinesCollection::setPriority(int64_t priority_)
{
    priority = priority_;
}
