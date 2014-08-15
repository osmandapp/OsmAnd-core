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
        struct PinPoint Q_DECL_FINAL
        {
            PointI point31;

            unsigned int basePathPointIndex;
            double offsetFromBasePathPoint31;
            float normalizedOffsetFromBasePathPoint;
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
        QList<PinPoint> pinPoints;
    };
}

#endif // !defined(_OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_)
