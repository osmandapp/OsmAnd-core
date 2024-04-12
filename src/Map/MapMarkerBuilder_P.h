#ifndef _OSMAND_CORE_MAP_MARKER_BUILDER_P_H_
#define _OSMAND_CORE_MAP_MARKER_BUILDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QHash>
#include <QReadWriteLock>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkImage.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "MapMarker.h"
#include "TextRasterizer.h"
#include <OsmAndCore/SingleSkImage.h>

namespace OsmAnd
{
    class MapMarkersCollection;
    class MapMarkersCollection_P;

    class MapMarkerBuilder;
    class MapMarkerBuilder_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarkerBuilder_P);

    private:
    protected:
        MapMarkerBuilder_P(MapMarkerBuilder* const owner);

        mutable QReadWriteLock _lock;

        bool _isHidden;

        int _markerId;
        int _baseOrder;

        bool _isAccuracyCircleSupported;
        bool _isAccuracyCircleVisible;
        double _accuracyCircleRadius;
        FColorRGB _accuracyCircleBaseColor;

        PointI _position;

        float _height;
        float _elevationScaleFactor;

        sk_sp<const SkImage> _pinIcon;
        MapMarker::PinIconVerticalAlignment _pinIconVerticalAlignment;
        MapMarker::PinIconHorisontalAlignment _pinIconHorisontalAlignment;
        PointI _pinIconOffset;
        ColorARGB _pinIconModulationColor;

        QString _caption;
        TextRasterizer::Style _captionStyle;
        double _captionTopSpace;

        QHash< MapMarker::OnSurfaceIconKey, sk_sp<const SkImage> > _onMapSurfaceIcons;

        std::shared_ptr<const Model3D> _model3D;
        QHash<QString, FColorARGB> _model3DCustomMaterialColors;
        int _model3DMaxSizeInPixels;
    public:
        virtual ~MapMarkerBuilder_P();

        ImplementationInterface<MapMarkerBuilder> owner;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        int getMarkerId() const;
        void setMarkerId(const int markerId);

        int getBaseOrder() const;
        void setBaseOrder(const int baseOrder);

        bool isAccuracyCircleSupported() const;
        void setIsAccuracyCircleSupported(const bool supported);
        bool isAccuracyCircleVisible() const;
        void setIsAccuracyCircleVisible(const bool visible);
        double getAccuracyCircleRadius() const;
        void setAccuracyCircleRadius(const double radius);
        FColorRGB getAccuracyCircleBaseColor() const;
        void setAccuracyCircleBaseColor(const FColorRGB baseColor);

        PointI getPosition() const;
        void setPosition(const PointI position);

        float getHeight() const;
        void setHeight(const float height);

        float getElevationScaleFactor() const;
        void setElevationScaleFactor(const float scaleFactor);

        sk_sp<const SkImage> getPinIcon() const;
        void setPinIcon(const SingleSkImage& image);

        MapMarker::PinIconVerticalAlignment getPinIconVerticalAlignment() const;
        MapMarker::PinIconHorisontalAlignment getPinIconHorisontalAlignment() const;
        void setPinIconVerticalAlignment(const MapMarker::PinIconVerticalAlignment value);
        void setPinIconHorisontalAlignment(const MapMarker::PinIconHorisontalAlignment value);
        PointI getPinIconOffset() const;
        void setPinIconOffset(const PointI pinIconOffset);

        ColorARGB getPinIconModulationColor() const;
        void setPinIconModulationColor(const ColorARGB colorValue);

        QString getCaption() const;
        void setCaption(const QString& caption);
        TextRasterizer::Style getCaptionStyle() const;
        void setCaptionStyle(const TextRasterizer::Style captionStyle);
        double getCaptionTopSpace() const;
        void setCaptionTopSpace(const double captionTopSpace);

        QHash< MapMarker::OnSurfaceIconKey, sk_sp<const SkImage> > getOnMapSurfaceIcons() const;
        void addOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key, const SingleSkImage& image);
        void removeOnMapSurfaceIcon(const MapMarker::OnSurfaceIconKey key);
        void clearOnMapSurfaceIcons();

        std::shared_ptr<const Model3D> getModel3D() const;
        void setModel3D(const std::shared_ptr<const Model3D>& model3D);
        QHash<QString, FColorARGB> getModel3DCustomMaterialColors() const;
        void addModel3DCustomMaterialColor(const QString& materialName, const FColorARGB& color);
        void removeModel3DCustomMaterialColor(const QString& materialName);
        void clearModel3DCustomMaterialColors();
        int getModel3DMaxSizeInPixels() const;
        void setModel3DMaxSizeInPixels(const int maxSizeInPixels);

        std::shared_ptr<MapMarker> buildAndAddToCollection(const std::shared_ptr<MapMarkersCollection>& collection);

    friend class OsmAnd::MapMarkerBuilder;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_BUILDER_P_H_)
