#ifndef _OSMAND_CORE_BILLBOARD_RASTER_MAP_SYMBOL_H_
#define _OSMAND_CORE_BILLBOARD_RASTER_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterMapSymbol.h>
#include <OsmAndCore/Map/IBillboardMapSymbol.h>

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
        enum PositionType : unsigned int
        {
            Coordinate31 = 0,
            PrimaryGridX = 1,
            PrimaryGridY = 2,
            SecondaryGridX = 3,
            SecondaryGridY = 4
        };

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
    };
}

#endif // !defined(_OSMAND_CORE_BILLBOARD_RASTER_MAP_SYMBOL_H_)
