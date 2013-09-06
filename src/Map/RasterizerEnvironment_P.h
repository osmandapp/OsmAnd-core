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

#ifndef __RASTERIZER_ENVIRONMENT_P_H_
#define __RASTERIZER_ENVIRONMENT_P_H_

#include <cstdint>
#include <memory>

#include <QMap>
#include <QVector>
#include <QHash>
#include <QMutex>

#include <SkPaint.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapStyle.h>
#include <OsmAndCore/Map/MapStyleRule.h>
#include <OsmAndCore/Map/Rasterizer.h>
#include <OsmAndCore/CommonTypes.h>

class SkBitmapProcShader;

namespace OsmAnd {

    class MapStyle;
    class MapStyleEvaluator;
    class Rasterizer;

    class RasterizerEnvironment;
    class OSMAND_CORE_API RasterizerEnvironment_P
    {
    private:
    protected:
        RasterizerEnvironment_P(RasterizerEnvironment* owner);

        void initialize();

        SkPaint _mapPaint;
        SkColor _defaultBgColor;
        uint32_t _shadowLevelMin;
        uint32_t _shadowLevelMax;
        double _polygonMinSizeToDisplay;
        uint32_t _roadDensityZoomTile;
        uint32_t _roadsDensityLimitPerTile;
        int _shadowRenderingMode;
        SkColor _shadowRenderingColor;

        std::shared_ptr<const MapStyleRule> _attributeRule_defaultColor;
        std::shared_ptr<const MapStyleRule> _attributeRule_shadowRendering;
        std::shared_ptr<const MapStyleRule> _attributeRule_polygonMinSizeToDisplay;
        std::shared_ptr<const MapStyleRule> _attributeRule_roadDensityZoomTile;
        std::shared_ptr<const MapStyleRule> _attributeRule_roadsDensityLimitPerTile;

        QVector< SkPaint > _oneWayPaints;
        QVector< SkPaint > _reverseOneWayPaints;
        static void initializeOneWayPaint(SkPaint& paint);

        QMutex _bitmapShadersMutex;
        QHash< QString, SkBitmapProcShader* > _bitmapShaders;
    public:
        virtual ~RasterizerEnvironment_P();

        RasterizerEnvironment* const owner;

        const SkPaint& mapPaint;

        const SkColor& defaultBgColor;
        const uint32_t& shadowLevelMin;
        const uint32_t& shadowLevelMax;
        const double& polygonMinSizeToDisplay;
        const uint32_t& roadDensityZoomTile;
        const uint32_t& roadsDensityLimitPerTile;
        const int& shadowRenderingMode;
        const SkColor& shadowRenderingColor;

        const std::shared_ptr<const MapStyleRule>& attributeRule_defaultColor;
        const std::shared_ptr<const MapStyleRule>& attributeRule_shadowRendering;
        const std::shared_ptr<const MapStyleRule>& attributeRule_polygonMinSizeToDisplay;
        const std::shared_ptr<const MapStyleRule>& attributeRule_roadDensityZoomTile;
        const std::shared_ptr<const MapStyleRule>& attributeRule_roadsDensityLimitPerTile;

        const QVector< SkPaint >& oneWayPaints;
        const QVector< SkPaint >& reverseOneWayPaints;

        void applyTo(MapStyleEvaluator& evaluator) const;

        bool obtainBitmapShader(const QString& name, SkBitmapProcShader* &outShader) const;

    friend class OsmAnd::RasterizerEnvironment;
    };

} // namespace OsmAnd

#endif // __RASTERIZER_ENVIRONMENT_P_H_
