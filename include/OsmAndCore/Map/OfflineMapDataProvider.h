#ifndef _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_H_
#define _OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    class IObfsCollection;
    class MapStyle;
    class RasterizerEnvironment;
    class RasterizerSharedContext;
    class OfflineMapDataTile;
    class IExternalResourcesProvider;

    class OfflineMapDataProvider_P;
    class OSMAND_CORE_API OfflineMapDataProvider
    {
        Q_DISABLE_COPY(OfflineMapDataProvider);
    private:
        PrivateImplementation<OfflineMapDataProvider_P> _p;
    protected:
    public:
        OfflineMapDataProvider(
            const std::shared_ptr<const IObfsCollection>& obfsCollection,
            const std::shared_ptr<const MapStyle>& mapStyle,
            const float displayDensityFactor,
            const QString& localeLanguageId = QLatin1String("en"),
            const std::shared_ptr<const IExternalResourcesProvider>& externalResourcesProvider = nullptr);
        virtual ~OfflineMapDataProvider();

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        const std::shared_ptr<const MapStyle> mapStyle;
        const std::shared_ptr<RasterizerEnvironment> rasterizerEnvironment;
        const std::shared_ptr<RasterizerSharedContext> rasterizerSharedContext;

        void obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile) const;
    };
}

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_DATA_PROVIDER_H_)
