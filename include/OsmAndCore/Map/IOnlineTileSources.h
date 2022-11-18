#ifndef _OSMAND_CORE_I_ONLINE_TILE_SOURCES_H_
#define _OSMAND_CORE_I_ONLINE_TILE_SOURCES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QHash>
#include <QString>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/WebClient.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

namespace OsmAnd
{
    class OnlineRasterMapLayerProvider;

    class OSMAND_CORE_API IOnlineTileSources
    {
    public:
        struct OSMAND_CORE_API Source
        {
            Source(const QString& name);
            virtual ~Source();
            
            const QString name;
            int priority;
            ZoomLevel maxZoom;
            ZoomLevel minZoom;
            unsigned int tileSize;
            QString urlToLoad;
            QString ext;
            QString referer;
            QString userAgent;
            unsigned int avgSize;
            unsigned int bitDensity;
            // -1 never expires,
            long expirationTimeMillis;
            bool ellipticYTile;
            bool invertedYTile;
            QString randoms;
            QList<QString> randomsArray;
            QString rule;
            bool hidden; // if hidden in configure map settings, for example mapillary sources

        private:
            Q_DISABLE_COPY_AND_MOVE(Source);
        };

    private:
    protected:
        IOnlineTileSources();
    public:
        virtual ~IOnlineTileSources();

        virtual QHash< QString, std::shared_ptr<const Source> > getCollection() const = 0;
        virtual std::shared_ptr<const Source> getSourceByName(const QString& sourceName) const = 0;

        virtual std::shared_ptr<OnlineRasterMapLayerProvider> createProviderFor(
            const QString& sourceName,
            const std::shared_ptr<const IWebClient>& webClient = std::shared_ptr<const IWebClient>(new WebClient())) const;
    };
}

#endif // !defined(_OSMAND_CORE_I_ONLINE_TILE_SOURCES_H_)
