#ifndef _OSMAND_CORE_I_MAP_STYLES_COLLECTION_H_
#define _OSMAND_CORE_I_MAP_STYLES_COLLECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QList>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class MapStyle;

    class OSMAND_CORE_API IMapStylesCollection
    {
        Q_DISABLE_COPY(IMapStylesCollection);
    private:
    protected:
        IMapStylesCollection();
    public:
        virtual ~IMapStylesCollection();

        virtual QList< std::shared_ptr<const MapStyle> > getCollection() const = 0;
        virtual std::shared_ptr<const MapStyle> getAsIsStyle(const QString& name) const = 0;
        virtual bool obtainBakedStyle(const QString& name, std::shared_ptr<const MapStyle>& outStyle) const = 0;

        inline std::shared_ptr<const MapStyle> getBakedStyle(const QString& name) const
        {
            std::shared_ptr<const MapStyle> style;
            if (!obtainBakedStyle(name, style))
                return nullptr;
            return style;
        }
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_STYLES_COLLECTION_H_)
