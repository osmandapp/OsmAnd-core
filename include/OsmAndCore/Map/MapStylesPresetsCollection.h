#ifndef _OSMAND_CORE_MAP_STYLES_PRESETS_COLLECTION_H_
#define _OSMAND_CORE_MAP_STYLES_PRESETS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QIODevice>
#include <QByteArray>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapStylesPresetsCollection.h>

namespace OsmAnd
{
    class MapStylesPresetsCollection_P;
    class OSMAND_CORE_API MapStylesPresetsCollection : public IMapStylesPresetsCollection
    {
        Q_DISABLE_COPY(MapStylesPresetsCollection);
    private:
        PrivateImplementation<MapStylesPresetsCollection_P> _p;
    protected:
    public:
        MapStylesPresetsCollection();
        virtual ~MapStylesPresetsCollection();

        bool loadFrom(const QByteArray& content);
        bool loadFrom(QIODevice& ioDevice);
        bool loadFrom(const QString& fileName);
        bool saveTo(QIODevice& ioDevice) const;
        bool saveTo(const QString& fileName) const;

        bool addPreset(const std::shared_ptr<MapStylePreset>& preset);
        bool removePreset(const std::shared_ptr<MapStylePreset>& preset);

        virtual QList< std::shared_ptr<const MapStylePreset> > getCollection() const;
        virtual QList< std::shared_ptr<const MapStylePreset> > getCollectionFor(const QString& styleName) const;
        virtual std::shared_ptr<const MapStylePreset> getPreset(const QString& styleName, const QString& presetName) const;

        QList< std::shared_ptr<MapStylePreset> > getCollection();
        QList< std::shared_ptr<MapStylePreset> > getCollectionFor(const QString& styleName);
        std::shared_ptr<MapStylePreset> getPreset(const QString& styleName, const QString& presetName);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_PRESETS_COLLECTION_H_)
