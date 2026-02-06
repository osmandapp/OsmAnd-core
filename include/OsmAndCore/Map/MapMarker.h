#ifndef _OSMAND_CORE_MAP_MARKER_H_
#define _OSMAND_CORE_MAP_MARKER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IUpdatableMapSymbolsGroup.h>
#include <OsmAndCore/TextRasterizer.h>
#include <OsmAndCore/Map/Model3D.h>

namespace OsmAnd
{
    class MapMarkerBuilder;
    class MapMarkerBuilder_P;
    class MapMarkersCollection;
    class MapMarkersCollection_P;

    class MapMarker_P;
    class OSMAND_CORE_API MapMarker
    {
        Q_DISABLE_COPY_AND_MOVE(MapMarker);

    public:
        typedef const void* OnSurfaceIconKey;

        enum PinIconVerticalAlignment : unsigned int
        {
            Top = 0,
            CenterVertical = 1,
            Bottom = 2,
        };

        enum PinIconHorisontalAlignment : unsigned int
        {
            Left = 0,
            CenterHorizontal = 1,
            Right = 2,
        };

        class SymbolsGroup
            : public MapSymbolsGroup
            , public IUpdatableMapSymbolsGroup
        {
        private:
            const std::weak_ptr<MapMarker_P> _mapMarkerP;
        protected:
            SymbolsGroup(const std::shared_ptr<MapMarker_P>& mapMarkerP);
        public:
            virtual ~SymbolsGroup();

            const MapMarker* getMapMarker() const;

            virtual bool updatesPresent();
            virtual UpdateResult update(const MapState& mapState);

            virtual bool supportsResourcesRenew();

        friend class OsmAnd::MapMarker;
        friend class OsmAnd::MapMarker_P;
        };

    private:
        PrivateImplementation<MapMarker_P> _p;
    protected:
        MapMarker(
            const int markerId,
            const int baseOrder,
            const sk_sp<const SkImage>& pinIcon,
            const PinIconVerticalAlignment pinIconVerticalAlignment,
            const PinIconHorisontalAlignment pinIconHorisontalAlignment,
            const PointI pinIconOffset,
            const double captionTopSpace,
            const QHash< OnSurfaceIconKey, sk_sp<const SkImage> >& onMapSurfaceIcons,
            const std::shared_ptr<const Model3D>& model3D,
            const QHash<QString, FColorARGB>& model3DCustomMaterialColors,
            const bool isAccuracyCircleSupported,
            const FColorRGB accuracyCircleBaseColor,
            const int groupId = 0);

        bool applyChanges();
    public:
        virtual ~MapMarker();

        const int markerId;
        const int groupId;
        const int baseOrder;
        const sk_sp<const SkImage> pinIcon;
        const PinIconVerticalAlignment pinIconVerticalAlignment;
        const PinIconHorisontalAlignment pinIconHorisontalAlignment;
        const PointI pinIconOffset;
        const QHash< OnSurfaceIconKey, sk_sp<const SkImage> > onMapSurfaceIcons;
        const std::shared_ptr<const Model3D> model3D;
        const QHash<QString, FColorARGB> model3DCustomMaterialColors;
        const bool isAccuracyCircleSupported;
        const FColorRGB accuracyCircleBaseColor;
        const double captionTopSpace;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        bool isAccuracyCircleVisible() const;
        void setIsAccuracyCircleVisible(const bool visible);
        double getAccuracyCircleRadius() const;
        void setAccuracyCircleRadius(const double radius);

        PointI getPosition() const;
        void setPosition(const PointI position);

        PositionType getPositionType() const;
        void setPositionType(const PositionType positionType);

        double getAdditionalPosition() const;
        void setAdditionalPosition(const double additionalPosition);

        float getHeight() const;
        void setHeight(const float height);

        float getElevationScaleFactor() const;
        void setElevationScaleFactor(const float scaleFactor);

        bool getAdjustElevationToVectorObject() const;
        void setAdjustElevationToVectorObject(const bool adjust);

        float getOnMapSurfaceIconDirection(const OnSurfaceIconKey key) const;
        void setOnMapSurfaceIconDirection(const OnSurfaceIconKey key, const float direction);

        int getModel3DMaxSizeInPixels() const;
        void setModel3DMaxSizeInPixels(const int maxSizeInPixels);
        float getModel3DDirection() const;
        void setModel3DDirection(const float direction);

        ColorARGB getPinIconModulationColor() const;
        void setPinIconModulationColor(const ColorARGB colorValue);
        
        ColorARGB getOnSurfaceIconModulationColor() const;
        void setOnSurfaceIconModulationColor(const ColorARGB colorValue);
        
        void attachToVectorLine(const QVector<PointI64>& points);
        void attachToVectorLine(QVector<PointI64>&& segmentPoints);
        
        void setOffsetFromLine(int offset);

        void setUpdateAfterCreated(bool updateAfterCreated);

        void setCaption(const QString& caption);
        void setCaptionStyle(const TextRasterizer::Style& captionStyle);

        bool hasUnappliedChanges() const;
        bool hasUnappliedPrimitiveChanges() const;

        std::shared_ptr<SymbolsGroup> createSymbolsGroup(int subsection) const;

    friend class OsmAnd::MapMarkerBuilder;
    friend class OsmAnd::MapMarkerBuilder_P;
    friend class OsmAnd::MapMarkersCollection;
    friend class OsmAnd::MapMarkersCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_MARKER_H_)
