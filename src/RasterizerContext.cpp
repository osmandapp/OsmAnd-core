#include "RasterizerContext.h"

#include <limits>

#include "RasterizationStyleEvaluator.h"

OsmAnd::RasterizerContext::RasterizerContext( const std::shared_ptr<RasterizationStyle>& style_ )
    : style(style_)
{
    initialize();
}

OsmAnd::RasterizerContext::RasterizerContext( const std::shared_ptr<RasterizationStyle>& style_, const QMap< std::shared_ptr<RasterizationStyle::ValueDefinition>, RasterizationRule::Value >& styleInitialSettings )
    : style(style_)
    , _previousZoom(std::numeric_limits<uint32_t>::max())
    , _styleInitialSettings(styleInitialSettings)
{
    initialize();
}

OsmAnd::RasterizerContext::~RasterizerContext()
{
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

void OsmAnd::RasterizerContext::refresh( uint32_t zoom )
{
    if(_previousZoom == zoom)
        return;
    _previousZoom = zoom;

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

void OsmAnd::RasterizerContext::applyContext( RasterizationStyleEvaluator& evaluator ) const
{
    for(auto itSetting = _styleInitialSettings.begin(); itSetting != _styleInitialSettings.end(); ++itSetting)
    {
        evaluator.setValue(itSetting.key(), *itSetting);
    }
}

