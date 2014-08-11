#ifndef _OSMAND_CORE_MAP_STYLES_PRESETS_COLLECTION_P_H_
#define _OSMAND_CORE_MAP_STYLES_PRESETS_COLLECTION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QHash>
#include <QList>
#include <QString>
#include <QIODevice>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    class MapStylePreset;

    class MapStylesPresetsCollection;
    class MapStylesPresetsCollection_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapStylesPresetsCollection_P);
    private:
    protected:
        MapStylesPresetsCollection_P(MapStylesPresetsCollection* const owner);

        bool deserializeFrom(QXmlStreamReader& xmlReader);
        bool serializeTo(QXmlStreamWriter& xmlWriter) const;

        QList< std::shared_ptr<MapStylePreset> > _order;
        QHash< QString, QHash< QString, std::shared_ptr<MapStylePreset> > > _collection;
    public:
        virtual ~MapStylesPresetsCollection_P();

        ImplementationInterface<MapStylesPresetsCollection> owner;

        bool loadFrom(const QByteArray& content);
        bool loadFrom(QIODevice& ioDevice);
        bool saveTo(QIODevice& ioDevice) const;

        bool addPreset(const std::shared_ptr<MapStylePreset>& preset);
        bool removePreset(const std::shared_ptr<MapStylePreset>& preset);

        QList< std::shared_ptr<const MapStylePreset> > getCollection() const;
        QList< std::shared_ptr<const MapStylePreset> > getCollectionFor(const QString& styleName) const;
        std::shared_ptr<const MapStylePreset> getPreset(const QString& styleName, const QString& presetName) const;

        QList< std::shared_ptr<MapStylePreset> > getCollection();
        QList< std::shared_ptr<MapStylePreset> > getCollectionFor(const QString& styleName);
        std::shared_ptr<MapStylePreset> getPreset(const QString& styleName, const QString& presetName);

    friend class OsmAnd::MapStylesPresetsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_PRESETS_COLLECTION_P_H_)
