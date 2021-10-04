#ifndef _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_
#define _OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QMap>
#include <QList>
#include <QHash>
#include <QMutex>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkPaint.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "MapStyleConstantValue.h"
#include "MapRasterizer.h"
#include "MapPresentationEnvironment.h"

class SkBitmap;

namespace OsmAnd
{
    class UnresolvedMapStyle;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;

    class MapPresentationEnvironment_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapPresentationEnvironment_P);

    public:
        typedef MapPresentationEnvironment::LanguagePreference LanguagePreference;
        typedef MapPresentationEnvironment::ShadowMode ShadowMode;

    private:
        QHash<IMapStyle::ValueDefinitionId, OsmAnd::MapStyleConstantValue> resolveSettings(const QHash<QString, QString> &newSettings) const;
    protected:
        MapPresentationEnvironment_P(MapPresentationEnvironment* owner);

        void initialize();

        mutable QMutex _settingsChangeMutex;
        QHash< IMapStyle::ValueDefinitionId, MapStyleConstantValue > _settings;

        std::shared_ptr<const IMapStyle::IAttribute> _defaultBackgroundColorAttribute;
        ColorARGB _defaultBackgroundColor;

        std::shared_ptr<const IMapStyle::IAttribute> _shadowOptionsAttribute;
        ShadowMode _shadowMode;
        ColorARGB _shadowColor;

        std::shared_ptr<const IMapStyle::IAttribute> _polygonMinSizeToDisplayAttribute;
        double _polygonMinSizeToDisplay;

        std::shared_ptr<const IMapStyle::IAttribute> _roadDensityZoomTileAttribute;
        unsigned int _roadDensityZoomTile;

        std::shared_ptr<const IMapStyle::IAttribute> _roadsDensityLimitPerTileAttribute;
        unsigned int _roadsDensityLimitPerTile;

        std::shared_ptr<const IMapStyle::IAttribute> _defaultSymbolPathSpacingAttribute;
        float _defaultSymbolPathSpacing;

        std::shared_ptr<const IMapStyle::IAttribute> _defaultBlockPathSpacingAttribute;
        float _defaultBlockPathSpacing;

        std::shared_ptr<const IMapStyle::IAttribute> _globalPathPaddingAttribute;
        float _globalPathPadding;

        MapStubStyle _desiredStubsStyle;

        mutable QMutex _shadersBitmapsMutex;
        mutable QHash< QString, std::shared_ptr<SkBitmap> > _shadersBitmaps;

        mutable QMutex _mapIconsMutex;
        mutable QHash< QString, std::shared_ptr<const SkBitmap> > _mapIcons;

        mutable QMutex _textShieldsMutex;
        mutable QHash< QString, std::shared_ptr<const SkBitmap> > _textShields;

        mutable QMutex _iconShieldsMutex;
        mutable QHash< QString, std::shared_ptr<const SkBitmap> > _iconShields;

        QByteArray obtainResourceByName(const QString& name) const;
    public:
        virtual ~MapPresentationEnvironment_P();

        ImplementationInterface<MapPresentationEnvironment> owner;

        QHash< IMapStyle::ValueDefinitionId, MapStyleConstantValue > getSettings() const;
        void setSettings(const QHash< OsmAnd::IMapStyle::ValueDefinitionId, MapStyleConstantValue >& newSettings);
        
        void setSettings(const QHash< QString, QString >& newSettings);

        void applyTo(MapStyleEvaluator& evaluator) const;
        void applyTo(MapStyleEvaluator &evaluator, const QHash< IMapStyle::ValueDefinitionId, MapStyleConstantValue > &settings) const;

        bool obtainShaderBitmap(const QString& name, std::shared_ptr<const SkBitmap>& outBitmap) const;
        bool obtainMapIcon(const QString& name, std::shared_ptr<const SkBitmap>& outIcon) const;
        bool obtainTextShield(const QString& name, std::shared_ptr<const SkBitmap>& outTextShield) const;
        bool obtainIconShield(const QString& name, std::shared_ptr<const SkBitmap>& outTextShield) const;

        ColorARGB getDefaultBackgroundColor(const ZoomLevel zoom) const;
        void obtainShadowOptions(const ZoomLevel zoom, ShadowMode& mode, ColorARGB& color) const;
        double getPolygonAreaMinimalThreshold(const ZoomLevel zoom) const;
        unsigned int getRoadDensityZoomTile(const ZoomLevel zoom) const;
        unsigned int getRoadsDensityLimitPerTile(const ZoomLevel zoom) const;
        float getDefaultSymbolPathSpacing() const;
        float getDefaultBlockPathSpacing() const;
        float getGlobalPathPadding() const;
        MapStubStyle getDesiredStubsStyle() const;
        ColorARGB getTransportRouteColor(const bool nightMode, const QString& renderAttrName) const;
        QHash<QString, int> getLineRenderingAttributes(const QString& renderAttrName) const;
        QHash<QString, int> getGpxColors() const;
        QHash<QString, QList<int>> getGpxWidth() const;
        QPair<QString, uint32_t> getRoadRenderingAttributes(const QString& renderAttrName, const QHash<QString, QString>& additionalSettings) const;

    friend class OsmAnd::MapPresentationEnvironment;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_PRESENTATION_ENVIRONMENT_P_H_)
