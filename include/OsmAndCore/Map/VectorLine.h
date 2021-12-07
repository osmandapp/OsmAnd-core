#ifndef _OSMAND_CORE_VECTOR_LINE_H_
#define _OSMAND_CORE_VECTOR_LINE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QVector>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Color.h>
#include <OsmAndCore/Map/MapSymbolsGroup.h>
#include <OsmAndCore/Map/IUpdatableMapSymbolsGroup.h>
#include <Polyline2D/Polyline2D.h>

class SkBitmap;

namespace OsmAnd
{
    class VectorLineBuilder;
    class VectorLineBuilder_P;
    class VectorLinesCollection;
    class VectorLinesCollection_P;

    class VectorLine_P;

    typedef crushedpixel::Polyline2D::EndCapStyle LineEndCapStyle;
    typedef enum
    {
        kRoundJoinType = 0,
        kBezelJoinType,
        kMiterJoinType
        
    } kVectorLineJoinType;
    
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
            OnPathSymbolData(PointI position31, float direction);
            ~OnPathSymbolData();
            
            PointI position31;
            float direction;
        };

    private:
        PrivateImplementation<VectorLine_P> _p;
    protected:
        VectorLine(
            const int lineId,
            const int baseOrder,
            const std::shared_ptr<const SkBitmap>& pathIcon = nullptr,
            const std::shared_ptr<const SkBitmap>& specialPathIcon = nullptr,
            const float pathIconStep = -1,
            const float screenScale = 2,
            const LineEndCapStyle endCapStyle = LineEndCapStyle::ROUND
        );

        bool applyChanges();
    public:
        virtual ~VectorLine();

        const int lineId;
        const int baseOrder;
        const std::shared_ptr<const SkBitmap> pathIcon;
        const std::shared_ptr<const SkBitmap> specialPathIcon;
        const float pathIconStep;
        const float screenScale;
        LineEndCapStyle endCapStyle;

        bool isHidden() const;
        void setIsHidden(const bool hidden);
        
        bool showArrows() const;
        void setShowArrows(const bool showArrows);

        bool isApproximationEnabled() const;
        void setApproximationEnabled(const bool enabled);

        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI>& points);
        
        QList<FColorARGB> getColorizationMapping() const;
        void setColorizationMapping(const QList<FColorARGB>& colorizationMapping);
        
        double getLineWidth() const;
        void setLineWidth(const double width);
        
        double getOutlineWidth() const;
        void setOutlineWidth(const double width);

        void setColorizationScheme(const int colorizationScheme);

        FColorARGB getFillColor() const;
        void setFillColor(const FColorARGB color);

        std::vector<double> getLineDash() const;
        void setLineDash(const std::vector<double> dashPattern);
        
        const std::shared_ptr<const SkBitmap> getPointBitmap() const;

        bool hasUnappliedChanges() const;

        std::shared_ptr<SymbolsGroup> createSymbolsGroup(const MapState& mapState);
        const QList<OsmAnd::VectorLine::OnPathSymbolData> getArrowsOnPath() const;
        
        OSMAND_OBSERVER_CALLABLE(OnChangesApplied,
                                 const VectorLine* const vectorLine);
        const ObservableAs<VectorLine::OnChangesApplied> lineUpdatedObservable;

    friend class OsmAnd::VectorLineBuilder;
    friend class OsmAnd::VectorLineBuilder_P;
    friend class OsmAnd::VectorLinesCollection;
    friend class OsmAnd::VectorLinesCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_H_)
