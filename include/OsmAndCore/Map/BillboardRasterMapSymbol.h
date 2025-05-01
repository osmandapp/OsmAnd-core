#ifndef _OSMAND_CORE_BILLBOARD_RASTER_MAP_SYMBOL_H_
#define _OSMAND_CORE_BILLBOARD_RASTER_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterMapSymbol.h>
#include <OsmAndCore/Map/IBillboardMapSymbol.h>
#include <OsmAndCore/PointsAndAreas.h>

namespace OsmAnd
{
    class OSMAND_CORE_API BillboardRasterMapSymbol
        : public RasterMapSymbol
        , public IBillboardMapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(BillboardRasterMapSymbol);
    private:
    protected:
    public:
        BillboardRasterMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group);
        virtual ~BillboardRasterMapSymbol();

        bool drawAlongPath;

        PointI offset;
        virtual PointI getOffset() const;
        virtual void setOffset(const PointI offset);

        PointI position31;
        virtual PointI getPosition31() const;
        virtual void setPosition31(const PointI position);

        PositionType positionType;
        virtual PositionType getPositionType() const;
        virtual void setPositionType(const PositionType positionType);

        double additionalPosition;
        virtual double getAdditionalPosition() const;
        virtual void setAdditionalPosition(const double additionalPosition);

        float elevation;
        float getElevation() const;
        void setElevation(const float elevation);

        float elevationScaleFactor;
        float getElevationScaleFactor() const;
        void setElevationScaleFactor(const float scaleFactor);
        
        QVector<PointI64> linePoints;
    };
}

#endif // !defined(_OSMAND_CORE_BILLBOARD_RASTER_MAP_SYMBOL_H_)
