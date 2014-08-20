#ifndef _OSMAND_CORE_ON_PATH_RASTER_MAP_SYMBOL_H_
#define _OSMAND_CORE_ON_PATH_RASTER_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QVector>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterMapSymbol.h>
#include <OsmAndCore/Map/IOnPathMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API OnPathRasterMapSymbol
        : public RasterMapSymbol
        , public IOnPathMapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(OnPathRasterMapSymbol);

    private:
    protected:
    public:
        OnPathRasterMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable);
        virtual ~OnPathRasterMapSymbol();

        QVector<float> glyphsWidth;
        QVector<PointI> path31;
        PinPoint pinPointOnPath;

        virtual QVector<PointI> getPath31() const;
        virtual void setPath31(const QVector<PointI>& path31);

        virtual PinPoint getPinPointOnPath() const;
        virtual void setPinPointOnPath(const PinPoint& pinPoint);
    };
}

#endif // !defined(_OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_)
