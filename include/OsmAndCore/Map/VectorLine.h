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

class SkBitmap;

namespace OsmAnd
{
    class VectorLineBuilder;
    class VectorLineBuilder_P;
    class VectorLinesCollection;
    class VectorLinesCollection_P;

    class VectorLine_P;
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

    private:
        PrivateImplementation<VectorLine_P> _p;
    protected:
        VectorLine(
            const int lineId,
            const int baseOrder,
            const double lineWidth,
            const std::shared_ptr<const SkBitmap>& pathIcon = nullptr,
            const float pathIconStep = -1
        );

        bool applyChanges();
    public:
        virtual ~VectorLine();

        const int lineId;
        const int baseOrder;
        const double lineWidth;
        const std::shared_ptr<const SkBitmap> pathIcon;
        const float pathIconStep;

        bool isHidden() const;
        void setIsHidden(const bool hidden);

        bool isApproximationEnabled() const;
        void setApproximationEnabled(const bool enabled);

        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI>& points);
        
        FColorARGB getFillColor() const;
        void setFillColor(const FColorARGB color);

        std::vector<double> getLineDash() const;
        void setLineDash(const std::vector<double> dashPattern);

        bool hasUnappliedChanges() const;

        std::shared_ptr<SymbolsGroup> createSymbolsGroup(const MapState& mapState);

    friend class OsmAnd::VectorLineBuilder;
    friend class OsmAnd::VectorLineBuilder_P;
    friend class OsmAnd::VectorLinesCollection;
    friend class OsmAnd::VectorLinesCollection_P;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_H_)
