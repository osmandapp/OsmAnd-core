#ifndef _OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_
#define _OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QVector>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API OnPathMapSymbol : public RasterMapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(OnPathMapSymbol);

    public:
        struct OSMAND_CORE_API PinPoint Q_DECL_FINAL
        {
            PinPoint();
            PinPoint(
                const unsigned int basePathPointIndex,
                const double offsetFromBasePathPoint,
                const float kOffsetFromBasePathPoint,
                const PointI& point);
            ~PinPoint();

            unsigned int basePathPointIndex;
            double offsetFromBasePathPoint;
            double kOffsetFromBasePathPoint;
            PointI point;
        };

    private:
    protected:
    public:
        OnPathMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable);
        virtual ~OnPathMapSymbol();

        QVector<PointI> path;
        QVector<float> glyphsWidth;
        QVector<PinPoint> pinPoints;
    };
}

#endif // !defined(_OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_)
