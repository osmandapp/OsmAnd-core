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
#include "MapTypes.h"
#include "OnlineTileSources.h"

namespace OsmAnd
{
    class OnlineTileSources_P
    {
    public:
        typedef OnlineTileSources::Source Source;

    private:
        static std::shared_ptr<OnlineTileSources> _builtIn;
    protected:
        OnlineTileSources_P(OnlineTileSources* owner);

        bool loadFrom(QXmlStreamReader& xmlReader);
        bool saveTo(QXmlStreamWriter& xmlWriter) const;

        QHash< QString, std::shared_ptr<Source> > _collection;
    public:
        virtual ~OnlineTileSources_P();

        ImplementationInterface<OnlineTileSources> owner;

        bool loadFrom(const QByteArray& content);
        bool loadFrom(QIODevice& ioDevice);
        bool saveTo(QIODevice& ioDevice) const;

        QList< std::shared_ptr<Source> > getCollection() const;
        std::shared_ptr<Source> getSourceByName(const QString& sourceName) const;
        bool addSource(const std::shared_ptr<Source>& source);
        bool removeSource(const QString& sourceName);

        std::shared_ptr<OnlineMapRasterTileProvider> createProviderFor(const QString& sourceId) const;

        static std::shared_ptr<const OnlineTileSources> getBuiltIn();

    friend class OsmAnd::OnlineTileSources;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_TILE_SOURCES_P_H_)
