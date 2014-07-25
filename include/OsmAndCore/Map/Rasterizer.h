#ifndef _OSMAND_CORE_RASTERIZER_H_
#define _OSMAND_CORE_RASTERIZER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/Primitiviser.h>

class SkCanvas;

namespace OsmAnd
{
    class RasterizedSymbolsGroup;

    class Rasterizer_P;
    class OSMAND_CORE_API Rasterizer
    {
        Q_DISABLE_COPY(Rasterizer);
    private:
        PrivateImplementation<Rasterizer_P> _p;
    protected:
    public:
        Rasterizer();
        virtual ~Rasterizer();

        void rasterizeMap(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            SkCanvas& canvas,
            const bool fillBackground = true,
            const AreaI* const destinationArea = nullptr,
            const IQueryController* const controller = nullptr);

        void rasterizeSymbolsWithoutPaths(
            const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            std::function<bool (const std::shared_ptr<const Model::BinaryMapObject>& mapObject)> filter = nullptr,
            const IQueryController* const controller = nullptr);
    };
}

#endif // !defined(_OSMAND_CORE_RASTERIZER_H_)
