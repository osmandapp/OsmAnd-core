#ifndef _OSMAND_CORE_VECTOR_LINE_H_
#define _OSMAND_CORE_VECTOR_LINE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QVector>
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
#include <OsmAndCore/Map/MapMarker.h>

namespace OsmAnd
{
    class VectorLineBuilder;
    class VectorLineBuilder_P;
    class VectorLinesCollection;
    class VectorLinesCollection_P;

    class VectorLine_P;

    class OSMAND_CORE_API VectorLine
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLine);

    public:
        class SymbolsGroup
            : public MapSymbolsGroup
            , public IUpdatableMapSymbolsGroup
        {
        private:
            const std::weak_ptr<VectorLine_P> _vectorLineP;
        protected:
            SymbolsGroup(const std::shared_ptr<VectorLine_P>& vectorLineP);
        public:
            virtual ~SymbolsGroup();

            const VectorLine* getVectorLine() const;

            virtual bool updatesPresent();
            virtual UpdateResult update(const MapState& mapState);

            virtual bool supportsResourcesRenew();

        friend class OsmAnd::VectorLine;
        friend class OsmAnd::VectorLine_P;
        };

        struct OnPathSymbolData
        {
            OnPathSymbolData(
                PointI position31, float distance, float direction, float elevation, float elevationScaleFactor);
            ~OnPathSymbolData();

            PointI position31;
            float distance;
            float direction;
            float elevation;
            float elevationScaleFactor;
        };

        enum class JointStyle {
            MITER,
            BEVEL,
            ROUND
        };

        enum class EndCapStyle {
            BUTT,
            SQUARE,
            ROUND,
            JOINT,
            ARROW
        };

    private:
        PrivateImplementation<VectorLine_P> _p;
    protected:
        VectorLine(
            const int lineId,
            const int baseOrder,
            const sk_sp<const SkImage>& pathIcon = nullptr,
            const sk_sp<const SkImage>& specialPathIcon = nullptr,
            const bool pathIconOnSurface = true,
            const float screenScale = 2,
            const EndCapStyle endCapStyle = EndCapStyle::ROUND,
            const JointStyle jointStyle = JointStyle::ROUND
        );

        bool applyChanges();
    public:
        virtual ~VectorLine();

        const int lineId;
        const int baseOrder;
        const sk_sp<const SkImage> pathIcon;
        const sk_sp<const SkImage> specialPathIcon;
        const bool pathIconOnSurface;
        const float screenScale;
        EndCapStyle endCapStyle;
        JointStyle jointStyle;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        float getStartingDistance() const;
        void setStartingDistance(const float distanceInMeters);

        float getArrowStartingGap() const;

        bool showArrows() const;
        void setShowArrows(const bool showArrows);

        bool isApproximationEnabled() const;
        void setApproximationEnabled(const bool enabled);

        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI>& points);
        
        void attachMarker(const std::shared_ptr<MapMarker>& marker);
        void detachMarker(const std::shared_ptr<MapMarker>& marker);

        QList<float> getHeights() const;
        void setHeights(const QList<float>& heights);

        QList<FColorARGB> getColorizationMapping() const;
        void setColorizationMapping(const QList<FColorARGB>& colorizationMapping);

        QList<FColorARGB> getOutlineColorizationMapping() const;
        void setOutlineColorizationMapping(const QList<FColorARGB>& colorizationMapping);

        double getLineWidth() const;
        void setLineWidth(const double width);

        double getOutlineWidth() const;
        void setOutlineWidth(const double width);

        FColorARGB getOutlineColor() const;
        void setOutlineColor(const FColorARGB color);

        FColorARGB getNearOutlineColor() const;
        void setNearOutlineColor(const FColorARGB color);

        FColorARGB getFarOutlineColor() const;
        void setFarOutlineColor(const FColorARGB color);

        float getPathIconStep() const;
        void setPathIconStep(const float step);

        float getSpecialPathIconStep() const;
        void setSpecialPathIconStep(const float step);

        void setColorizationScheme(const int colorizationScheme);

        FColorARGB getFillColor() const;
        void setFillColor(const FColorARGB color);

        std::vector<double> getLineDash() const;
        void setLineDash(const std::vector<double> dashPattern);

        bool getElevatedLineVisibility() const;
        void setElevatedLineVisibility(const bool visible);

        bool getSurfaceLineVisibility() const;
        void setSurfaceLineVisibility(const bool visible);

        float getElevationScaleFactor() const;
        void setElevationScaleFactor(const float scaleFactor);

        sk_sp<const SkImage> getPointImage() const;

        bool hasUnappliedChanges() const;

        std::shared_ptr<SymbolsGroup> createSymbolsGroup(const MapState& mapState);
        const QList<VectorLine::OnPathSymbolData> getArrowsOnPath() const;

        OSMAND_OBSERVER_CALLABLE(Updated,
             const VectorLine* vectorLine);
        const ObservableAs<VectorLine::Updated> updatedObservable;

    friend class OsmAnd::VectorLineBuilder;
    friend class OsmAnd::VectorLineBuilder_P;
    friend class OsmAnd::VectorLinesCollection;
    friend class OsmAnd::VectorLinesCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_H_)
