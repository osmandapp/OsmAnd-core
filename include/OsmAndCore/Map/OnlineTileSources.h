#ifndef _OSMAND_CORE_ONLINE_TILE_SOURCES_H_
#define _OSMAND_CORE_ONLINE_TILE_SOURCES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QIODevice>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd
{
    class OnlineMapRasterTileProvider;

    class OnlineTileSources_P;
    class OSMAND_CORE_API OnlineTileSources
    {
    public:
        struct OSMAND_CORE_API Source
        {
            Source(const QString& name);
            virtual ~Source();

            const QString name;
            QString urlPattern;
            ZoomLevel minZoom;
            ZoomLevel maxZoom;
            unsigned int maxConcurrentDownloads;
            unsigned int tileSize;
            AlphaChannelData alphaChannelData;
        };

    private:
        PrivateImplementation<OnlineTileSources_P> _p;
    protected:
    public:
        OnlineTileSources();
        virtual ~OnlineTileSources();

        bool loadFrom(const QByteArray& content);
        bool loadFrom(QIODevice& ioDevice);
        bool loadFrom(const QString& fileName);
        bool saveTo(QIODevice& ioDevice) const;
        bool saveTo(const QString& fileName) const;

        QList< std::shared_ptr<Source> > getCollection() const;
        std::shared_ptr<Source> getSourceByName(const QString& sourceName) const;
        bool addSource(const std::shared_ptr<Source>& source);
        bool removeSource(const QString& sourceName);

        std::shared_ptr<OnlineMapRasterTileProvider> createProviderFor(const QString& sourceName) const;

        static std::shared_ptr<const OnlineTileSources> getBuiltIn();
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_TILE_SOURCES_H_)
