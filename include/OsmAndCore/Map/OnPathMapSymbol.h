#ifndef _OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_
#define _OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API OnPathMapSymbol : public MapSymbol
    {
        Q_DISABLE_COPY(OnPathMapSymbol);
    private:
    protected:
    public:
        OnPathMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const IntersectionModeFlags intersectionModeFlags,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const QVector<PointI>& path,
            const QVector<float>& glyphsWidth);
        virtual ~OnPathMapSymbol();

        const QVector<PointI> path;
        const QVector<float> glyphsWidth;

        virtual std::shared_ptr<MapSymbol> cloneWithBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const;
    };

}

#endif // !defined(_OSMAND_CORE_ON_PATH_MAP_SYMBOL_H_)
