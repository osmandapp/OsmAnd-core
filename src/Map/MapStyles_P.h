#ifndef _OSMAND_CORE_MAP_STYLES_P_H_
#define _OSMAND_CORE_MAP_STYLES_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QHash>
#include <QReadWriteLock>

#include "OsmAndCore.h"

namespace OsmAnd
{
    class MapStyle;

    class MapStyles;
    class MapStyles_P
    {
    private:
    protected:
        MapStyles_P(MapStyles* owner);

        MapStyles* const owner;

        QHash< QString, std::shared_ptr<MapStyle> > _styles;
        mutable QReadWriteLock _stylesLock;

        bool registerEmbeddedStyle(const QString& resourceName);
    public:
        virtual ~MapStyles_P();

        bool registerStyle(const QString& filePath);
        bool obtainStyle(const QString& name, std::shared_ptr<const MapStyle>& outStyle) const;

    friend class OsmAnd::MapStyles;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_P_H_)
