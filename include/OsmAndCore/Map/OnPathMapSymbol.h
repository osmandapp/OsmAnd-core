#ifndef _OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_
#define _OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/RasterMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API OnPathMapSymbol : public RasterMapSymbol
    {
        Q_DISABLE_COPY(OnPathMapSymbol);
    private:
    protected:
    public:
        OnPathMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group,
            const bool isShareable,
            const int order,
            const IntersectionModeFlags intersectionModeFlags,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const QVector<PointI>& path,
            const QVector<float>& glyphsWidth);
        virtual ~OnPathMapSymbol();

        const QVector<PointI> path;
        const QVector<float> glyphsWidth;
    };
}

#endif // !defined(_OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_)
