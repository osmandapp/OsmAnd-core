#include "GridMarksProvider.h"
#include "GridMarksProvider_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::GridMarksProvider::GridMarksProvider()
    : _p(new GridMarksProvider_P(this))
{
}

OsmAnd::GridMarksProvider::~GridMarksProvider()
{
}

void OsmAnd::GridMarksProvider::setPrimaryStyle(const TextRasterizer::Style& style, const float offset,
    bool center /* = false */)
{
    _p->setPrimaryStyle(style, offset, center);
}

void OsmAnd::GridMarksProvider::setSecondaryStyle(const TextRasterizer::Style& style, const float offset,
    bool center /* = false */)
{
    _p->setSecondaryStyle(style, offset, center);
}

void OsmAnd::GridMarksProvider::setPrimary(const bool withValues, const QString& northernSuffix,
    const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix)
{
    _p->setPrimary(withValues, northernSuffix, southernSuffix, easternSuffix, westernSuffix);
}

void OsmAnd::GridMarksProvider::setSecondary(const bool withValues, const QString& northernSuffix,
    const QString& southernSuffix, const QString& easternSuffix, const QString& westernSuffix)
{
    _p->setSecondary(withValues, northernSuffix, southernSuffix, easternSuffix, westernSuffix);
}

void OsmAnd::GridMarksProvider::applyMapChanges(IMapRenderer* renderer)
{
    _p->applyMapChanges(renderer);
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::GridMarksProvider::getProvidedDataKeys() const
{
    return _p->getProvidedDataKeys();
}

bool OsmAnd::GridMarksProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::GridMarksProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();

    return _p->obtainData(request, outData);
}

bool OsmAnd::GridMarksProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::GridMarksProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::GridMarksProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::GridMarksProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

int64_t OsmAnd::GridMarksProvider::getPriority() const
{
    return std::numeric_limits<int64_t>::max();
}
