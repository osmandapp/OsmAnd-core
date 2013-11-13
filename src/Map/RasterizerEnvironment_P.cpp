#include "RasterizerEnvironment_P.h"
#include "RasterizerEnvironment.h"

#include <limits>

#include <SkDashPathEffect.h>
#include <SkBitmapProcShader.h>
#include <SkImageDecoder.h>
#include <SkStream.h>

#include "MapStyleEvaluator.h"
#include "MapStyleValue.h"
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
    , textPaint(_textPaint)
    , oneWayPaints(_oneWayPaints)
    , reverseOneWayPaints(_reverseOneWayPaints)
{
}

OsmAnd::RasterizerEnvironment_P::~RasterizerEnvironment_P()
{
    {
        QMutexLocker scopedLock(&_shadersBitmapsMutex);

        _shadersBitmaps.clear();
    }

    {
        QMutexLocker scopedLock(&_pathEffectsMutex);

        for(auto itPathEffect = _pathEffects.cbegin(); itPathEffect != _pathEffects.cend(); ++itPathEffect)
        {
            auto pathEffect = *itPathEffect;
            pathEffect->unref();
        }
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

    _textPaint.setAntiAlias(true);
    _textPaint.setLCDRenderText(true);
    _textPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    /*_textPaint.setStyle(SkPaint::kFill_Style);
    _textPaint.setStrokeWidth(1);
    _textPaint.setColor(SK_ColorBLACK);
    _textPaint.setTextAlign(SkPaint::kCenter_Align);*/
    static_assert(sizeof(QChar) == 2, "If QChar is not 2 bytes, then encoding is not kUTF16_TextEncoding");

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
}

QMap< std::shared_ptr<const OsmAnd::MapStyleValueDefinition>, OsmAnd::MapStyleValue > OsmAnd::RasterizerEnvironment_P::getSettings() const
{
    return _settings;
}

void OsmAnd::RasterizerEnvironment_P::setSettings( const QMap< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue >& newSettings )
{
    QMutexLocker scopedLocker(&_settingsChangeMutex);

    _settings = newSettings;
}

void OsmAnd::RasterizerEnvironment_P::applyTo( MapStyleEvaluator& evaluator ) const
{
    QMutexLocker scopedLocker(&_settingsChangeMutex);

    for(auto itSetting = _settings.cbegin(); itSetting != _settings.cend(); ++itSetting)
    {
        evaluator.setValue(itSetting.key(), *itSetting);
    }
}

bool OsmAnd::RasterizerEnvironment_P::obtainBitmapShader( const QString& name, SkBitmapProcShader* &outShader ) const
{
    QMutexLocker scopedLock(&_shadersBitmapsMutex);

    auto itShaderBitmap = _shadersBitmaps.constFind(name);
    if(itShaderBitmap == _shadersBitmaps.cend())
    {
        const auto shaderBitmapPath = QString::fromLatin1("map/shaders/%1.png").arg(name);

        // Get data from embedded resources
        const auto data = EmbeddedResources::decompressResource(shaderBitmapPath);

        // Decode bitmap for a shader
        auto shaderBitmap = new SkBitmap();
        SkMemoryStream dataStream(data.constData(), data.length(), false);
        if(!SkImageDecoder::DecodeStream(&dataStream, shaderBitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;
        itShaderBitmap = _shadersBitmaps.insert(name, std::shared_ptr<SkBitmap>(shaderBitmap));
    }

    // Create shader from that bitmap
    outShader = new SkBitmapProcShader(*itShaderBitmap->get(), SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
    return true;
}

bool OsmAnd::RasterizerEnvironment_P::obtainPathEffect( const QString& encodedPathEffect, SkPathEffect* &outPathEffect ) const
{
    QMutexLocker scopedLock(&_pathEffectsMutex);

    auto itPathEffects = _pathEffects.constFind(encodedPathEffect);
    if(itPathEffects == _pathEffects.cend())
    {
        const auto& strIntervals = encodedPathEffect.split('_', QString::SkipEmptyParts);

        const auto intervals = new SkScalar[strIntervals.size()];
        auto interval = intervals;
        for(auto itInterval = strIntervals.cbegin(); itInterval != strIntervals.cend(); ++itInterval, interval++)
            *interval = itInterval->toFloat();

        SkPathEffect* pathEffect = new SkDashPathEffect(intervals, strIntervals.size(), 0);
        delete[] intervals;

        itPathEffects = _pathEffects.insert(encodedPathEffect, pathEffect);
    }

    outPathEffect = *itPathEffects;
    return true;
}

bool OsmAnd::RasterizerEnvironment_P::obtainIcon( const QString& name, std::shared_ptr<const SkBitmap>& outIcon ) const
{
    QMutexLocker scopedLock(&_iconsMutex);

    auto itIcon = _icons.constFind(name);
    if(itIcon == _icons.cend())
    {
        const auto bitmapPath = QString::fromLatin1("map/icons/mm_%1.png").arg(name);

        // Get data from embedded resources
        auto data = EmbeddedResources::decompressResource(bitmapPath);

        // Decode data
        auto bitmap = new SkBitmap();
        SkMemoryStream dataStream(data.constData(), data.length(), false);
        if(!SkImageDecoder::DecodeStream(&dataStream, bitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;

        itIcon = _icons.insert(name, std::shared_ptr<const SkBitmap>(bitmap));
    }

    outIcon = *itIcon;
    return true;
}
