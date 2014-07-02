#ifndef _OSMAND_CORE_RASTERIZER_ENVIRONMENT_P_H_
#define _OSMAND_CORE_RASTERIZER_ENVIRONMENT_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QMap>
#include <QVector>
#include <QHash>
#include <QMutex>

#include <SkPaint.h>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "MapStyle.h"
#include "MapStyleRule.h"
#include "Rasterizer.h"

class SkBitmapProcShader;
class SkPathEffect;
class SkBitmap;

namespace OsmAnd
{
    class MapStyle;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class Rasterizer;
    class ObfMapSectionInfo;

    class RasterizerEnvironment;
    class RasterizerEnvironment_P Q_DECL_FINAL
    {
    private:
    protected:
        RasterizerEnvironment_P(RasterizerEnvironment* owner);

        void initialize();

        mutable QMutex _settingsChangeMutex;
        QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue > _settings;

        SkPaint _mapPaint;
        SkPaint _textPaint;

        struct FontsRegisterEntry
        {
            QString resource;
            bool bold;
            bool italic;
        };
        QList< FontsRegisterEntry > _fontsRegister;

        mutable QMutex _fontTypefacesCacheMutex;
        mutable QHash< QString, SkTypeface* > _fontTypefacesCache;
        void clearFontsCache();
        SkTypeface* getTypefaceForFontResource(const QString& fontResource) const;

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

        mutable QMutex _mapIconsMutex;
        mutable QHash< QString, std::shared_ptr<const SkBitmap> > _mapIcons;

        mutable QMutex _textShieldsMutex;
        mutable QHash< QString, std::shared_ptr<const SkBitmap> > _textShields;

        QByteArray obtainResourceByName(const QString& name) const;
    public:
        virtual ~RasterizerEnvironment_P();

        ImplementationInterface<RasterizerEnvironment> owner;

        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> styleBuiltinValueDefs;

        const SkPaint& mapPaint;
        const SkPaint& textPaint;
        void configurePaintForText(SkPaint& paint, const QString& text, const bool bold, const bool italic) const;

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
        void setSettings(const QHash< QString, QString >& newSettings);

        void applyTo(MapStyleEvaluator& evaluator) const;

        bool obtainBitmapShader(const QString& name, SkBitmapProcShader* &outShader) const;
        bool obtainPathEffect(const QString& encodedPathEffect, SkPathEffect* &outPathEffect) const;
        bool obtainMapIcon(const QString& name, std::shared_ptr<const SkBitmap>& outIcon) const;
        bool obtainTextShield(const QString& name, std::shared_ptr<const SkBitmap>& outIcon) const;

    friend class OsmAnd::RasterizerEnvironment;
    };
}

#endif // !defined(_OSMAND_CORE_RASTERIZER_ENVIRONMENT_P_H_)
