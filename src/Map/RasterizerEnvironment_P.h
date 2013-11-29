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

#ifndef _OSMAND_CORE_RASTERIZER_ENVIRONMENT_P_H_
#define _OSMAND_CORE_RASTERIZER_ENVIRONMENT_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
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
class SkPathEffect;
class SkBitmap;

namespace OsmAnd {

    class MapStyle;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class Rasterizer;
    class ObfMapSectionInfo;

    class RasterizerEnvironment;
    class RasterizerEnvironment_P
    {
    private:
    protected:
        RasterizerEnvironment_P(RasterizerEnvironment* owner);

        void initialize();

        mutable QMutex _settingsChangeMutex;
        QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue > _settings;

        SkPaint _mapPaint;
        SkPaint _textPaint;

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

        mutable QMutex _shadersBitmapsMutex;
        mutable QHash< QString, std::shared_ptr<SkBitmap> > _shadersBitmaps;

        mutable QMutex _pathEffectsMutex;
        mutable QHash< QString, SkPathEffect* > _pathEffects;

        mutable QMutex _iconsMutex;
        mutable QHash< QString, std::shared_ptr<const SkBitmap> > _icons;
    public:
        virtual ~RasterizerEnvironment_P();

        RasterizerEnvironment* const owner;

        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> styleBuiltinValueDefs;

        const SkPaint& mapPaint;
        const SkPaint& textPaint;

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

        const std::shared_ptr<const ObfMapSectionInfo> dummyMapSection;

        QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue > getSettings() const;
        void setSettings(const QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue >& newSettings);

        void applyTo(MapStyleEvaluator& evaluator) const;

        bool obtainBitmapShader(const QString& name, SkBitmapProcShader* &outShader) const;
        bool obtainPathEffect(const QString& encodedPathEffect, SkPathEffect* &outPathEffect) const;
        bool obtainIcon(const QString& name, std::shared_ptr<const SkBitmap>& outIcon) const;

    friend class OsmAnd::RasterizerEnvironment;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_RASTERIZER_ENVIRONMENT_P_H_
