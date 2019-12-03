#ifndef _OSMAND_CORE_ONLINE_TILE_SOURCES_P_H_
#define _OSMAND_CORE_ONLINE_TILE_SOURCES_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QHash>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "MapCommonTypes.h"
#include "OnlineTileSources.h"

namespace OsmAnd
{
    class OnlineTileSources_P Q_DECL_FINAL
    {
    public:
        typedef OnlineTileSources::Source Source;

    private:
        static QList<QString> parseRandoms(const QString &randoms);
    protected:
        OnlineTileSources_P(OnlineTileSources* owner);

        std::shared_ptr<Source> createTileSourceTemplate(const QXmlStreamAttributes &attributes) const;
        std::shared_ptr<Source> createSimpleTileSourceTemplate(const QXmlStreamAttributes &attributes, const QString &rule) const;
        std::shared_ptr<OsmAnd::OnlineTileSources::Source> createWmsTileSourceTemplate(const QXmlStreamAttributes &attributes) const;
        int parseInt(const QXmlStreamAttributes &attributes, QString attributeName, int defaultValue) const;
        long parseLong(const QXmlStreamAttributes &attributes, QString attributeName, long defaultValue) const;
        
        bool deserializeFrom(QXmlStreamReader& xmlReader);
        bool serializeTo(QXmlStreamWriter& xmlWriter) const;

        QHash< QString, std::shared_ptr<const Source> > _collection;
    public:
        virtual ~OnlineTileSources_P();

        ImplementationInterface<OnlineTileSources> owner;

        bool loadFrom(const QByteArray& content);
        bool loadFrom(QIODevice& ioDevice);
        bool saveTo(QIODevice& ioDevice) const;

        QHash< QString, std::shared_ptr<const Source> > getCollection() const;
        std::shared_ptr<const Source> getSourceByName(const QString& sourceName) const;
        bool addSource(const std::shared_ptr<const Source>& source);
        bool removeSource(const QString& sourceName);

        static bool createTileSourceTemplate(const QString& metaInfoPath, std::shared_ptr<Source>& source);
        static void installTileSource(const std::shared_ptr<const OnlineTileSources::Source> toInstall, const QString& cachePath);

    friend class OsmAnd::OnlineTileSources;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_TILE_SOURCES_P_H_)
