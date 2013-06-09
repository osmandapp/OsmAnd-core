/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __RASTERIZER_CONTEXT_H_
#define __RASTERIZER_CONTEXT_H_

#include <stdint.h>
#include <memory>

#include <QMap>

#include <SkPaint.h>
#include <SkPathEffect.h>

#include <OsmAndCore.h>
#include <RasterizationStyle.h>
#include <RasterizationRule.h>
#include <Rasterizer.h>
#include <CommonTypes.h>

namespace OsmAnd {

    class RasterizationStyle;
    class RasterizationStyleEvaluator;
    class Rasterizer;
    
    class OSMAND_CORE_API RasterizerContext
    {
    private:
    protected:
        bool _wasAborted;
        AreaI _area31;
        uint32_t _zoom;
        double _tileDivisor;
        uint32_t _tileSidePixelLength;
        double _precomputed31toPixelDivisor;
        PointF _tlOriginOffset;
        AreaF _renderViewport;
        bool _hasBasemap;
        bool _hasWater;
        bool _hasLand;
        QList< std::shared_ptr<OsmAnd::Model::MapObject> > _mapObjects, _coastlineObjects, _basemapMapObjects, _basemapCoastlineObjects;
        QList< std::shared_ptr<OsmAnd::Model::MapObject> > _combinedMapObjects, _triangulatedCoastlineObjects;
        QVector< Rasterizer::Primitive > _polygons, _lines, _points;
        QVector< Rasterizer::TextPrimitive > _texts;

        SkPaint _mapPaint;
        uint32_t _defaultBgColor;
        uint32_t _shadowLevelMin;
        uint32_t _shadowLevelMax;
        double _polygonMinSizeToDisplay;
        uint32_t _roadDensityZoomTile;
        uint32_t _roadsDensityLimitPerTile;
        int _shadowRenderingMode;
        uint32_t _shadowRenderingColor;

        const QMap< std::shared_ptr<RasterizationStyle::ValueDefinition>, RasterizationRule::Value > _styleInitialSettings;

        std::shared_ptr<RasterizationRule> attributeRule_defaultColor;
        std::shared_ptr<RasterizationRule> attributeRule_shadowRendering;
        std::shared_ptr<RasterizationRule> attributeRule_polygonMinSizeToDisplay;
        std::shared_ptr<RasterizationRule> attributeRule_roadDensityZoomTile;
        std::shared_ptr<RasterizationRule> attributeRule_roadsDensityLimitPerTile;

        QVector< SkPaint > _oneWayPaints;
        QVector< SkPaint > _reverseOneWayPaints;
        static void initializeOneWayPaint(SkPaint& paint);

        void initialize();
        bool update(const AreaI& area31, uint32_t zoom, const PointF& tlOriginOffset, uint32_t tileSidePixelLength);

        QHash< QString, SkPathEffect* > _pathEffects;
        SkPathEffect* obtainPathEffect(const QString& pathEffect);

        SkPaint _textPaint;
        std::shared_ptr<SkTypeface> _textFont;
    public:
        RasterizerContext(const std::shared_ptr<RasterizationStyle>& style);
        RasterizerContext(const std::shared_ptr<RasterizationStyle>& style, const QMap< std::shared_ptr<RasterizationStyle::ValueDefinition>, RasterizationRule::Value >& styleInitialSettings);
        virtual ~RasterizerContext();

        const std::shared_ptr<RasterizationStyle> style;

        void applyTo(RasterizationStyleEvaluator& evaluator) const;

    friend class Rasterizer;
    };

} // namespace OsmAnd

#endif // __RASTERIZER_CONTEXT_H_
