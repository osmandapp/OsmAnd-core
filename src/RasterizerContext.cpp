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

void OsmAnd::RasterizerContext::initializeOneWayPaint( SkPaint& paint )
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xff6c70d5);
}

void OsmAnd::RasterizerContext::initialize()
{
    _wasAborted = true;
    _mapPaint.setAntiAlias(true);
    _shadowLevelMin = 0;
    _shadowLevelMax = 256;
    _roadDensityZoomTile = 0;
    _roadsDensityLimitPerTile = 0;
    _shadowRenderingMode = 0;
    _shadowRenderingColor = 0xff969696;
    _polygonMinSizeToDisplay = 0.0;
    _defaultBgColor = 0xfff1eee8;

    style->resolveAttribute("defaultColor", attributeRule_defaultColor);
    style->resolveAttribute("shadowRendering", attributeRule_shadowRendering);
    style->resolveAttribute("polygonMinSizeToDisplay", attributeRule_polygonMinSizeToDisplay);
    style->resolveAttribute("roadDensityZoomTile", attributeRule_roadDensityZoomTile);
    style->resolveAttribute("roadsDensityLimitPerTile", attributeRule_roadsDensityLimitPerTile);

    {
        const float intervals_oneway[4][4] =
        {
            {0, 12, 10, 152},
            {0, 12, 9, 153},
            {0, 18, 2, 154},
            {0, 18, 1, 155}
        };
        SkPathEffect* arrowDashEffect1 = new SkDashPathEffect(intervals_oneway[0], 4, 0);
        SkPathEffect* arrowDashEffect2 = new SkDashPathEffect(intervals_oneway[1], 4, 1);
        SkPathEffect* arrowDashEffect3 = new SkDashPathEffect(intervals_oneway[2], 4, 1);
        SkPathEffect* arrowDashEffect4 = new SkDashPathEffect(intervals_oneway[3], 4, 1);

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(1.0f);
            paint.setPathEffect(arrowDashEffect1)->unref();
            _oneWayPaints.push_back(paint);
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(2.0f);
            paint.setPathEffect(arrowDashEffect2)->unref();
            _oneWayPaints.push_back(paint);
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(3.0f);
            paint.setPathEffect(arrowDashEffect3)->unref();
            _oneWayPaints.push_back(paint);
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(4.0f);
            paint.setPathEffect(arrowDashEffect4)->unref();
            _oneWayPaints.push_back(paint);
        }
    }
    
    {
        const float intervals_reverse[4][4] =
        {
            {0, 12, 10, 152},
            {0, 13, 9, 152},
            {0, 14, 2, 158},
            {0, 15, 1, 158}
        };
        SkPathEffect* arrowDashEffect1 = new SkDashPathEffect(intervals_reverse[0], 4, 0);
        SkPathEffect* arrowDashEffect2 = new SkDashPathEffect(intervals_reverse[1], 4, 1);
        SkPathEffect* arrowDashEffect3 = new SkDashPathEffect(intervals_reverse[2], 4, 1);
        SkPathEffect* arrowDashEffect4 = new SkDashPathEffect(intervals_reverse[3], 4, 1);

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(1.0f);
            paint.setPathEffect(arrowDashEffect1)->unref();
            _reverseOneWayPaints.push_back(paint);
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(2.0f);
            paint.setPathEffect(arrowDashEffect2)->unref();
            _reverseOneWayPaints.push_back(paint);
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(3.0f);
            paint.setPathEffect(arrowDashEffect3)->unref();
            _reverseOneWayPaints.push_back(paint);
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(4.0f);
            paint.setPathEffect(arrowDashEffect4)->unref();
            _reverseOneWayPaints.push_back(paint);
        }
    }

    _textPaint.setStyle(SkPaint::kFill_Style);
    _textPaint.setStrokeWidth(1);
    _textPaint.setColor(SK_ColorBLACK);
    _textPaint.setTextAlign(SkPaint::kCenter_Align);
    _textPaint.setAntiAlias(true);
}

bool OsmAnd::RasterizerContext::update( const AreaI& area31, uint32_t zoom, const PointF& tlOriginOffset, uint32_t tileSidePixelLength, float densityFactor )
{
    bool evaluateAttributes = false;
    bool evaluate31ToPixelDivisor = false;
    bool evaluateRenderViewport = false;
    evaluateAttributes = evaluateAttributes || (_zoom != zoom);
    evaluate31ToPixelDivisor = evaluate31ToPixelDivisor || evaluateAttributes;
    evaluate31ToPixelDivisor = evaluate31ToPixelDivisor || (_tileSidePixelLength != tileSidePixelLength);
    evaluateRenderViewport = evaluateRenderViewport || evaluate31ToPixelDivisor;
    evaluateRenderViewport = evaluateRenderViewport || (_area31 != area31);
    evaluateRenderViewport = evaluateRenderViewport || (_tlOriginOffset != tlOriginOffset);

    if(evaluateAttributes)
    {
        _tileDivisor = Utilities::getPowZoom(31 - zoom);
        if(attributeRule_defaultColor)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_defaultColor);
            applyTo(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
                evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_COLOR_VALUE, _defaultBgColor);
        }
        if(attributeRule_shadowRendering)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_shadowRendering);
            applyTo(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
            {
                evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, _shadowRenderingMode);
                evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_SHADOW_COLOR, _shadowRenderingColor);
            }
        }
        if(attributeRule_polygonMinSizeToDisplay)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_polygonMinSizeToDisplay);
            applyTo(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
            {
                int polygonMinSizeToDisplay;
                if(evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, polygonMinSizeToDisplay))
                    _polygonMinSizeToDisplay = polygonMinSizeToDisplay;
            }
        }
        if(attributeRule_roadDensityZoomTile)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_roadDensityZoomTile);
            applyTo(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
                evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, _roadDensityZoomTile);
        }
        if(attributeRule_roadsDensityLimitPerTile)
        {
            RasterizationStyleEvaluator evaluator(style, attributeRule_roadsDensityLimitPerTile);
            applyTo(evaluator);
            evaluator.setIntegerValue(RasterizationStyle::builtinValueDefinitions.INPUT_MINZOOM, zoom);
            if(evaluator.evaluate())
                evaluator.getIntegerValue(RasterizationStyle::builtinValueDefinitions.OUTPUT_ATTR_INT_VALUE, _roadsDensityLimitPerTile);
        }
    }

    if(evaluate31ToPixelDivisor)
    {
        _precomputed31toPixelDivisor = _tileDivisor / tileSidePixelLength;
    }

    if(evaluateRenderViewport)
    {
        const auto pixelWidth = static_cast<float>(area31.width()) / _precomputed31toPixelDivisor;
        const auto pixelHeight = static_cast<float>(area31.height()) / _precomputed31toPixelDivisor;
        _renderViewport.topLeft = tlOriginOffset;
        _renderViewport.bottomRight.x = tlOriginOffset.x + pixelWidth;
        _renderViewport.bottomRight.y = pixelHeight - tlOriginOffset.x;
    }

    _zoom = zoom;
    _area31 = area31;
    _tileSidePixelLength = tileSidePixelLength;
    _densityFactor = densityFactor;

    return evaluateAttributes || evaluate31ToPixelDivisor || evaluateRenderViewport;
}

void OsmAnd::RasterizerContext::applyTo( RasterizationStyleEvaluator& evaluator ) const
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

