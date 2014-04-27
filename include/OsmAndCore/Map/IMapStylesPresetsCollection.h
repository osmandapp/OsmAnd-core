#ifndef _OSMAND_CORE_I_MAP_STYLES_PRESETS_COLLECTION_H_
#define _OSMAND_CORE_I_MAP_STYLES_PRESETS_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class MapStylePreset;

    class OSMAND_CORE_API IMapStylesPresetsCollection
    {
        Q_DISABLE_COPY(IMapStylesPresetsCollection);
    private:
    protected:
        IMapStylesPresetsCollection();
    public:
        virtual ~IMapStylesPresetsCollection();

        virtual QList< std::shared_ptr<const MapStylePreset> > getCollection() const = 0;
        virtual QList< std::shared_ptr<const MapStylePreset> > getCollectionFor(const QString& styleName) const = 0;
        virtual std::shared_ptr<const MapStylePreset> getPreset(const QString& styleName, const QString& presetName) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_STYLES_PRESETS_COLLECTION_H_)
