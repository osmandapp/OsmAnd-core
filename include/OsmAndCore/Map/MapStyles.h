#ifndef _OSMAND_CORE_MAP_STYLES_H_
#define _OSMAND_CORE_MAP_STYLES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class MapStyle;
    class MapStyle_P;

    class MapStyles_P;
    class OSMAND_CORE_API MapStyles
    {
        Q_DISABLE_COPY(MapStyles);
    private:
        const std::unique_ptr<MapStyles_P> _d;
    protected:
    public:
        MapStyles();
        virtual ~MapStyles();

        bool registerStyle(const QString& filePath);
        bool obtainStyle(const QString& name, std::shared_ptr<const MapStyle>& outStyle) const;

    friend class OsmAnd::MapStyle;
    friend class OsmAnd::MapStyle_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_H_)
