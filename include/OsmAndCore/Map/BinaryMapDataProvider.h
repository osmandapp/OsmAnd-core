#ifndef _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_H_
#define _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTiledDataProvider.h>

namespace OsmAnd
{
    class IObfsCollection;
    class MapStyle;
    class RasterizerEnvironment;
    class RasterizerContext;
    class RasterizerSharedContext;
    class BinaryMapDataTile;
    class IExternalResourcesProvider;
    namespace Model
    {
        class MapObject;
    }

    class BinaryMapDataProvider_P;
    class OSMAND_CORE_API BinaryMapDataProvider : public IMapTiledDataProvider
    {
        Q_DISABLE_COPY(BinaryMapDataProvider);
    private:
        PrivateImplementation<BinaryMapDataProvider_P> _p;
    protected:
    public:
        BinaryMapDataProvider(
            const std::shared_ptr<const IObfsCollection>& obfsCollection,
            const std::shared_ptr<const MapStyle>& mapStyle,
            const float displayDensityFactor,
            const QString& localeLanguageId = QLatin1String("en"),
            const std::shared_ptr<const IExternalResourcesProvider>& externalResourcesProvider = nullptr);
        virtual ~BinaryMapDataProvider();

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        const std::shared_ptr<const MapStyle> mapStyle;
        const std::shared_ptr<RasterizerEnvironment> rasterizerEnvironment;
        const std::shared_ptr<RasterizerSharedContext> rasterizerSharedContext;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<const MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr);
    };

    class BinaryMapDataTile_P;
    class OSMAND_CORE_API BinaryMapDataTile : public MapTiledData
    {
        Q_DISABLE_COPY(BinaryMapDataTile);
    private:
        PrivateImplementation<BinaryMapDataTile_P> _p;
    protected:
        BinaryMapDataTile(
            const MapFoundationType tileFoundation,
            const QList< std::shared_ptr<const Model::MapObject> >& mapObjects,
            const std::shared_ptr< const RasterizerContext >& rasterizerContext,
            const bool nothingToRasterize,
            const TileId tileId,
            const ZoomLevel zoom);
    public:
        virtual ~BinaryMapDataTile();

        const MapFoundationType tileFoundation;
        const QList< std::shared_ptr<const Model::MapObject> >& mapObjects;

        const std::shared_ptr< const RasterizerContext >& rasterizerContext;

        const bool nothingToRasterize;

    friend class OsmAnd::BinaryMapDataProvider;
    friend class OsmAnd::BinaryMapDataProvider_P;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_H_)
