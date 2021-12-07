#ifndef _OSMAND_CORE_VECTOR_LINE_BUILDER_P_H_
#define _OSMAND_CORE_VECTOR_LINE_BUILDER_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QVector>
#include <QHash>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "VectorLine.h"


namespace OsmAnd
{
    class VectorLinesCollection;
    class VectorLinesCollection_P;

    class VectorLineBuilder;
    class VectorLineBuilder_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLineBuilder_P);

    private:
    protected:
        VectorLineBuilder_P(VectorLineBuilder* const owner);

        mutable QReadWriteLock _lock;

        bool _isHidden;
        bool _showArrows;
        bool _isApproximationEnabled;
        
        int _lineId;
        int _baseOrder;
        int _colorizationScheme;
        
        LineEndCapStyle _endCapStyle;
        
        double _outlineWidth;
        double _lineWidth;
        FColorARGB _fillColor;
        std::vector<double> _dashPattern;

        QVector<PointI> _points;
        QList<FColorARGB> _colorizationMapping;

        float _direction;

        std::shared_ptr<const SkBitmap> _pathIcon;
        std::shared_ptr<const SkBitmap> _specialPathIcon;
        float _pathIconStep;
        float _screenScale;
 
    public:
        virtual ~VectorLineBuilder_P();

        ImplementationInterface<VectorLineBuilder> owner;

        bool isHidden() const;
        void setIsHidden(const bool hidden);
        
        bool shouldShowArrows() const;
        void setShouldShowArrows(const bool showArrows);

        bool isApproximationEnabled() const;
        void setApproximationEnabled(const bool enabled);
        
        int getColorizationScheme() const;
        void setColorizationScheme(const int colorizationScheme);
        
        int getLineId() const;
        void setLineId(const int lineId);

        int getBaseOrder() const;
        void setBaseOrder(const int baseOrder);
        
        double getOutlineWidth() const;
        void setOutlineWidth(const double width);

        double getLineWidth() const;
        void setLineWidth(const double width);
        
        FColorARGB getFillColor() const;
        void setFillColor(const FColorARGB baseColor);

        std::vector<double> getLineDash() const;
        void setLineDash(const std::vector<double> dashPattern);
        
        QVector<PointI> getPoints() const;
        void setPoints(const QVector<PointI> poinst);
        
        QList<OsmAnd::FColorARGB> getColorizationMapping() const;
        void setColorizationMapping(const QList<OsmAnd::FColorARGB>& colorizationMapping);

        std::shared_ptr<const SkBitmap> getPathIcon() const;
        void setPathIcon(const std::shared_ptr<const SkBitmap>& bitmap);
        
        std::shared_ptr<const SkBitmap> getSpecialPathIcon() const;
        void setSpecialPathIcon(const std::shared_ptr<const SkBitmap>& bitmap);

        float getPathIconStep() const;
        void setPathIconStep(const float step);
        
        float getScreenScale() const;
        void setScreenScale(const float screenScale);
        
        void setEndCapStyle(const LineEndCapStyle endCapStyle);

        std::shared_ptr<VectorLine> buildAndAddToCollection(const std::shared_ptr<VectorLinesCollection>& collection);
        std::shared_ptr<VectorLine> build();

    friend class OsmAnd::VectorLineBuilder;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_BUILDER_P_H_)
