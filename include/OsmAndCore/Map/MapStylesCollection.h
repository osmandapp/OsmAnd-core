#ifndef _OSMAND_CORE_MAP_STYLES_COLLECTION_H_
#define _OSMAND_CORE_MAP_STYLES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapStylesCollection.h>

namespace OsmAnd
{
    class MapStylesCollection_P;
    class OSMAND_CORE_API MapStylesCollection : public IMapStylesCollection
    {
        Q_DISABLE_COPY_AND_MOVE(MapStylesCollection)
    private:
        PrivateImplementation<MapStylesCollection_P> _p;
    protected:
    public:
        MapStylesCollection();
        virtual ~MapStylesCollection();

        bool addStyleFromFile(const QString& filePath, const bool doNotReplace = false);
        bool addStyleFromByteArray(const QByteArray& data, const QString& name, const bool doNotReplace = false);

        virtual QList< std::shared_ptr<const UnresolvedMapStyle> > getCollection() const;
        virtual std::shared_ptr<const UnresolvedMapStyle> getStyleByName(const QString& name) const;
        virtual std::shared_ptr<const ResolvedMapStyle> getResolvedStyleByName(const QString& name) const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_COLLECTION_H_)
