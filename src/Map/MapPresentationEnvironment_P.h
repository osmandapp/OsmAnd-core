#ifndef _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_
#define _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QMap>
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

    class MapPresentationEnvironment;
    class MapPresentationEnvironment_P Q_DECL_FINAL
    {
    private:
    protected:
        MapPresentationEnvironment_P(MapPresentationEnvironment* owner);

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

        ColorARGB _defaultBgColor;
        double _polygonMinSizeToDisplay;
        unsigned int _roadDensityZoomTile;
        unsigned int _roadsDensityLimitPerTile;
        int _shadowRenderingMode;
        ColorARGB _shadowRenderingColor;

        std::shared_ptr<const MapStyleRule> _attributeRule_defaultColor;
        std::shared_ptr<const MapStyleRule> _attributeRule_shadowRendering;
        std::shared_ptr<const MapStyleRule> _attributeRule_polygonMinSizeToDisplay;
        std::shared_ptr<const MapStyleRule> _attributeRule_roadDensityZoomTile;
        std::shared_ptr<const MapStyleRule> _attributeRule_roadsDensityLimitPerTile;

        QList< SkPaint > _oneWayPaints;
        QList< SkPaint > _reverseOneWayPaints;
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
        virtual ~MapPresentationEnvironment_P();

        ImplementationInterface<MapPresentationEnvironment> owner;

        void configurePaintForText(SkPaint& paint, const QString& text, const bool bold, const bool italic) const;

        const std::shared_ptr<const ObfMapSectionInfo> dummyMapSection;

        QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue > getSettings() const;
        void setSettings(const QHash< std::shared_ptr<const MapStyleValueDefinition>, MapStyleValue >& newSettings);
        void setSettings(const QHash< QString, QString >& newSettings);

        void applyTo(MapStyleEvaluator& evaluator) const;

        bool obtainBitmapShader(const QString& name, SkBitmapProcShader* &outShader) const;
        bool obtainPathEffect(const QString& encodedPathEffect, SkPathEffect* &outPathEffect) const;
        bool obtainMapIcon(const QString& name, std::shared_ptr<const SkBitmap>& outIcon) const;
        bool obtainTextShield(const QString& name, std::shared_ptr<const SkBitmap>& outIcon) const;

        ColorARGB getDefaultBackgroundColor(const ZoomLevel zoom) const;
        void obtainShadowRenderingOptions(const ZoomLevel zoom, int& mode, ColorARGB& color) const;
        double getPolygonAreaMinimalThreshold(const ZoomLevel zoom) const;
        unsigned int getRoadDensityZoomTile(const ZoomLevel zoom) const;
        unsigned int getRoadsDensityLimitPerTile(const ZoomLevel zoom) const;

    friend class OsmAnd::MapPresentationEnvironment;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_)
