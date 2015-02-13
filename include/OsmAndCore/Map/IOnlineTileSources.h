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
            Source(const QString& name, const QString& title);
            virtual ~Source();

            const QString name;
            QString title;
            QString urlPattern;
            ZoomLevel minZoom;
            ZoomLevel maxZoom;
            unsigned int maxConcurrentDownloads;
            unsigned int tileSize;
            AlphaChannelPresence alphaChannelPresence;

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

        virtual std::shared_ptr<OnlineRasterMapLayerProvider> createProviderFor(const QString& sourceName) const;
    };
}

#endif // !defined(_OSMAND_CORE_I_ONLINE_TILE_SOURCES_H_)
