#ifndef _OSMAND_CORE_SQLITE_HEIGHTMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_SQLITE_HEIGHTMAP_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QDir>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapElevationDataProvider.h>

namespace OsmAnd
{
    class ITileSqliteDatabasesCollection;
    class IGeoTiffCollection;

    class SqliteHeightmapTileProvider_P;
    class OSMAND_CORE_API SqliteHeightmapTileProvider
        : public std::enable_shared_from_this<SqliteHeightmapTileProvider>
        , public IMapElevationDataProvider
    {
        Q_DISABLE_COPY_AND_MOVE(SqliteHeightmapTileProvider);

    private:
        PrivateImplementation<SqliteHeightmapTileProvider_P> _p;
    protected:
    public:
        SqliteHeightmapTileProvider();
        SqliteHeightmapTileProvider(
            const std::shared_ptr<const ITileSqliteDatabasesCollection>& sourcesCollection,
            uint32_t outputTileSize
        );
        SqliteHeightmapTileProvider(
            const std::shared_ptr<const IGeoTiffCollection>& filesCollection,
            uint32_t outputTileSize
        );
        SqliteHeightmapTileProvider(
            const std::shared_ptr<const ITileSqliteDatabasesCollection>& sourcesCollection,
            const std::shared_ptr<const IGeoTiffCollection>& filesCollection,
            uint32_t outputTileSize
        );
        virtual ~SqliteHeightmapTileProvider();

        const std::shared_ptr<const ITileSqliteDatabasesCollection> sourcesCollection;
        const std::shared_ptr<const IGeoTiffCollection> filesCollection;
        const uint32_t outputTileSize;

        virtual bool hasDataResources() const Q_DECL_OVERRIDE;

        virtual ZoomLevel getMinZoom() const Q_DECL_OVERRIDE;
        virtual ZoomLevel getMaxZoom() const Q_DECL_OVERRIDE;
        virtual int getMaxMissingDataZoomShift() const Q_DECL_OVERRIDE;

        virtual uint32_t getTileSize() const Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainData() const Q_DECL_OVERRIDE;
        virtual bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric = nullptr) Q_DECL_OVERRIDE;

        virtual bool supportsNaturalObtainDataAsync() const Q_DECL_OVERRIDE;
        virtual void obtainDataAsync(
            const IMapDataProvider::Request& request,
            const IMapDataProvider::ObtainDataAsyncCallback callback,
            const bool collectMetric = false) Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_SQLITE_HEIGHTMAP_TILE_PROVIDER_H_)
