#ifndef _OSMAND_CORE_MAP_STYLES_PRESETS_H_
#define _OSMAND_CORE_MAP_STYLES_PRESETS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QIODevice>
#include <QByteArray>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapStylePreset.h>

namespace OsmAnd
{
    class MapStylesPresets_P;
    class OSMAND_CORE_API MapStylesPresets
    {
        Q_DISABLE_COPY(MapStylesPresets);
    private:
        PrivateImplementation<MapStylesPresets_P> _p;
    protected:
    public:
        MapStylesPresets();
        virtual ~MapStylesPresets();

        bool loadFrom(const QByteArray& content);
        bool loadFrom(QIODevice& ioDevice);
        bool loadFrom(const QString& fileName);
        bool saveTo(QIODevice& ioDevice) const;
        bool saveTo(const QString& fileName) const;

        QHash< QString, std::shared_ptr<MapStylePreset> > getCollection() const;
        std::shared_ptr<MapStylePreset> getPresetByName(const QString& name) const;
        bool addPreset(const std::shared_ptr<MapStylePreset>& preset);
        bool removePreset(const QString& name);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_PRESETS_H_)
