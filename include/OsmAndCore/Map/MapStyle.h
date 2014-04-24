#ifndef _OSMAND_CORE_MAP_STYLE_H_
#define _OSMAND_CORE_MAP_STYLE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QIODevice>
#include <QByteArray>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapStyleBuiltinValueDefinitions.h>

namespace OsmAnd
{
    class IMapStylesCollection;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;
    class RasterizerEnvironment_P;

    class MapStyleValueDefinition;
    class MapStyleRule;

    enum class MapStyleRulesetType : uint32_t
    {
        Invalid = 0,

        Point = 1,
        Polyline = 2,
        Polygon = 3,
        Text = 4,
        Order = 5,
    };

    class MapStyle_P;
    class OSMAND_CORE_API MapStyle
    {
        Q_DISABLE_COPY(MapStyle);
    private:
        PrivateImplementation<MapStyle_P> _p;
    protected:
    public:
        MapStyle(
            /*const std::shared_ptr<const IMapStylesCollection>&*/const IMapStylesCollection* const collection,
            const QString& name,
            const std::shared_ptr<QIODevice>& source);
        MapStyle(
            /*const std::shared_ptr<IMapStylesCollection>&*/const IMapStylesCollection* const collection,
            const QString& name,
            const QString& fileName);
        MapStyle(
            /*const std::shared_ptr<IMapStylesCollection>&*/const IMapStylesCollection* const collection,
            const QString& fileName);
        virtual ~MapStyle();

        /*const std::shared_ptr<const IMapStylesCollection>&*/const IMapStylesCollection* const collection;

        const QString& title;

        const QString& name;
        const QString& parentName;

        bool isStandalone() const;
        bool loadMetadata();
        bool loadStyle();

        bool resolveValueDefinition(const QString& name, std::shared_ptr<const MapStyleValueDefinition>& outDefinition) const;
        bool resolveAttribute(const QString& name, std::shared_ptr<const MapStyleRule>& outAttribute) const;

        void dump(const QString& prefix = QString::null) const;
        void dump(MapStyleRulesetType type, const QString& prefix = QString::null) const;

        static std::shared_ptr<const MapStyleBuiltinValueDefinitions> getBuiltinValueDefinitions();

    friend class OsmAnd::MapStyle_P;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleEvaluator_P;
    friend class OsmAnd::MapStyleRule;
    friend class OsmAnd::RasterizerEnvironment_P;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_H_)
