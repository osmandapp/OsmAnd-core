#ifndef _OSMAND_CORE_RASTER_MAP_SYMBOL_H_
#define _OSMAND_CORE_RASTER_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Map/MapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API RasterMapSymbol : public MapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(RasterMapSymbol);

    private:
    protected:
        RasterMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group);
    public:
        virtual ~RasterMapSymbol();

        sk_sp<const SkImage> image;
        PointI size;
        QString content;
        LanguageId languageId;
        float minDistance;
        AreaI margin;
    };
}

#endif // !defined(_OSMAND_CORE_RASTER_MAP_SYMBOL_H_)
