#ifndef _OSMAND_CORE_MAP_STYLES_COLLECTION_H_
#define _OSMAND_CORE_MAP_STYLES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapStylesCollection.h>

namespace OsmAnd
{
    class MapStylesCollection_P;
    class OSMAND_CORE_API MapStylesCollection : public IMapStylesCollection
    {
        Q_DISABLE_COPY_AND_MOVE(MapStylesCollection);
    private:
        PrivateImplementation<MapStylesCollection_P> _p;
    protected:
    public:
        MapStylesCollection();
        virtual ~MapStylesCollection();

        bool registerStyle(const QString& filePath);

        virtual QList< std::shared_ptr<const MapStyle> > getCollection() const;
        virtual std::shared_ptr<const MapStyle> getAsIsStyle(const QString& name) const;
        virtual bool obtainBakedStyle(const QString& name, std::shared_ptr<const MapStyle>& outStyle) const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_COLLECTION_H_)
