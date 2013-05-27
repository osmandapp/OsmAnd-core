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
#include <Area.h>

namespace OsmAnd {

    class RasterizationStyle;
    class RasterizationStyleEvaluator;
    class Rasterizer;
    
    class OSMAND_CORE_API RasterizerContext
    {
    private:
    protected:
        AreaD _areaGeo;
        AreaD _areaTileD;
        uint32_t _zoom;
        double _tileDivisor;
        uint32_t _tileSidePixelLength;
        double _tileWidth;
        double _tileHeight;
        AreaI _viewport;

        SkPaint _paint;
        uint32_t _defaultColor;
        uint32_t _lastRenderedKey;
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

        void initialize();
        void refresh(const AreaD& areaGeo, uint32_t zoom, const PointI& tlOriginOffset, uint32_t tileSidePixelLength);

        QHash< QString, SkPathEffect* > _pathEffects;
        SkPathEffect* obtainPathEffect(const QString& pathEffect);
    public:
        RasterizerContext(const std::shared_ptr<RasterizationStyle>& style);
        RasterizerContext(const std::shared_ptr<RasterizationStyle>& style, const QMap< std::shared_ptr<RasterizationStyle::ValueDefinition>, RasterizationRule::Value >& styleInitialSettings);
        virtual ~RasterizerContext();

        const std::shared_ptr<RasterizationStyle> style;

        void applyContext(RasterizationStyleEvaluator& evaluator) const;

    friend class Rasterizer;
    };

} // namespace OsmAnd

#endif // __RASTERIZER_CONTEXT_H_
