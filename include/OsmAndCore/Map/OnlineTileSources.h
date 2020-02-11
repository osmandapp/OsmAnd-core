#ifndef _OSMAND_CORE_ONLINE_TILE_SOURCES_H_
#define _OSMAND_CORE_ONLINE_TILE_SOURCES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QIODevice>
#include <QXmlStreamAttributes>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/IOnlineTileSources.h>

namespace OsmAnd
{
    class OnlineRasterMapLayerProvider;

    class OnlineTileSources_P;
    class OSMAND_CORE_API OnlineTileSources : public IOnlineTileSources
    {
    public:
        typedef IOnlineTileSources::Source Source;

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

        virtual QHash< QString, std::shared_ptr<const Source> > getCollection() const;
        virtual std::shared_ptr<const Source> getSourceByName(const QString& sourceName) const;
        bool addSource(const std::shared_ptr<const Source>& source);
        bool removeSource(const QString& sourceName);

        static const QString BuiltInOsmAndHD;
        
        static const QString normalizeUrl(QString &url);
        
        static bool createTileSourceTemplate(const QString& metaInfoPath, std::shared_ptr<Source>& source);
        static void installTileSource(const std::shared_ptr<const Source> toInstall, const QString& cachePath);
        static std::shared_ptr<const OnlineTileSources> getBuiltIn();
        static std::shared_ptr<Source> createTileSourceTemplate(const QXmlStreamAttributes &attributes);
        
        static QList<QString> parseRandoms(const QString &randoms);
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_TILE_SOURCES_H_)
