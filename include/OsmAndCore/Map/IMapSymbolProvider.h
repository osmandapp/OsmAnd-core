#ifndef _OSMAND_CORE_I_MAP_SYMBOL_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_SYMBOL_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QVector>
#include <QString>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Map/IMapProvider.h>
#include <OsmAndCore/Map/IRetainableResource.h>

namespace OsmAnd
{
    namespace Model
    {
        class ObjectWithId;
    }

    class OSMAND_CORE_API IMapSymbolProvider : public IMapProvider
    {
        Q_DISABLE_COPY(IMapSymbolProvider);

    public:
        OSMAND_CALLABLE(FilterCallback, bool, const IMapSymbolProvider* const provider, const std::shared_ptr<const Model::ObjectWithId>& object, const bool shareable);

    private:
    protected:
        IMapSymbolProvider();
    public:
        virtual ~IMapSymbolProvider();
    };

    class MapSymbol;
    class OSMAND_CORE_API MapSymbolsGroup
    {
        Q_DISABLE_COPY(MapSymbolsGroup);
    private:
    protected:
    public:
        MapSymbolsGroup(const std::shared_ptr<const Model::ObjectWithId>& object);
        virtual ~MapSymbolsGroup();

        const std::shared_ptr<const Model::ObjectWithId> object;

        QList< std::shared_ptr<const MapSymbol> > symbols;
    };

    class OSMAND_CORE_API MapSymbol : public IRetainableResource
    {
        Q_DISABLE_COPY(MapSymbol);
    private:
    protected:
        std::shared_ptr<const SkBitmap> _bitmap;

        MapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance);
    public:
        virtual ~MapSymbol();

        const std::weak_ptr<const MapSymbolsGroup> group;
        const uint64_t groupObjectId;

        const bool isShareable;

        const std::shared_ptr<const SkBitmap>& bitmap;
        const int order;
        const QString content;
        const LanguageId languageId;
        const PointI minDistance;

        virtual void releaseNonRetainedData();

        virtual MapSymbol* cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const = 0;
    };

    class OSMAND_CORE_API BoundToPointMapSymbol : public MapSymbol
    {
        Q_DISABLE_COPY(BoundToPointMapSymbol);
    private:
    protected:
        BoundToPointMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const PointI& location31);
    public:
        virtual ~BoundToPointMapSymbol();

        const PointI location31;
    };

    class OSMAND_CORE_API PinnedMapSymbol : public BoundToPointMapSymbol
    {
        Q_DISABLE_COPY(PinnedMapSymbol);
    private:
    protected:
    public:
        PinnedMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const PointI& location31,
            const PointI& offset);
        virtual ~PinnedMapSymbol();

        const PointI offset;

        virtual MapSymbol* cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const;
    };

    class OSMAND_CORE_API OnSurfaceMapSymbol : public BoundToPointMapSymbol
    {
        Q_DISABLE_COPY(OnSurfaceMapSymbol);
    private:
    protected:
    public:
        OnSurfaceMapSymbol(
            const std::shared_ptr<const MapSymbolsGroup>& group,
            const bool isShareable,
            const std::shared_ptr<const SkBitmap>& bitmap,
            const int order,
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const PointI& location31,
            const double areaRadius,
            const SkColor areaBaseColor);
        virtual ~OnSurfaceMapSymbol();

        const double areaRadius;
        const SkColor areaBaseColor;

        virtual MapSymbol* cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const;
    };

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
            const QString& content,
            const LanguageId& languageId,
            const PointI& minDistance,
            const QVector<PointI>& path,
            const QVector<float>& glyphsWidth);
        virtual ~OnPathMapSymbol();

        const QVector<PointI> path;
        const QVector<float> glyphsWidth;

        virtual MapSymbol* cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& bitmap) const;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_SYMBOL_PROVIDER_H_)
