#ifndef _OSMAND_CORE_MAP_STYLES_P_H_
#define _OSMAND_CORE_MAP_STYLES_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QHash>

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

        bool registerEmbeddedStyle(const QString& resourceName);
        bool registerStyle(const QString& filePath);
        bool obtainStyle(const QString& name, std::shared_ptr<const OsmAnd::MapStyle>& outStyle);
    public:
        virtual ~MapStyles_P();

    friend class OsmAnd::MapStyles;
    };
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_MAP_STYLES_P_H_)
