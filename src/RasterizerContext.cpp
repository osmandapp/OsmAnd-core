#include "RasterizerContext.h"

#include <limits>
#include <SkDashPathEffect.h>

#include "RasterizationStyleEvaluator.h"
#include "Utilities.h"

OsmAnd::RasterizerContext::RasterizerContext( const std::shared_ptr<RasterizationStyle>& style_ )
    : style(style_)
{
    initialize();
}

OsmAnd::RasterizerContext::RasterizerContext( const std::shared_ptr<RasterizationStyle>& style_, const QMap< std::shared_ptr<RasterizationStyle::ValueDefinition>, RasterizationRule::Value >& styleInitialSettings )
    : style(style_)
    , _zoom(std::numeric_limits<uint32_t>::max())
    , _styleInitialSettings(styleInitialSettings)
{
    initialize();
}

OsmAnd::RasterizerContext::~RasterizerContext()
{
    for(auto itPathEffect = _pathEffects.begin(); itPathEffect != _pathEffects.end(); ++itPathEffect)
    {
        auto pathEffect = *itPathEffect;
        pathEffect->unref();
    }
}

void OsmAnd::RasterizerContext::initialize()
{
    _paint.setAntiAlias(true);
    _shadowLevelMin = 0;
    _shadowLevelMax = 256;
    _roadDensityZoomTile = 0;
    _roadsDensityLimitPerTile = 0;
    _shadowRenderingMode = 0;
    _shadowRenderingColor = 0xff969696;
    _polygonMinSizeToDisplay = 0.0;
    _defaultColor = 0xfff1eee8;

    style->resolveAttribute("defaultColor", attributeRule_defaultColor);
    style->resolveAttribute("shadowRendering", attributeRule_shadowRendering);
    style->resolveAttribute("polygonMinSizeToDisplay", attributeRule_polygonMinSizeToDisplay);
    style->resolveAttribute("roadDensityZoomTile", attributeRule_roadDensityZoomTile);
    style->resolveAttribute("roadsDensityLimitPerTile", attributeRule_roadsDensityLimitPerTile);
}

void OsmAnd::RasterizerContext::refresh( const AreaD& areaGeo, uint32_t zoom, const PointI& tlOriginOffset, uint32_t tileSidePixelLength )
{
    _areaGeo = areaGeo;
    if(_zoom != zoom)
    {
        _zoom = zoom;
        _tileDivisor = Utilities::getPowZoom(31 - zoom);
        if(attributeRule_defaultColor)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_defaultColor);
            applyContext(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
                _defaultColor = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_COLOR_VALUE);
        }
        if(attributeRule_shadowRendering)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_shadowRendering);
            applyContext(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
            {
                _shadowRenderingMode = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE);
                _shadowRenderingColor = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_SHADOW_COLOR);
            }
        }
        if(attributeRule_polygonMinSizeToDisplay)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_polygonMinSizeToDisplay);
            applyContext(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
                _polygonMinSizeToDisplay = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE);
        }
        if(attributeRule_roadDensityZoomTile)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_roadDensityZoomTile);
            applyContext(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
                _roadDensityZoomTile = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE);
        }
        if(attributeRule_roadsDensityLimitPerTile)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_roadsDensityLimitPerTile);
            applyContext(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
                _roadsDensityLimitPerTile = evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE);
        }
    }
    _tileSidePixelLength = tileSidePixelLength;
    _areaTileD.right = Utilities::getTileNumberX(zoom, areaGeo.right);
    _areaTileD.left = Utilities::getTileNumberX(zoom, areaGeo.left);
    _areaTileD.top = Utilities::getTileNumberY(zoom, areaGeo.top);
    _areaTileD.bottom = Utilities::getTileNumberY(zoom, areaGeo.bottom);
    _tileWidth = _areaTileD.right - _areaTileD.left;
    _tileHeight = _areaTileD.bottom - _areaTileD.top;
    const auto pixelWidth = static_cast<int32_t>(_tileWidth * tileSidePixelLength);
    const auto pixelHeight = static_cast<int32_t>(_tileHeight * tileSidePixelLength);
    _viewport.topLeft = tlOriginOffset;
    _viewport.bottomRight = tlOriginOffset;
    _viewport.bottomRight.x += pixelWidth;
    _viewport.bottomRight.y = pixelHeight - tlOriginOffset.x;
}

void OsmAnd::RasterizerContext::applyContext( RasterizationStyleEvaluator& evaluator ) const
{
    for(auto itSetting = _styleInitialSettings.begin(); itSetting != _styleInitialSettings.end(); ++itSetting)
    {
        evaluator.setValue(itSetting.key(), *itSetting);
    }
}

SkPathEffect* OsmAnd::RasterizerContext::obtainPathEffect( const QString& pathEffect )
{
    auto itEffect = _pathEffects.find(pathEffect);
    if(itEffect != _pathEffects.end())
        return *itEffect;
    
    const auto& strIntervals = pathEffect.split('_', QString::SkipEmptyParts);
    
    SkScalar* intervals = new SkScalar[strIntervals.size()];
    uint32_t idx = 0;
    for(auto itInterval = strIntervals.begin(); itInterval != strIntervals.end(); ++itInterval, idx++)
        intervals[idx] = itInterval->toFloat();

    SkPathEffect* pPathEffect = new SkDashPathEffect(intervals, strIntervals.size(), 0);
    delete[] intervals;
    _pathEffects.insert(pathEffect, pPathEffect);
    return pPathEffect;
}

