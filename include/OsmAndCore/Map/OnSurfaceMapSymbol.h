#ifndef _OSMAND_CORE_ON_SURFACE_MAP_SYMBOL_H_
#define _OSMAND_CORE_ON_SURFACE_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/BoundToPointMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API OnSurfaceMapSymbol : public BoundToPointMapSymbol
    {
        Q_DISABLE_COPY(OnSurfaceMapSymbol);
    private:
    protected:
    public:
        OnSurfaceMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const IntersectionModeFlags intersectionModeFlags,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const PointI& location31);
        virtual ~OnSurfaceMapSymbol();

        // NaN value is considered as "aligned to azimuth"
        float direction;
        bool isAzimuthAlignedDirection() const;
    };
}

#endif // !defined(_OSMAND_CORE_ON_SURFACE_MAP_SYMBOL_H_)
