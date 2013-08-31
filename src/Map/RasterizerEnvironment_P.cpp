#include "RasterizerEnvironment_P.h"
#include "RasterizerEnvironment.h"

#include <limits>

#include <SkDashPathEffect.h>
#include <SkBitmapProcShader.h>
#include <SkImageDecoder.h>
#include <SkStream.h>

#include "MapStyleEvaluator.h"
#include "EmbeddedResources.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::RasterizerEnvironment_P::RasterizerEnvironment_P( RasterizerEnvironment* owner_ )
    : owner(owner_)
    , defaultBgColor(_defaultBgColor)
    , shadowLevelMin(_shadowLevelMin)
    , shadowLevelMax(_shadowLevelMax)
    , polygonMinSizeToDisplay(_polygonMinSizeToDisplay)
    , roadDensityZoomTile(_roadDensityZoomTile)
    , roadsDensityLimitPerTile(_roadsDensityLimitPerTile)
    , shadowRenderingMode(_shadowRenderingMode)
    , shadowRenderingColor(_shadowRenderingColor)
    , attributeRule_defaultColor(_attributeRule_defaultColor)
    , attributeRule_shadowRendering(_attributeRule_shadowRendering)
    , attributeRule_polygonMinSizeToDisplay(_attributeRule_polygonMinSizeToDisplay)
    , attributeRule_roadDensityZoomTile(_attributeRule_roadDensityZoomTile)
    , attributeRule_roadsDensityLimitPerTile(_attributeRule_roadsDensityLimitPerTile)
    , mapPaint(_mapPaint)
    , oneWayPaints(_oneWayPaints)
    , reverseOneWayPaints(_reverseOneWayPaints)
{
}

OsmAnd::RasterizerEnvironment_P::~RasterizerEnvironment_P()
{
    {
        QMutexLocker scopedLock(&_bitmapShadersMutex);

        for(auto itShaderEntry = _bitmapShaders.begin(); itShaderEntry != _bitmapShaders.end(); ++itShaderEntry)
            (*itShaderEntry)->unref();
        _bitmapShaders.clear();
    }
}

void OsmAnd::RasterizerEnvironment_P::initializeOneWayPaint( SkPaint& paint )
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xff6c70d5);
}

void OsmAnd::RasterizerEnvironment_P::initialize()
{
    _mapPaint.setAntiAlias(true);
    _shadowLevelMin = 0;
    _shadowLevelMax = 256;
    _roadDensityZoomTile = 0;
    _roadsDensityLimitPerTile = 0;
    _shadowRenderingMode = 0;
    _shadowRenderingColor = 0xff969696;
    _polygonMinSizeToDisplay = 0.0;
    _defaultBgColor = 0xfff1eee8;

    owner->style->resolveAttribute(QString::fromLatin1("defaultColor"), _attributeRule_defaultColor);
    owner->style->resolveAttribute(QString::fromLatin1("shadowRendering"), _attributeRule_shadowRendering);
    owner->style->resolveAttribute(QString::fromLatin1("polygonMinSizeToDisplay"), _attributeRule_polygonMinSizeToDisplay);
    owner->style->resolveAttribute(QString::fromLatin1("roadDensityZoomTile"), _attributeRule_roadDensityZoomTile);
    owner->style->resolveAttribute(QString::fromLatin1("roadsDensityLimitPerTile"), _attributeRule_roadsDensityLimitPerTile);

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

    /*_textPaint.setStyle(SkPaint::kFill_Style);
    _textPaint.setStrokeWidth(1);
    _textPaint.setColor(SK_ColorBLACK);
    _textPaint.setTextAlign(SkPaint::kCenter_Align);
    _textPaint.setAntiAlias(true);*/
}

void OsmAnd::RasterizerEnvironment_P::applyTo( MapStyleEvaluator& evaluator ) const
{
    for(auto itSetting = owner->settings.begin(); itSetting != owner->settings.end(); ++itSetting)
    {
        evaluator.setValue(itSetting.key(), *itSetting);
    }
}

bool OsmAnd::RasterizerEnvironment_P::obtainBitmapShader( const QString& name, SkBitmapProcShader* &outShader ) const
{
    //TODO: I'm not sure that it should be const_cast, like that...
    QMutexLocker scopedLock(&const_cast<RasterizerEnvironment_P*>(this)->_bitmapShadersMutex);

    auto itShader = _bitmapShaders.find(name);
    if(itShader == _bitmapShaders.end())
    {
        const auto shaderBitmapPath = QString::fromLatin1("map/shaders/%1.png").arg(name);

        // Get data from embedded resources
        auto data = EmbeddedResources::decompressResource(shaderBitmapPath);

        // Decode data
        SkBitmap shaderBitmap;
        SkMemoryStream dataStream(data.constData(), data.length(), false);
        if(!SkImageDecoder::DecodeStream(&dataStream, &shaderBitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;

        // Create shader from that bitmap
        auto shader = new SkBitmapProcShader(shaderBitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
        itShader = const_cast<RasterizerEnvironment_P*>(this)->_bitmapShaders.insert(name, shader);
    }

    outShader = *itShader;
    return true;
}
