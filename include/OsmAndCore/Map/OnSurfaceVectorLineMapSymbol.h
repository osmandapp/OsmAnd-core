#ifndef _OSMAND_CORE_ON_SURFACE_VECTOR_LINE_MAP_SYMBOL_H_
#define _OSMAND_CORE_ON_SURFACE_VECTOR_LINE_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Map/OnSurfaceVectorMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API OnSurfaceVectorLineMapSymbol : public OnSurfaceVectorMapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(OnSurfaceVectorLineMapSymbol);

    private:
    protected:
        double _lineWidth;
        QVector<PointI> _points31;
    public:
        OnSurfaceVectorLineMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group);
        virtual ~OnSurfaceVectorLineMapSymbol();

        virtual double getLineWidth() const;
        virtual void setLineWidth(const double width);

        virtual QVector<PointI> getPoints31() const;
        virtual void setPoints31(const QVector<PointI>& points31);
    };
}

#endif // !defined(_OSMAND_CORE_ON_SURFACE_VECTOR_LINE_MAP_SYMBOL_H_)
