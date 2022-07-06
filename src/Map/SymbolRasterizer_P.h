#ifndef _OSMAND_CORE_SYMBOL_RASTERIZER_P_H_
#define _OSMAND_CORE_SYMBOL_RASTERIZER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkCanvas.h>
#include <SkPaint.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapCommonTypes.h"
#include "MapPrimitiviser.h"
#include "SymbolRasterizer.h"
#include "OsmAndCore/PointsAndAreas.h"

namespace OsmAnd
{
    class MapObject;
    class IQueryController;

    class SymbolRasterizer_P Q_DECL_FINAL
    {
    public:
        typedef SymbolRasterizer::RasterizedSymbolsGroup RasterizedSymbolsGroup;
        typedef SymbolRasterizer::RasterizedSymbol RasterizedSymbol;
        typedef SymbolRasterizer::RasterizedSpriteSymbol RasterizedSpriteSymbol;
        typedef SymbolRasterizer::RasterizedOnPathSymbol RasterizedOnPathSymbol;
        typedef SymbolRasterizer::FilterByMapObject FilterByMapObject;

    private:
    protected:
        SymbolRasterizer_P(SymbolRasterizer* const owner);
    public:
        ~SymbolRasterizer_P();

        ImplementationInterface<SymbolRasterizer> owner;

        void rasterize(
            const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
            QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
            const FilterByMapObject filter,
            const std::shared_ptr<const IQueryController>& queryController) const;
    private:
        QHash<std::shared_ptr<const MapPrimitiviser::TextSymbol>, QVector<PointI>> combineSimilarText(
            const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
            const FilterByMapObject filter,
            const std::shared_ptr<const IQueryController>& queryController,
            std::vector<const std::shared_ptr<const MapObject>>& filteredMapObjects) const;
        bool combine2Segments(QVector<PointI>& pointsS, QVector<PointI>& pointsP, QVector<PointI>& outPoints, float combineGap,
                         float* gapMetric, bool combine, PointD scaleDivisor31ToPixel) const;
        double calcLength(QVector<PointI>& pointsP, PointD scaleDivisor31ToPixel) const;

    friend class OsmAnd::SymbolRasterizer;
    };

    struct CombineOnPathText
    {
        CombineOnPathText(const std::shared_ptr<const MapPrimitiviser::TextSymbol> textSymbol_, QVector<PointI> points31_)
        : textSymbol(textSymbol_), points31(points31_), combined(false)
        {
        }
        const std::shared_ptr<const MapPrimitiviser::TextSymbol> textSymbol;
        QVector<PointI> points31;
        bool combined;
    };
}

#endif // !defined(_OSMAND_CORE_SYMBOL_RASTERIZER_P_H_)
