#include "IOnlineTileSources.h"

#include "OnlineRasterMapLayerProvider.h"

OsmAnd::IOnlineTileSources::IOnlineTileSources()
{
}

OsmAnd::IOnlineTileSources::~IOnlineTileSources()
{
}

std::shared_ptr<OsmAnd::OnlineRasterMapLayerProvider> OsmAnd::IOnlineTileSources::createProviderFor(
    const QString& sourceName,
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/) const
{
    const auto source = getSourceByName(sourceName);
    if (!source)
        return nullptr;
    return std::shared_ptr<OsmAnd::OnlineRasterMapLayerProvider>(new OnlineRasterMapLayerProvider(
        source,
        webClient));
}

OsmAnd::IOnlineTileSources::Source::Source(const QString& name_)
: maxZoom(ZoomLevel0)
, minZoom(ZoomLevel0)
, name(name_)
, tileSize(0)
, avgSize(0)
, bitDensity(0)
, expirationTimeMillis(-1)
, ellipticYTile(false)
, invertedYTile(false)
, hidden(false)
{
}

OsmAnd::IOnlineTileSources::Source::~Source()
{
}
