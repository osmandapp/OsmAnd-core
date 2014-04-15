#ifndef _OSMAND_CORE_MAP_STYLES_H_
#define _OSMAND_CORE_MAP_STYLES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>

namespace OsmAnd
{
    class MapStyle;
    class MapStyle_P;

    class MapStyles_P;
    class OSMAND_CORE_API MapStyles
    {
        Q_DISABLE_COPY(MapStyles);
    private:
        PrivateImplementation<MapStyles_P> _p;
    protected:
    public:
        MapStyles();
        virtual ~MapStyles();

        bool registerStyle(const QString& filePath);
        bool obtainStyle(const QString& name, std::shared_ptr<const MapStyle>& outStyle) const;

        inline std::shared_ptr<const MapStyle> getStyle(const QString& name) const
        {
            std::shared_ptr<const MapStyle> style;
            obtainStyle(name, style);
            return style;
        }

    friend class OsmAnd::MapStyle;
    friend class OsmAnd::MapStyle_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLES_H_)
