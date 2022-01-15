#include "IOnlineTileSources.h"

#include "OnlineRasterMapLayerProvider.h"
#include "Logging.h"

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
    {
        LogPrintf(LogSeverityLevel::Error, "Online tile source \"%s\" not found",
            qPrintable(sourceName)
        );
        return nullptr;
    }
    return std::shared_ptr<OsmAnd::OnlineRasterMapLayerProvider>(new OnlineRasterMapLayerProvider(
        source,
        webClient));
}

OsmAnd::IOnlineTileSources::Source::Source(const QString& name_)
    : name(name_)
    , priority(0)
    , maxZoom(ZoomLevel0)
    , minZoom(ZoomLevel0)
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
