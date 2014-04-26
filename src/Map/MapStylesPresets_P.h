#ifndef _OSMAND_CORE_MAP_STYLES_PRESETS_P_H_
#define _OSMAND_CORE_MAP_STYLES_PRESETS_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QHash>
#include <QString>
#include <QIODevice>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"

namespace OsmAnd
{
    class MapStylePreset;

    class MapStylesPresets;
    class MapStylesPresets_P
    {
        Q_DISABLE_COPY(MapStylesPresets_P);
    private:
    protected:
        MapStylesPresets_P(MapStylesPresets* const owner);

        bool deserializeFrom(QXmlStreamReader& xmlReader);
        bool serializeTo(QXmlStreamWriter& xmlWriter) const;

        QHash< QString, std::shared_ptr<MapStylePreset> > _collection;
    public:
        virtual ~MapStylesPresets_P();

        ImplementationInterface<MapStylesPresets> owner;

        bool loadFrom(const QByteArray& content);
        bool loadFrom(QIODevice& ioDevice);
        bool saveTo(QIODevice& ioDevice) const;

        QHash< QString, std::shared_ptr<MapStylePreset> > getCollection() const;
        std::shared_ptr<MapStylePreset> getPresetByName(const QString& name) const;
        bool addPreset(const std::shared_ptr<MapStylePreset>& preset);
        bool removePreset(const QString& name);

    friend class OsmAnd::MapStylesPresets;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_PRESETS_P_H_)
