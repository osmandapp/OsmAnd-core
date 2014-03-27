#ifndef _OSMAND_CORE_RASTERIZER_H_
#define _OSMAND_CORE_RASTERIZER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

class SkCanvas;

namespace OsmAnd
{
    class RasterizerEnvironment;
    class RasterizerContext;
    class RasterizedSymbolsGroup;
    namespace Model {
        class MapObject;
    } // namespace Model
    class IQueryController;
    namespace Rasterizer_Metrics {
        struct Metric_prepareContext;
    } // namespace Rasterizer_Metrics

    class Rasterizer_P;
    class OSMAND_CORE_API Rasterizer
    {
        Q_DISABLE_COPY(Rasterizer);
    private:
        const std::unique_ptr<Rasterizer_P> _d;
    protected:
    public:
        Rasterizer(const std::shared_ptr<const RasterizerContext>& context);
        virtual ~Rasterizer();

        const std::shared_ptr<const RasterizerContext> context;

        static void prepareContext(
            RasterizerContext& context,
            const AreaI& area31,
            const ZoomLevel zoom,
            const MapFoundationType foundation,
            const QList< std::shared_ptr<const Model::MapObject> >& objects,
            bool* nothingToRasterize = nullptr,
            const IQueryController* const controller = nullptr,
            Rasterizer_Metrics::Metric_prepareContext* const metric = nullptr);

        void rasterizeMap(
            SkCanvas& canvas,
            const bool fillBackground = true,
            const AreaI* const destinationArea = nullptr,
            const IQueryController* const controller = nullptr);

        void rasterizeSymbolsWithoutPaths(
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            std::function<bool (const std::shared_ptr<const Model::MapObject>& mapObject)> filter = nullptr,
            const IQueryController* const controller = nullptr);
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_RASTERIZER_H_)
