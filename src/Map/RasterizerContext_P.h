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

#ifndef __RASTERIZER_CONTEXT_P_H_
#define __RASTERIZER_CONTEXT_P_H_

#include <stdint.h>
#include <memory>

#include <QList>
#include <QVector>

#include <SkColor.h>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <Rasterizer_P.h>

namespace OsmAnd {

    class Rasterizer_P;

    class RasterizerContext;
    class RasterizerContext_P
    {
    private:
    protected:
        RasterizerContext_P(RasterizerContext* owner);

        RasterizerContext* const owner;

        SkColor _defaultBgColor;
        int _shadowRenderingMode;
        SkColor _shadowRenderingColor;
        double _polygonMinSizeToDisplay;
        uint32_t _roadDensityZoomTile;
        uint32_t _roadsDensityLimitPerTile;

        AreaI _area31;
        ZoomLevel _zoom;
        double _tileDivisor;
        uint32_t _tileSize;
        float _densityFactor;
        double _precomputed31toPixelDivisor;
        PointF _tlOriginOffset;
        AreaF _renderViewport;
        bool _hasWater;
        bool _hasLand;
        uint32_t _shadowLevelMin;
        uint32_t _shadowLevelMax;

        SkPaint _mapPaint;

        QList< std::shared_ptr<const OsmAnd::Model::MapObject> > _mapObjects, _coastlineObjects, _basemapMapObjects, _basemapCoastlineObjects;
        QList< std::shared_ptr<const OsmAnd::Model::MapObject> > _combinedMapObjects, _triangulatedCoastlineObjects;
        QVector< Rasterizer_P::Primitive > _polygons, _lines, _points;
        QVector< Rasterizer_P::TextPrimitive > _texts;

        QHash< QString, SkPathEffect* > _pathEffects;

        void clear();
    public:
        virtual ~RasterizerContext_P();

    friend class OsmAnd::RasterizerContext;
    friend class OsmAnd::Rasterizer_P;
    };

} // namespace OsmAnd

#endif // __RASTERIZER_CONTEXT_P_H_
