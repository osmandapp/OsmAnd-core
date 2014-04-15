#ifndef _OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDERS_DB_H_
#define _OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDERS_DB_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd
{
    class OnlineMapRasterTileProvider;

    class OnlineMapRasterTileProvidersDB_P;
    class OSMAND_CORE_API OnlineMapRasterTileProvidersDB
    {
    public:
        struct Entry
        {
            QString id;
            QString name;
            QString urlPattern;
            ZoomLevel minZoom;
            ZoomLevel maxZoom;
            unsigned int maxConcurrentDownloads;
            unsigned int tileSize;
            AlphaChannelData alphaChannelData;
        };

    private:
        PrivateImplementation<OnlineMapRasterTileProvidersDB_P> _p;
    protected:
        OnlineMapRasterTileProvidersDB();
    public:
        virtual ~OnlineMapRasterTileProvidersDB();

        const QHash< QString, Entry >& entries;

        bool add(const Entry& entry);
        bool remove(const QString& id);

        bool saveTo(const QString& filePath) const;

        std::shared_ptr<OnlineMapRasterTileProvider> createProvider(const QString& providerId) const;

        static std::shared_ptr<OnlineMapRasterTileProvidersDB> createDefaultDB();
        static std::shared_ptr<OnlineMapRasterTileProvidersDB> loadFrom(const QString& filePath);

        friend class OsmAnd::OnlineMapRasterTileProvidersDB_P;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDERS_DB_H_)
